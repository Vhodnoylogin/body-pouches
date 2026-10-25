#include "Harness.h"
#include "FakePack.h"
#include "core/Assignments.h"
#include "core/LoadGate.h"
#include <algorithm>

using namespace BodyPouches::Core;

namespace
{
	Filter Effects(std::initializer_list<FormKey> keys) { return { {}, keys }; }
	Assignments Health() { return { { 13, Effects({ { "Skyrim.esm", 0x4250 } }) } }; }
	struct Records : AssignmentRecords
	{
		struct Record { std::uint32_t type, version, length; std::vector<std::uint8_t> bytes; };
		std::vector<Record> records;
		std::size_t next{};
		int reads{};
		bool Next(std::uint32_t& type, std::uint32_t& version, std::uint32_t& length) override
		{
			if (next == records.size()) { return false; }
			const auto& record = records[next++];
			type = record.type; version = record.version; length = record.length;
			return true;
		}
		std::uint32_t Read(std::span<std::uint8_t> bytes) override
		{
			++reads;
			const auto& stored = records[next - 1].bytes;
			const auto size = std::min(bytes.size(), stored.size());
			std::copy_n(stored.begin(), size, bytes.begin());
			return static_cast<std::uint32_t>(size);
		}
		void Add(std::uint32_t type = kAssignmentRecord, std::uint32_t version = kAssignmentVersion)
		{
			const auto bytes = *EncodeAssignments(Health());
			records.push_back({ type, version, static_cast<std::uint32_t>(bytes.size()), bytes });
		}
	};
}

TEST(assignment_save_roundtrip_preserves_multi_effect_and_runtime_exact_keys)
{
	Assignments original{ { 13, Effects({ { "Skyrim.esm", 0x4250 }, { "Mod.esp", 0xABC } }) },
		{ 14, { { "", 0xFF000123 }, {} } } };
	const auto bytes = EncodeAssignments(original);
	CHECK(bytes.has_value());
	const auto loaded = DecodeAssignments(*bytes);
	CHECK(loaded.has_value());
	CHECK_EQ(loaded->size(), 2U);
	CHECK_EQ(loaded->at(0).filter.effects, original[0].filter.effects);
	CHECK_EQ(loaded->at(1).filter.exact, original[1].filter.exact);
}

TEST(assignment_empty_legacy_restore_clears_prior_save_and_hand_attribution)
{
	PouchSet pouches;
	pouches.Set(Pouch(13, Mode::Exclusive));
	RestoreAssignments(pouches, Health());
	pouches.NoteDrawn(true, 13, { "Skyrim.esm", 0x3EADE });
	CHECK(pouches.Find(13)->IsAssigned());
	RestoreAssignments(pouches, {});
	CHECK(!pouches.Find(13)->IsAssigned());
	CHECK(!pouches.SlotOfHand(true).has_value());
}

TEST(assignment_save_a_b_a_isolation_preserves_current_slot_configuration)
{
	PouchSet pouches;
	pouches.Set(Pouch(13, Mode::Shared));
	pouches.Set(Pouch(14, Effects({ { "Old.esp", 1 } }), Mode::Exclusive));
	const Assignments mana{ { 13, Effects({ { "Skyrim.esm", 0x4251 } }) }, { 12, Effects({ { "Mod.esp", 4 } }) } };
	RestoreAssignments(pouches, Health());
	RestoreAssignments(pouches, mana);
	CHECK_EQ(pouches.Find(13)->PouchFilter().effects[0].localId, 0x4251U);
	CHECK(!pouches.Find(14)->IsAssigned());
	CHECK(pouches.Find(12) == nullptr);
	RestoreAssignments(pouches, Health());
	CHECK_EQ(pouches.Find(13)->PouchFilter().effects[0].localId, 0x4250U);
	CHECK(pouches.Find(13)->PouchMode() == Mode::Shared);
	CHECK_EQ(CaptureAssignments(pouches).size(), 1U);
}

TEST(assignment_loading_missing_effect_rejects_whole_filter_not_partial_meaning)
{
	const Assignments source{ { 13, Effects({ { "Skyrim.esm", 1 }, { "Missing.esp", 2 } }) },
		{ 14, Effects({ { "Skyrim.esm", 3 } }) } };
	const auto loaded = ResolveAssignments(source, [](const FormKey& key, bool effect) -> std::optional<FormKey> {
		CHECK(effect);
		return key.plugin == "Missing.esp" ? std::nullopt : std::optional<FormKey>(key);
	});
	CHECK_EQ(loaded.size(), 1U);
	CHECK_EQ(loaded[0].slot, 14);
}

TEST(assignment_runtime_remapping_and_missing_exact_are_explicit)
{
	Assignments source{ { 13, { { "", 0xFF000123 }, {} } }, { 14, { { "Gone.esp", 9 }, {} } } };
	const auto loaded = ResolveAssignments(source, [](const FormKey& key, bool effect) -> std::optional<FormKey> {
		CHECK(!effect);
		if (key.plugin == "Gone.esp") { return std::nullopt; }
		return FormKey{ "", 0xFF000456 };
	});
	CHECK_EQ(loaded.size(), 1U);
	CHECK_EQ(loaded[0].filter.exact.localId, 0xFF000456U);
}

TEST(assignment_static_identity_survives_load_order_change)
{
	const auto saved = Health();
	const auto loaded = ResolveAssignments(saved, [](const FormKey& key, bool) -> std::optional<FormKey> { return key; });
	CHECK_EQ(loaded[0].filter.effects[0], saved[0].filter.effects[0]);
	// Neither key contains the old or new full load-order index.
	CHECK_EQ(loaded[0].filter.effects[0].plugin, std::string("Skyrim.esm"));
}

TEST(assignment_decoder_rejects_every_truncated_prefix_and_trailing_data)
{
	const auto bytes = *EncodeAssignments(Health());
	for (std::size_t size = 0; size < bytes.size(); ++size) {
		CHECK(!DecodeAssignments(std::span(bytes.data(), size)).has_value());
	}
	auto extra = bytes;
	extra.push_back(0);
	CHECK(!DecodeAssignments(extra).has_value());
}

TEST(assignment_decoder_rejects_duplicate_slots_bad_counts_and_unbounded_data)
{
	auto bytes = *EncodeAssignments(Health());
	const std::vector<std::uint8_t> entry(bytes.begin() + 4, bytes.end());
	bytes[0] = 2;
	bytes.insert(bytes.end(), entry.begin(), entry.end());
	CHECK(!DecodeAssignments(bytes).has_value());
	bytes[0] = 255;
	CHECK(!DecodeAssignments(bytes).has_value());
	CHECK(!DecodeAssignments(std::vector<std::uint8_t>(kAssignmentBytes + 1)).has_value());
}

TEST(assignment_encoder_rejects_ambiguous_or_wildcard_runtime_keys)
{
	CHECK(!EncodeAssignments({ { 13, { { "", 0x123 }, {} } } }).has_value());
	CHECK(!EncodeAssignments({ { 13, { { "Skyrim.esm", 0x3EADE }, { { "Skyrim.esm", 1 } } } } }).has_value());
	CHECK(!EncodeAssignments({ { 15, Effects({ { "Skyrim.esm", 1 } }) } }).has_value());
	CHECK(!EncodeAssignments({ { 13, Effects({ { std::string(256, 'x'), 1 } }) } }).has_value());
	CHECK(!EncodeAssignments({ { 13, Filter{} } }).has_value());
}

TEST(assignment_empty_save_is_explicit_not_absent_or_inherited)
{
	const auto bytes = EncodeAssignments({});
	CHECK(bytes.has_value());
	CHECK_EQ(bytes->size(), 4U);
	const auto loaded = DecodeAssignments(*bytes);
	CHECK(loaded.has_value());
	CHECK(loaded->empty());
}

TEST(assignment_record_reader_missing_and_unknown_records_are_empty)
{
	Records reader;
	CHECK(ReadAssignmentRecords(reader)->empty());
	reader.Add(0x12345678);
	CHECK(ReadAssignmentRecords(reader)->empty());
	CHECK_EQ(reader.reads, 0);
}

TEST(assignment_record_reader_rejects_duplicate_unsupported_oversized_and_short_records)
{
	Records duplicate;
	duplicate.Add(); duplicate.Add();
	CHECK(!ReadAssignmentRecords(duplicate));
	Records version;
	version.Add(kAssignmentRecord, 2);
	CHECK(!ReadAssignmentRecords(version));
	CHECK_EQ(version.reads, 0);
	Records oversized;
	oversized.Add(); oversized.records[0].length = static_cast<std::uint32_t>(kAssignmentBytes + 1);
	CHECK(!ReadAssignmentRecords(oversized));
	CHECK_EQ(oversized.reads, 0);
	Records shortRecord;
	shortRecord.Add(); shortRecord.records[0].bytes.pop_back();
	CHECK(!ReadAssignmentRecords(shortRecord));
}

TEST(assignment_record_reader_skips_unknown_record_then_reads_current_assignment)
{
	Records reader;
	reader.Add(0x12345678);
	reader.Add();
	const auto loaded = ReadAssignmentRecords(reader);
	CHECK(loaded.has_value());
	CHECK_EQ(loaded->size(), 1U);
	CHECK_EQ(reader.reads, 1);
}

TEST(load_gate_blocks_active_load_through_revert_until_completion)
{
	LoadGate gate;
	gate.Begin();
	gate.Revert();
	CHECK(gate.Blocked());
	gate.Complete();
	CHECK(!gate.Blocked());
}

TEST(load_gate_failed_load_then_new_game_revert_needs_no_new_game_message)
{
	LoadGate gate;
	gate.Begin();
	gate.Fail();
	CHECK(gate.Blocked());
	gate.Revert();
	CHECK(!gate.Blocked());
}

TEST(load_gate_failed_load_can_be_retried_and_normal_new_game_revert_is_idle)
{
	LoadGate gate;
	gate.Revert();
	CHECK(!gate.Blocked());
	gate.Begin(); gate.Fail(); gate.Begin(); gate.Revert();
	CHECK(gate.Blocked());
	gate.Complete();
	CHECK(!gate.Blocked());
}
