#include "Harness.h"
#include "core/PouchSet.h"
#include "diagnostics/Contract.h"

using namespace BodyPouches;

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
