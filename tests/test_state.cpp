#include "Harness.h"
#include "core/PouchSet.h"
#include "diagnostics/Contract.h"

using namespace BodyPouches;

TEST(assignment_diagnostics_expose_actual_filter_identity_not_only_count)
{
	Diagnostics::State state;
	state.available = true;
	state.pouches.push_back({ 13, Core::Mode::Exclusive, true });
	state.assignments.push_back({ 13, { {}, { { "Skyrim.esm", 0x4250 } } } });
	const auto result = Diagnostics::StateValues(state);
	const auto& entry = result.at("runtime").at("assignments").at(0);
	CHECK_EQ(entry.at("slot"), 13);
	CHECK_EQ(entry.at("effects").at(0).at("plugin"), "Skyrim.esm");
	CHECK_EQ(entry.at("effects").at(0).at("localId"), 0x4250);
}

TEST(shared_reach_diagnostics_keep_units_physical_mapping_and_unavailable_distance)
{
	Diagnostics::State state;
	state.available = true;
	state.handReach.push_back({ true, false, 0, false, 13, 1.25, 10, 13 });
	state.handReach.push_back({ false, true, 0, false, 0, std::nullopt, 10, 0 });
	const auto result = Diagnostics::StateValues(state);
	CHECK_EQ(result.at("handReach").at(0).at("physicalHand"), "left");
	CHECK_EQ(result.at("handReach").at(0).at("secondary"), false);
	CHECK_EQ(result.at("handReach").at(0).at("distanceGameUnits"), 1.25);
	CHECK_EQ(result.at("handReach").at(0).at("activationRadiusGameUnits"), 10);
	CHECK(result.at("handReach").at(1).at("distanceGameUnits").is_null());
}

TEST(expired_canceled_or_old_world_reads_cannot_start_or_return_values)
{
	Diagnostics::ReadGate expired(100, 7);
	CHECK(!expired.Begin(100, 7));
	Diagnostics::ReadGate canceled(100, 7);
	canceled.Cancel();
	CHECK(!canceled.Begin(10, 7));
	Diagnostics::ReadGate oldWorld(100, 7);
	CHECK(!oldWorld.Begin(10, 8));
	Diagnostics::ReadGate reading(100, 7);
	CHECK(reading.Begin(10, 7));
	CHECK(reading.Valid(99, 7));
	CHECK(!reading.Valid(100, 7));
	CHECK(!reading.Valid(20, 8));
	reading.Cancel();
	CHECK(!reading.Valid(20, 7));
}

TEST(unavailable_native_state_never_exports_default_settings_or_empty_assignments)
{
	Diagnostics::State state;
	state.settings.pouches.push_back({ 13, "exclusive" });
	const auto result = Diagnostics::StateValues(state);
	CHECK_EQ(result.at("available"), false);
	CHECK(!result.contains("settings"));
	CHECK(!result.contains("runtime"));
}

TEST(actual_normalized_pouches_and_assignments_win_over_requested_settings)
{
	Core::PouchSet pouches;
	pouches.Set(Core::Pouch(13, Core::Mode::Exclusive));
	pouches.Set(Core::Pouch(13, Core::Mode::Shared));
	pouches.Set(Core::Pouch(14, Core::Mode::Exclusive));
	Core::Item potion;
	potion.key = { "Skyrim.esm", 0x03EADD };
	potion.effects = { { "Skyrim.esm", 0x03EAF3 } };
	potion.count = 1;
	CHECK(pouches.Assign(14, potion));
	Diagnostics::State state;
	state.available = true;
	state.settings.pouches = { {13, "exclusive"}, {13, "unknown-mode"} };
	state.pouches = pouches.Snapshot();
	state.inputLeftHanded = false;
	const auto result = Diagnostics::StateValues(state);
	CHECK_EQ(result.at("settings").at("pouches").size(), 2U);
	CHECK_EQ(result.at("settings").at("pouches").at(0).at("mode"), "shared");
	CHECK_EQ(result.at("runtime").at("assignmentCount"), 1);
	CHECK_EQ(result.at("settings").at("inputHandedness"), "right");
	CHECK_EQ(pouches.Find(13)->PouchMode(), Core::Mode::Shared);
	CHECK_EQ(pouches.Size(), 2U);
	pouches.Clear();
	CHECK_EQ(Diagnostics::StateValues(state).at("runtime").at("assignmentCount"), 1);
}

TEST(slot_suspension_is_measured_not_inferred_from_exclusive_mode)
{
	Diagnostics::State state;
	state.available = true;
	state.pouches.push_back({ 13, Core::Mode::Exclusive, false });
	state.bodySlots = { { 13, false }, { 14, std::nullopt } };
	const auto result = Diagnostics::StateValues(state);
	CHECK_EQ(result.at("bodySlots").at(0).at("suspended"), false);
	CHECK(result.at("bodySlots").at(1).at("suspended").is_null());
	CHECK_EQ(result.at("bodySlots").at(1).at("suspendedAvailable"), false);
}

TEST(missing_game_handedness_is_unavailable_instead_of_guessed_right_handed)
{
	Diagnostics::State state;
	state.available = true;
	const auto result = Diagnostics::StateValues(state);
	CHECK(result.at("settings").at("inputHandedness").is_null());
	CHECK_EQ(result.at("inputHandednessAvailable"), false);
}

TEST(empty_pouch_configuration_still_reads_actual_all_slot_suspension)
{
	Diagnostics::State state;
	state.available = true;
	int reads = 0;
	state.bodySlots = Diagnostics::ReadBodySlots(true, [&reads](int a_slot) {
		++reads;
		CHECK(a_slot >= 1 && a_slot <= 14);
		return a_slot == 13;  // VRIK may be suspended by another consumer.
	});
	const auto result = Diagnostics::StateValues(state);
	CHECK_EQ(reads, 14);
	CHECK(result.at("settings").at("pouches").empty());
	CHECK_EQ(result.at("runtime").at("assignmentCount"), 0);
	CHECK_EQ(result.at("bodySlots").size(), 14U);
	CHECK_EQ(result.at("bodySlots").at(12).at("slot"), 13);
	CHECK_EQ(result.at("bodySlots").at(12).at("suspended"), true);
	CHECK_EQ(result.at("bodySlots").at(13).at("slot"), 14);
	CHECK_EQ(result.at("bodySlots").at(13).at("suspended"), false);
}

TEST(unavailable_vrik_is_not_read_and_does_not_become_false_for_any_slot)
{
	int reads = 0;
	const auto slots = Diagnostics::ReadBodySlots(false, [&reads](int) { ++reads; return false; });
	CHECK_EQ(reads, 0);
	CHECK_EQ(slots.size(), 14U);
	for (const auto& slot : slots) {
		CHECK(slot.slot >= 1 && slot.slot <= 14);
		CHECK(!slot.suspended.has_value());
	}
}

TEST(causal_history_disabled_is_explicitly_unavailable_not_complete_empty_capture)
{
	Diagnostics::State state;
	state.available = true;
	const auto result = Diagnostics::StateValues(state).at("causalTrace");
	CHECK_EQ(result.at("enabled"), false);
	CHECK_EQ(result.at("available"), false);
	CHECK(result.at("events").empty());
}

TEST(causal_history_serializes_native_identity_deadline_loss_and_global_null_hand)
{
	Diagnostics::State state;
	state.available = true;
	state.trace.enabled = true;
	state.trace.available = true;
	state.trace.nextSequence = 10;
	state.trace.overwritten = 2;
	state.trace.busyDrops = 1;
	Diagnostics::TraceEvent event;
	event.event = Diagnostics::Event::PendingArmed;
	event.sequence = 9;
	event.handle = 0xF0123456;
	event.pendingHandle = event.handle;
	event.pendingActive = true;
	event.monotonicMs = 1000;
	event.deadlineMs = 1200;
	state.trace.events.push_back(event);
	const auto result = Diagnostics::StateValues(state).at("causalTrace");
	CHECK_EQ(result.at("overwritten"), 2);
	CHECK_EQ(result.at("busyDrops"), 1);
	CHECK_EQ(result.at("events").at(0).at("handle").get<std::uint32_t>(), 0xF0123456);
	CHECK_EQ(result.at("events").at(0).at("deadlineMs"), 1200);
	CHECK(result.at("events").at(0).at("physicalHand").is_null());
	CHECK_EQ(result.at("events").at(0).at("event"), "pending.armed");
}
