#pragma once

#include "body_slots_instance_activation_api.h"
#include "body_slots_instance_configuration_api.h"
#include <memory>

namespace BodyPouches::VR {
// Private single-bag/native13 demo. One main-thread owner; no equipment/engine
// lookup, rendering, inventory calls, public ABI or supported provider hot unload.
class BodySlotsDemo final {
public:
    static constexpr const char* kEquipmentProvider = "body-pouches-demo-equipment";
    static constexpr const char* kTemplateProvider = "body-pouches-demo-template";
    static constexpr const char* kTemplateKey = "potion-bag";
    static constexpr const char* kEquipmentKey = "single-demo-bag";
    static constexpr const char* kSlotKey = "potion-slot";

    enum class Reason {
        Disconnected, Ready, EquipmentInvalidated, EquipmentUnobserved, Unworn,
        SourceUnavailable, BadInterface, BadSnapshot, TemplateMissing,
        TemplateChanged, InstanceMissing, ScopeChanged, InstanceChanged,
        StaleToken, Ineligible, Reset, Exhausted
    };
    struct Token {
        std::uint64_t equipmentEpoch{}, saveGeneration{}, viewRevision{};
        std::uint64_t instanceHandle{}, instanceRevision{};
        [[nodiscard]] bool Valid() const noexcept;
        friend bool operator==(const Token& a, const Token& b) noexcept;
        friend bool operator!=(const Token& a, const Token& b) noexcept { return !(a == b); }
    };
    struct CopiedState {
        bool connected{}, observed{}, worn{}, available{}, requestedActive{}, effectiveEnabled{}, exhausted{};
        std::uint64_t equipmentEpoch = 1;
        Token token{};
        body_slots::InstanceView view{};
        std::uint64_t pinnedTemplateRevision{}, currentTemplateRevision{};
        std::uint32_t templateState = static_cast<std::uint32_t>(body_slots::InstanceTemplateState::Missing);
        body_slots::Status status = body_slots::Status::Unavailable;
        Reason reason = Reason::Disconnected;
    };

    BodySlotsDemo();
    ~BodySlotsDemo();
    BodySlotsDemo(const BodySlotsDemo&) = delete;
    BodySlotsDemo& operator=(const BodySlotsDemo&) = delete;
    bool Connect() noexcept;
    // True means the observed request was reconciled, including known unworn.
    // It is not admission permission. Unknown equipment always fails closed.
    bool Reconcile(bool observed, bool worn) noexcept;
    void InvalidateEquipment() noexcept; // Always advances epoch; no provider calls.
    void Reset() noexcept; // Local fence; provider resets/reconciliation retire runtime state.
    [[nodiscard]] CopiedState Copy() const noexcept;
    [[nodiscard]] Token CurrentToken() const noexcept;
    // Fresh configured copy + exact scope/view eligibility; never cached permission.
    bool Allows(bool isLeft, const Token* captured = nullptr) noexcept;
    [[nodiscard]] static const char* ReasonName(Reason) noexcept;

private:
    struct Owner;
    struct Buffers;
    std::unique_ptr<Owner> owner_;
    std::unique_ptr<Buffers> buffers_;
    void Commit(const Owner&) noexcept; // The only authoritative state publication.
    bool Publish(const Owner&, std::uint64_t expectedEpoch) noexcept;
    bool ReadConfigured(Owner&) noexcept;
    bool CreateInactive(Owner&) noexcept;
    bool SetActive(Owner&, bool active) noexcept;
    static bool Advance(Owner&) noexcept;
    static void Describe(Owner&) noexcept;
    static void Fail(Owner&, body_slots::Status, Reason) noexcept;
};
}
