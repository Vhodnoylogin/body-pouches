#pragma once
#include "vriknativesuspension_api.h"
#include <array>

namespace BodyPouches::VR {
// Game-thread owner of this consumer's desired claims. Negotiated failures never
// fall back to an unowned legacy write. No callbacks or engine pointers are stored.
class NativeSuspension {
public:
    bool Attach(const vrik_native_suspension::Api*, bool advertised) noexcept;
    [[nodiscard]] bool Advertised() const noexcept { return _advertised; }
    bool Set(int slot, bool suspended) noexcept;
    bool Maintain() noexcept;
private:
    static constexpr std::uint32_t kTtlMillis = 2000;
    bool EnsureClient() noexcept;
    bool MaintainOne(std::size_t index, const vrik_native_suspension::Snapshot&);
    const vrik_native_suspension::Api* _api{};
    bool _advertised{};
    std::uint64_t _client{};
    std::array<bool, 14> _desired{};
    std::array<vrik_native_suspension::Claim, 14> _claims{};
};
}
