// SPDX-License-Identifier: MIT
#include "potion_observer.h"
#include "assignment_store.h"
#include "game/Forms.h"
#include <RE/Skyrim.h>
#include <cstring>
#include <utility>

namespace BodyPouches::Prototype {
namespace {
constexpr std::uint32_t kPlayerReference = 0x14;
constexpr std::uint64_t kFallbackScanNs = 1000000000;

bool Drinkable(const RE::AlchemyItem* potion) {
    return potion && potion->GetPlayable() && !potion->IsFood() && !potion->IsPoison();
}

Potion Describe(RE::AlchemyItem* potion) {
    Potion result;
    result.item = Game::Describe(potion);
    result.form = potion->GetFormID();
    const auto* model = potion->GetModel();
    if (model && std::strlen(model) < 192) result.model = model;
    return result;
}

bool SameIdentity(const HIGGS_ItemIdentity& a, const HIGGS_ItemIdentity& b) noexcept {
    return a.structSize == b.structSize && a.kind == b.kind && a.epoch == b.epoch &&
        a.instance == b.instance && a.generation == b.generation && a.provider == b.provider &&
        a.providerKey == b.providerKey && a.instanceDataVersion == b.instanceDataVersion &&
        a.validFields == b.validFields && a.baseFormId == b.baseFormId &&
        a.worldRefId == b.worldRefId && a.quantity == b.quantity;
}

std::shared_ptr<const std::vector<Potion>> ReadStock(RE::PlayerCharacter& player) {
    auto stock = std::make_shared<std::vector<Potion>>();
    for (auto& [object, entry] : player.GetInventory([](RE::TESBoundObject& o) {
        return o.Is(RE::FormType::AlchemyItem);
    })) {
        auto* potion = object ? object->As<RE::AlchemyItem>() : nullptr;
        if (entry.first <= 0 || !Drinkable(potion)) continue;
        auto candidate = Describe(potion);
        candidate.item.count = entry.first;
        std::int64_t plain = entry.first;
        std::uint32_t ordinal = 0, firstExtended = 0;
        if (entry.second && entry.second->extraLists) {
            for (const auto* extra : *entry.second->extraLists) {
                if (!extra) continue;
                if (++ordinal > 4096) { plain = -1; break; }
                const auto count = extra->GetCount();
                if (count <= 0) { plain = -1; break; }
                plain -= count;
                if (!firstExtended) firstExtended = ordinal;
            }
        }
        if (plain < 0 || (!plain && !firstExtended)) continue;
        candidate.ordinal = plain > 0 ? 0 : firstExtended;
        stock->push_back(std::move(candidate));
    }
    return stock;
}
}

bool PotionObserver::ConnectEvents() {
    if (eventsConnected_) return true;
    auto* events = RE::ScriptEventSourceHolder::GetSingleton();
    if (!events) return false;
    events->AddEventSink<RE::TESContainerChangedEvent>(this);
    eventsConnected_ = true;
    return true;
}

RE::BSEventNotifyControl PotionObserver::ProcessEvent(const RE::TESContainerChangedEvent* event,
    RE::BSTEventSource<RE::TESContainerChangedEvent>*) {
    if (event && (event->oldContainer == kPlayerReference || event->newContainer == kPlayerReference))
        stockDirty_.store(true);
    return RE::BSEventNotifyControl::kContinue;
}

void PotionObserver::Invalidate() noexcept {
    stockDirty_.store(true);
    heldDirty_.store(true);
}

bool PotionObserver::StockDue(std::uint64_t nowNs) const noexcept {
    return !stock_ || stockDirty_.load() || nowNs < scannedAtNs_ ||
        nowNs - scannedAtNs_ >= kFallbackScanNs;
}

std::shared_ptr<const std::vector<Potion>> PotionObserver::CopyStock(std::uint64_t nowNs, bool force) {
    if (!force && !StockDue(nowNs)) return stock_;
    // Clear before reading. An event arriving during enumeration must survive.
    stockDirty_.exchange(false);
    try {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) { stockDirty_.store(true); return nullptr; }
        auto next = ReadStock(*player);
        if (stockDirty_.load()) return nullptr;
        stock_ = std::move(next);
        scannedAtNs_ = nowNs;
        return stock_;
    } catch (...) {
        stockDirty_.store(true);
        throw;
    }
}

std::array<HeldPotion, 2> PotionObserver::CopyHeld(
    const std::array<HIGGS_ItemIdentity, 2>& identities, std::uint64_t epoch, std::uint64_t nowNs) {
    // Preserve the old bounded metadata refresh for mods that edit potion forms
    // in place without changing the held reference's public HIGGS identity.
    const bool refresh = heldDirty_.exchange(false) || heldEpoch_ != epoch || !heldScannedAtNs_ ||
        nowNs < heldScannedAtNs_ || nowNs - heldScannedAtNs_ >= 100000000;
    if (refresh) heldScannedAtNs_ = nowNs;
    heldEpoch_ = epoch;
    constexpr auto required = HIGGS_ITEM_BASE_KNOWN | HIGGS_ITEM_REFERENCE_KNOWN |
        HIGGS_ITEM_QUANTITY_KNOWN | HIGGS_ITEM_NATIVE_OBSERVED;
    for (std::size_t hand = 0; hand < held_.size(); ++hand) {
        const auto& item = identities[hand];
        if (!refresh && SameIdentity(item, heldIdentity_[hand])) continue;
        heldIdentity_[hand] = item;
        held_[hand] = {};
        if (item.structSize != sizeof(item) || item.epoch != epoch ||
            item.kind != HIGGS_ITEM_WORLD_REFERENCE || !item.worldRefId || !item.quantity ||
            (item.validFields & required) != required) continue;
        auto* potion = RE::TESForm::LookupByID<RE::AlchemyItem>(item.baseFormId);
        if (Drinkable(potion)) {
            auto described = Describe(potion);
            auto assignment = MakeAssignment(described.item);
            const bool assignable = assignment.has_value();
            held_[hand] = {std::move(described), item.worldRefId, item.quantity, assignable, std::move(assignment)};
        }
    }
    return held_;
}
}
