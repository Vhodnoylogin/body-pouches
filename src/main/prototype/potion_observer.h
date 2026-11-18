// SPDX-License-Identifier: MIT
#pragma once
#include "potion_policy.h"
#include <RE/B/BSTEvent.h>
#include <RE/T/TESContainerChangedEvent.h>
#include <atomic>

namespace BodyPouches::Prototype {
// Engine observation boundary. The owner keeps this object alive for the plugin
// lifetime after ConnectEvents; only Invalidate and the event sink run off-thread.
// All cache reads/updates are serialized by the owner's safe main-phase callback.
class PotionObserver final : private RE::BSTEventSink<RE::TESContainerChangedEvent> {
public:
    PotionObserver() = default;
    PotionObserver(const PotionObserver&) = delete;
    PotionObserver& operator=(const PotionObserver&) = delete;

    [[nodiscard]] bool ConnectEvents();
    void Invalidate() noexcept;
    [[nodiscard]] bool StockDue(std::uint64_t nowNs) const noexcept;
    // A concurrent container event leaves the cache dirty and suppresses this
    // publication. The next safe phase retries instead of publishing stale stock.
    [[nodiscard]] std::shared_ptr<const std::vector<Potion>> CopyStock(
        std::uint64_t nowNs, bool force = false);
    [[nodiscard]] std::array<HeldPotion, 2> CopyHeld(
        const std::array<HIGGS_ItemIdentity, 2>&, std::uint64_t epoch, std::uint64_t nowNs);

private:
    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent*,
        RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;

    std::atomic<bool> stockDirty_{true}, heldDirty_{true};
    bool eventsConnected_{};
    std::uint64_t scannedAtNs_{}, heldScannedAtNs_{}, heldEpoch_{};
    std::shared_ptr<const std::vector<Potion>> stock_;
    std::array<HIGGS_ItemIdentity, 2> heldIdentity_{};
    std::array<HeldPotion, 2> held_{};
};
}
