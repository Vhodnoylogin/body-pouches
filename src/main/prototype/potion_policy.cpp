// SPDX-License-Identifier: MIT
#include "potion_policy.h"
#include <algorithm>
#include <limits>
#include <tuple>
#include <string_view>

namespace BodyPouches::Prototype {
body_slots::examples::ContentProjection AssignmentProjection(const Core::Filter& filter) noexcept {
    if(!filter.IsSet() || filter.effects.size()>64)return {};
    std::uint64_t hash=14695981039346656037ull;
    const auto byte=[&](std::uint8_t value){hash=(hash^value)*1099511628211ull;};
    const auto number=[&](std::uint64_t value){for(unsigned shift=0;shift<64;shift+=8)byte(static_cast<std::uint8_t>(value>>shift));};
    const auto key=[&](const Core::FormKey& value){number(value.plugin.size());for(unsigned char c:value.plugin)byte(c);number(value.localId);};
    if(filter.exact.IsSet()){byte(1);key(filter.exact);}
    else {
        byte(2);
        std::array<const Core::FormKey*,64> keys{};
        for(std::size_t i=0;i<filter.effects.size();++i)keys[i]=&filter.effects[i];
        const auto end=keys.begin()+filter.effects.size();
        std::sort(keys.begin(),end,[](const auto* a,const auto* b){return std::tie(a->plugin,a->localId)<std::tie(b->plugin,b->localId);});
        const auto unique=std::unique(keys.begin(),end,[](const auto* a,const auto* b){return *a==*b;});
        number(static_cast<std::uint64_t>(unique-keys.begin()));
        for(auto it=keys.begin();it!=unique;++it)key(**it);
    }
    return {hash?hash:1,1};
}
const Potion* NextPotion(const PolicySnapshot& snapshot) noexcept {
    const Potion* next = nullptr;
    if (!snapshot.stock) return nullptr;
    for (const auto& potion : *snapshot.stock) {
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
    if (!snapshot.stock) return 0;
    for (const auto& potion : *snapshot.stock)
        if (potion.item.count > 0 && snapshot.filter.Matches(potion.item))
            count += static_cast<std::uint32_t>(potion.item.count);
    return count;
}
bool PlanPotion(const PolicySnapshot& snapshot, const body_slots::SourceActionContext& context,
    std::uint64_t nowNs, body_slots::examples::OrdinaryItemPlan& plan) noexcept {
    const auto& input = context.context.input;
    const auto& slot = context.context.slot;
    if (!snapshot.available || !snapshot.capturedNs || nowNs < snapshot.capturedNs ||
        nowNs - snapshot.capturedNs > 150000000 || input.epoch != snapshot.epoch ||
        slot.configured.instance.instanceHandle != snapshot.instance ||
        slot.epoch != snapshot.epoch || input.physicalHand < 1 || input.physicalHand > 2 ||
        input.button != HIGGS_INPUT_GRIP) return false;
    plan = {};
    if (input.edge == HIGGS_INPUT_DOWN && input.handOccupancy == HIGGS_OCCUPANCY_EMPTY) {
        const auto* next = NextPotion(snapshot);
        if (!next || !snapshot.assignmentProjection.identity || snapshot.assignmentProjection.quantity!=1) return false;
        plan.transferKind = HIGGS_TRANSFER_INVENTORY_TO_HAND;
        plan.baseFormId = next->form;
        plan.stackOrdinal = next->ordinal;
        plan.quantity = 1;
        plan.after=snapshot.assignmentProjection;
        return true;
    }
    if (input.edge != HIGGS_INPUT_HELD_RELEASE) return false;
    const auto& incoming = context.items[0];
    const auto& held = snapshot.held[input.physicalHand - 1];
    if (!incoming.known || !incoming.allowed || incoming.source != 1 || !held.reference ||
        incoming.referenceId != held.reference || incoming.baseFormId != held.potion.form ||
        !held.quantity || held.quantity > static_cast<std::uint32_t>((std::numeric_limits<std::int32_t>::max)()) ||
        (snapshot.filter.IsSet() && !snapshot.filter.Matches(held.potion.item))) return false;
    if (!snapshot.filter.IsSet() && !held.assignable) return false;
    plan.transferKind = HIGGS_TRANSFER_HAND_TO_INVENTORY;
    plan.baseFormId = held.potion.form;
    plan.quantity = held.quantity;
    plan.after=snapshot.filter.IsSet()?snapshot.assignmentProjection:held.assignmentProjection;
    return plan.after.identity && plan.after.quantity==1;
}
}
