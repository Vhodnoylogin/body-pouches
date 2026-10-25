#pragma once

#include <array>

namespace BodyPouches::Core
{
	// What to do with the VRIK slot a pouch sits on, decided away from VRIK.
	//
	// The decision itself is three lines of reasoning and no calls at all, which is
	// exactly why it lives here: in the adapter it could only ever be tried by loading
	// the game with a slot switched off by hand, and here it is tried in a second on any
	// machine. The adapter is left with the doing - ask VRIK, say the line, suspend.

	// Everything the decision depends on. Gathered by the adapter, which is the only
	// part that knows how to ask VRIK anything.
	struct SlotFacts
	{
		int  slot{};
		bool detectable{};   // visible and configured for this pouch mode; not proof of live reach
		bool exclusive{};    // the pouch was given this slot outright
		bool maySwitchOn{};  // the player allowed the mod to switch a slot on itself
	};

	// Three independent steps, not three choices: a slot that is off and that we are
	// allowed to switch on is switched on AND then taken over.
	struct SlotPlan
	{
		bool switchOn{};
		bool complain{};
		bool suspend{};
	};

	[[nodiscard]] constexpr SlotPlan PlanFor(const SlotFacts& a_facts) noexcept
	{
		SlotPlan plan;

		if (!a_facts.detectable) {
			// Visibility belongs to the player. Shared slots also retain a native
			// weapon category; an exclusive suspended pouch needs no such category.
			plan.switchOn = a_facts.maySwitchOn;
			plan.complain = !a_facts.maySwitchOn;
		}

		// Only exclusive pouches suspend the native weapon behavior. These settings
		// are prerequisites; VRIK still owns actual hand/zone/action decisions.
		plan.suspend = a_facts.exclusive && (a_facts.detectable || plan.switchOn);

		return plan;
	}

	struct SlotEnablePlan
	{
		bool visibility{};
		bool smallWeapons{};
	};

	[[nodiscard]] constexpr bool AllowsNativeWeapons(const std::array<double, 6>& a_allows) noexcept
	{
		for (const auto value : a_allows) {
			// The native compatibility predicate also accepts unordered values.
			if (!(value < 0.5)) { return true; }
		}
		return false;
	}

	[[nodiscard]] constexpr bool ConfiguredForPouch(const std::array<double, 6>& a_allows,
		double a_visible, bool a_exclusive) noexcept
	{
		return a_visible > 0.5 && (a_exclusive || AllowsNativeWeapons(a_allows));
	}

	[[nodiscard]] constexpr SlotEnablePlan EnablePlan(const std::array<double, 6>& a_allows,
		double a_visible, bool a_exclusive) noexcept
	{
		return { !(a_visible > 0.5), !a_exclusive && !AllowsNativeWeapons(a_allows) };
	}
}
