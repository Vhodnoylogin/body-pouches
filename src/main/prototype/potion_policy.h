// SPDX-License-Identifier: MIT
#pragma once
#include "core/Filter.h"
#include "ordinary_inventory_provider.h"
#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace BodyPouches::Prototype {
struct Potion {
    Core::Item item;
    std::uint32_t form{}, ordinal{};
    std::string model;
};
struct HeldPotion {
    Potion potion;
    std::uint32_t reference{}, quantity{};
    bool assignable{}; // pure save-codec preflight performed on the safe main phase
    std::optional<Core::Filter> assignment; // Same preflight result, retained across transfer.
    body_slots::examples::ContentProjection assignmentProjection;
};
// Main-phase observations only. Controller callbacks receive an immutable copy.
struct PolicySnapshot {
    std::uint64_t capturedNs{}, epoch{}, save{}, instance{};
    std::uint64_t assignmentGeneration{};
    bool available{}; // Includes Framework's equipment/body/activation gate.
    Core::Filter filter;
    body_slots::examples::ContentProjection assignmentProjection;
    std::shared_ptr<const std::vector<Potion>> stock;
    std::array<HeldPotion, 2> held;
    std::array<std::shared_ptr<const void>, 2> storeContexts;
};
[[nodiscard]] const Potion* NextPotion(const PolicySnapshot&) noexcept;
[[nodiscard]] std::uint64_t MatchingCount(const PolicySnapshot&) noexcept;
// Logical filter identity/cardinality; never an assertion about inventory stock.
[[nodiscard]] body_slots::examples::ContentProjection AssignmentProjection(const Core::Filter&) noexcept;
[[nodiscard]] bool PlanPotion(const PolicySnapshot&, const body_slots::SourceActionContext&,
    std::uint64_t nowNs, body_slots::examples::OrdinaryItemPlan&) noexcept;
}
