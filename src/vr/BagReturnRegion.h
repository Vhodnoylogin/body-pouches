#pragma once
#ifdef BODYPOUCHES_WEARABLE_DEMO
#include "body_slots_regions_api.h"
#include <array>
#include <memory>
#include <optional>

namespace BodyPouches::VR {
// One event-time publication for both hands. No input hooks or inventory operations.
class BagReturnRegion {
public:
    struct Scope {
        std::uint64_t epoch{}, save{};
        bool operator==(const Scope&) const = default;
    };
    using Point = std::array<float, 3>;
    using Wrists = std::array<std::optional<Point>, 2>; // physical left, right
    void Connect();
    void Connect(const body_slots::Api* registry, const body_slots::RegionsApi* regions);
    [[nodiscard]] bool Selected() const { return seenLiveScope_; }
    [[nodiscard]] Scope CurrentScope();
    [[nodiscard]] std::uint32_t Inside(Scope scope, std::uint64_t instance, std::uint64_t frame,
        double time, const float (&center)[3], float radius, const body_slots::RuntimeTransform& bag,
        const Wrists& wrists);
private:
    const body_slots::RegionsApi* regions_{};
    std::uint64_t writer_{};
    bool seenLiveScope_{};
    body_slots::InteractionRegionRecord region_{};
    std::unique_ptr<body_slots::RegionObservation[]> rows_;
};
}
#endif
