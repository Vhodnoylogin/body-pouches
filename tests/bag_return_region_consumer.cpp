// Link with the production BagReturnRegion and the pinned Body Slots region-test core.
// The two detail calls only establish the save/world usually supplied by the game.
#include "vr/BagReturnRegion.h"
#include "instances.h"
#include "main/interaction_regions.h"
#include <cstdio>

extern "C" const body_slots::Api* BS_CALL BodySlots_QueryInterface(std::uint32_t, std::uint32_t) noexcept;
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "failed line %d: %s\n", __LINE__, #expr); return 1; } } while (false)
int main() {
    using BodyPouches::VR::BagReturnRegion;
    namespace bs = body_slots;
    BagReturnRegion consumer;
    consumer.Connect();
    CHECK(!consumer.Selected()); // no game DLL loaded
    consumer.Connect(BodySlots_QueryInterface(1, sizeof(bs::Api)), &bs::detail::GetRegionsApi());
    CHECK(!consumer.CurrentScope().epoch && !consumer.Selected()); // old table, no live scope
    const auto save = bs::detail::CurrentSaveGeneration();
    bs::detail::SetInteractionRegionsScope(7, save);
    const auto scope = consumer.CurrentScope();
    CHECK(scope.epoch == 7 && scope.save == save && consumer.Selected());
    bs::RuntimeTransform bag{{100, 0, 0}, {0,-1,0, 1,0,0, 0,0,1}, 2};
    const float center[3]{2,0,0}; // sphere centre becomes (100,4,0)
    BagReturnRegion::Wrists hands{BagReturnRegion::Point{100,7,0}, BagReturnRegion::Point{100,9,0}};
    CHECK(consumer.Inside(scope, 23, 1, 1., center, 2, bag, hands) == 1);
    std::swap(hands[0], hands[1]);
    CHECK(consumer.Inside(scope, 23, 2, 1.1, center, 2, bag, hands) == 2);
    CHECK(consumer.Inside(scope, 23, 3, 1.2, center, 3, bag, hands) == 3);
    hands[1].reset();
    CHECK(consumer.Inside(scope, 23, 4, 1.3, center, 3, bag, hands) == 1);
    CHECK(!consumer.Inside(scope, 23, 4, 1.3, center, 3, bag, hands));
    bs::detail::SetInteractionRegionsScope(8, save);
    CHECK(!consumer.Inside(scope, 23, 5, 1.4, center, 3, bag, hands));
    bag.position[0] += 100;
    CHECK(!consumer.Inside(consumer.CurrentScope(), 23, 5, 1.4, center, 3, bag, hands));
    bs::detail::InvalidateInteractionRegions();
    CHECK(!consumer.CurrentScope().epoch && consumer.Selected()); // no fallback after a real scope
    return 0;
}
