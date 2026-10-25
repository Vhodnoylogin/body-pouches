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
		result["runtime"]["assignments"] = json::array();
		const auto keyValue = [](const Core::FormKey& key) {
			return json{ { "plugin", key.plugin }, { "localId", key.localId } };
		};
		for (const auto& assignment : a_state.assignments) {
			json effects = json::array();
			for (const auto& effect : assignment.filter.effects) { effects.push_back(keyValue(effect)); }
			result["runtime"]["assignments"].push_back({ { "slot", assignment.slot },
				{ "exact", keyValue(assignment.filter.exact) }, { "effects", effects } });
		}
		result["bodySlots"] = json::array();
		for (const auto& slot : a_state.bodySlots) {
			result["bodySlots"].push_back({ { "slot", slot.slot },
				{ "suspendedAvailable", slot.suspended.has_value() },
				{ "suspended", slot.suspended ? json(*slot.suspended) : json(nullptr) },
				{ "basis", "VRIK GetSlotSuspended; not inferred from pouch mode" } });
		}
		result["inputHandednessAvailable"] = a_state.inputLeftHanded.has_value();
		result["handReach"] = json::array();
		for (const auto& hand : a_state.handReach) {
			result["handReach"].push_back({ { "physicalHand", hand.physicalLeft ? "left" : "right" },
				{ "secondary", hand.secondary }, { "filteredSlot", hand.filteredSlot },
				{ "canBeHolstered", hand.canBeHolstered }, { "sharedCandidate", hand.sharedCandidate },
				{ "distanceGameUnits", hand.distanceGameUnits ? json(*hand.distanceGameUnits) : json(nullptr) },
				{ "activationRadiusGameUnits", hand.activationRadiusGameUnits },
				{ "activationRadiusAvailable", hand.activationRadiusGameUnits > 0 },
				{ "armedSharedSlot", hand.armedSharedSlot },
				{ "basis", "Filtered VRIK reach/CanBeHolstered and current first-person hand to third-person slot bone; sequential main-thread reads, no physics/contact or private weapon state" } });
		}
		result["inputHandednessBasis"] = "game bLeftHandedMode:VRInput; read on game thread, not configuration file";
		const auto& trace = a_state.trace;
		result["causalTrace"] = { { "enabled", trace.enabled }, { "available", trace.available },
			{ "reason", !trace.enabled ? "disabled; opt in with logLevel debug or trace" :
				(trace.available ? "bounded Body callback/task history; not complete HIGGS ownership" : "capture_busy") },
			{ "capacity", Trace::kCapacity }, { "nextSequence", trace.nextSequence },
			{ "overwritten", trace.overwritten }, { "busyDrops", trace.busyDrops },
			{ "clock", "std::chrono::steady_clock milliseconds; process-local epoch; same clock as PendingGrab deadline" },
			{ "phaseBasis", "named Body callback or SKSE task; higgsUpdate/callbackFrame are Body callback counters, not engine/render/physics frames" },
			{ "coverage", "Only instrumented Body paths. No complete native ownership, consumer, render or race-scheduling guarantee. Sequence gaps or capture loss make strict intervals unavailable." },
			{ "events", json::array() } };
		for (const auto& event : trace.events) {
			result["causalTrace"]["events"].push_back({ { "event", Name(event.event) },
				{ "sequence", event.sequence }, { "monotonicMs", event.monotonicMs },
				{ "worldGeneration", event.worldGeneration }, { "higgsUpdate", event.higgsUpdate },
				{ "callbackFrame", event.callbackFrame }, { "inHiggsUpdate", event.inHiggsUpdate },
				{ "physicalHand", event.physicalHand < 0 ? json(nullptr) : json(event.physicalHand == 0 ? "left" : "right") },
				{ "secondaryHand", event.secondaryHand < 0 ? json(nullptr) : json(event.secondaryHand == 1) },
				{ "slot", event.slot }, { "handle", event.handle }, { "pendingHandle", event.pendingHandle },
				{ "pendingActive", event.pendingActive }, { "deadlineMs", event.deadlineMs },
				{ "causalId", event.causalId }, { "dropSerial", event.dropSerial },
				{ "enqueuedWorldGeneration", event.enqueuedWorldGeneration }, { "quantityArgument", event.quantity } });
		}
		return result;
	}
}
