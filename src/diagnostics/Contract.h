#pragma once

#include "diagnostics/State.h"
#include <nlohmann/json.hpp>

namespace BodyPouches::Diagnostics
{
	inline nlohmann::json StateValues(const State& a_state)
	{
		using nlohmann::json;
		json result{ { "available", a_state.available }, { "reason", a_state.reason },
			{ "loadGeneration", a_state.loadGeneration },
			{ "callbackFrame", a_state.callbackFrame },
			{ "callbackFrameBasis", "BodyPouches HIGGS post-VRIK/post-HIGGS callback counter; not engine/render frame" } };
		if (!a_state.available) {
			return result;  // No guessed defaults, empty assignments or stale settings.
		}
		json pouches = json::array();
		std::size_t assignments = 0;
		for (const auto& pouch : a_state.pouches) {
			pouches.push_back({ { "slot", pouch.slot },
				{ "mode", pouch.mode == Core::Mode::Exclusive ? "exclusive" : "shared" } });
			assignments += pouch.assigned ? 1U : 0U;
		}
		result["settings"] = { { "language", a_state.settings.language },
			{ "logLevel", a_state.settings.logLevel },
			{ "mayEnableSlots", a_state.settings.mayEnableSlots }, { "pouches", pouches },
			{ "inputHandedness", a_state.inputLeftHanded ? json(*a_state.inputLeftHanded ? "left" : "right") : json(nullptr) } };
		result["runtime"] = { { "assignmentCount", assignments }, { "assignmentCountUnits", "assigned_pouches" } };
		result["bodySlots"] = json::array();
		for (const auto& slot : a_state.bodySlots) {
			result["bodySlots"].push_back({ { "slot", slot.slot },
				{ "suspendedAvailable", slot.suspended.has_value() },
				{ "suspended", slot.suspended ? json(*slot.suspended) : json(nullptr) },
				{ "basis", "VRIK GetSlotSuspended; not inferred from pouch mode" } });
		}
		result["inputHandednessAvailable"] = a_state.inputLeftHanded.has_value();
		result["inputHandednessBasis"] = "game bLeftHandedMode:VRInput; read on game thread, not configuration file";
		return result;
	}
}
