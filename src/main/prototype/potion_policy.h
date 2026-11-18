// SPDX-License-Identifier: MIT
#pragma once
#include "core/Filter.h"
#include "ordinary_inventory_provider.h"
#include <array>
#include <memory>
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
};
// Main-phase observations only. Controller callbacks receive an immutable copy.
struct PolicySnapshot {
    std::uint64_t capturedNs{}, epoch{}, save{}, instance{};
    bool worn{}, available{};
    Core::Filter filter;
    std::shared_ptr<const std::vector<Potion>> stock;
    std::array<HeldPotion, 2> held;
};
[[nodiscard]] const Potion* NextPotion(const PolicySnapshot&) noexcept;
[[nodiscard]] std::uint64_t MatchingCount(const PolicySnapshot&) noexcept;
[[nodiscard]] bool PlanPotion(const PolicySnapshot&, const body_slots::SourceActionContext&,
    std::uint64_t nowNs, body_slots::examples::OrdinaryItemPlan&) noexcept;
}
