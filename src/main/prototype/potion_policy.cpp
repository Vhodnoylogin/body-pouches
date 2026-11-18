// SPDX-License-Identifier: MIT
#include "potion_policy.h"
#include <algorithm>
#include <limits>
#include <tuple>
#include <string_view>

namespace BodyPouches::Prototype {
const Potion* NextPotion(const PolicySnapshot& snapshot) noexcept {
    const Potion* next = nullptr;
    for (const auto& potion : snapshot.stock) {
        if (potion.item.count <= 0 || !snapshot.filter.Matches(potion.item)) continue;
        const auto rank = [](const Potion& p) {
            return std::tuple(p.item.effects.size(), p.item.strength, std::string_view(p.item.key.plugin), p.item.key.localId);
        };
        if (!next || rank(potion) < rank(*next)) next = &potion;
    }
    return next;
}
std::uint64_t MatchingCount(const PolicySnapshot& snapshot) noexcept {
    std::uint64_t count = 0;
    for (const auto& potion : snapshot.stock)
        if (potion.item.count > 0 && snapshot.filter.Matches(potion.item))
            count += static_cast<std::uint32_t>(potion.item.count);
    return count;
}
bool PlanPotion(const PolicySnapshot& snapshot, const body_slots::SourceActionContext& context,
    std::uint64_t nowNs, body_slots::examples::OrdinaryItemPlan& plan) noexcept {
    const auto& input = context.context.input;
    const auto& slot = context.context.slot;
    if (!snapshot.worn || !snapshot.available || !snapshot.capturedNs || nowNs < snapshot.capturedNs ||
        nowNs - snapshot.capturedNs > 150000000 || input.epoch != snapshot.epoch ||
        slot.configured.instance.instanceHandle != snapshot.instance ||
        slot.epoch != snapshot.epoch || input.physicalHand < 1 || input.physicalHand > 2 ||
        input.button != HIGGS_INPUT_GRIP) return false;
    plan = {};
    if (input.edge == HIGGS_INPUT_DOWN && input.handOccupancy == HIGGS_OCCUPANCY_EMPTY) {
        const auto* next = NextPotion(snapshot);
        if (!next) return false;
        plan.transferKind = HIGGS_TRANSFER_INVENTORY_TO_HAND;
        plan.baseFormId = next->form;
        plan.stackOrdinal = next->ordinal;
        plan.quantity = 1;
        return true;
    }
    if (input.edge != HIGGS_INPUT_UP) return false;
    const auto& incoming = context.items[0];
    const auto& held = snapshot.held[input.physicalHand - 1];
    if (!incoming.known || !incoming.allowed || incoming.source != 1 || !held.reference ||
        incoming.referenceId != held.reference || incoming.baseFormId != held.potion.form ||
        !held.quantity || held.quantity > static_cast<std::uint32_t>((std::numeric_limits<std::int32_t>::max)()) ||
        (snapshot.filter.IsSet() && !snapshot.filter.Matches(held.potion.item))) return false;
    plan.transferKind = HIGGS_TRANSFER_HAND_TO_INVENTORY;
    plan.baseFormId = held.potion.form;
    plan.quantity = held.quantity;
    // The helper requires an after-projection. Hide it until a fresh main-phase
    // inventory observation: a planned count is not a factual inventory count.
    return true;
}
}
