#ifdef BODYPOUCHES_WEARABLE_DEMO
#include "vr/BagReturnRegion.h"
#include <Windows.h>
#include <algorithm>
#include <cstring>

namespace BodyPouches::VR {
namespace bs = body_slots;
namespace {
constexpr char provider[] = "body-pouches-wearable-return";
constexpr char key[] = "potion-bag-mouth";
bs::RegionsView View() { return {sizeof(bs::RegionsView), bs::kRegionsApiVersion}; }
bs::RegionSampleSource Source(const char* name, std::uint64_t identity,
                             const bs::RegionEvaluationFrame& frame) {
    bs::RegionSampleSource src{};
    std::memcpy(src.provider, provider, sizeof(provider));
    std::memcpy(src.source, name, std::strlen(name) + 1);
    src.identity = identity;
    // Each release is independently sampled. Evidence never represents a continuous dwell.
    src.generation = src.sampleId = src.frame = frame.frame;
    src.epoch = frame.epoch;
    src.saveGeneration = frame.saveGeneration;
    src.timeSeconds = frame.timeSeconds;
    src.units = static_cast<unsigned>(bs::RegionUnits::SkyrimWorld);
    return src;
}
}
void BagReturnRegion::Connect() {
    const auto module = GetModuleHandleW(L"BodySlots.dll");
    if (!module) return;
    const auto base = reinterpret_cast<bs::QueryInterface>(GetProcAddress(module, "BodySlots_QueryInterface"));
    const auto zones = reinterpret_cast<bs::QueryRegionsInterface>(GetProcAddress(module, "BodySlots_QueryRegionsInterface"));
    if (base && zones) Connect(base(bs::kApiVersion, sizeof(bs::Api)), zones(bs::kRegionsApiVersion, sizeof(bs::RegionsApi)));
}
void BagReturnRegion::Connect(const bs::Api* registry, const bs::RegionsApi* regions) {
    if (regions_ || !regions || regions->version != bs::kRegionsApiVersion ||
        regions->structSize < sizeof(bs::RegionsApi) || (regions->capabilities & 3) != 3 ||
        !regions->CopyRegions || !regions->UpsertRegion || !regions->Publish || !regions->CopyObservations) return;
    regions_ = regions;
    if (!registry || registry->version != bs::kApiVersion || registry->structSize < sizeof(bs::Api) ||
        !registry->RegisterClient) return;
    auto rows = std::make_unique<bs::RegionObservation[]>(bs::kMaxRegionObservations);
    if (registry->RegisterClient(provider, &writer_) == bs::Status::Ok)
        rows_ = std::move(rows); // Process-lifetime SKSE client, same lifetime as Mod.
}
BagReturnRegion::Scope BagReturnRegion::CurrentScope() {
    if (!rows_ || !writer_) return {};
    auto view = View();
    const auto status = regions_->CopyRegions(writer_, 0, nullptr, 0, &view);
    if (status != bs::Status::Ok && status != bs::Status::BufferTooSmall) return {};
    // Older builds expose this table but cannot maintain a world scope on stock HIGGS.
    if (view.epoch && view.saveGeneration) seenLiveScope_ = true;
    return {view.epoch, view.saveGeneration};
}
std::uint32_t BagReturnRegion::Inside(Scope scope, std::uint64_t instance, std::uint64_t frame,
    double time, const float (&center)[3], float radius, const bs::RuntimeTransform& bag, const Wrists& wrists) {
    if (!rows_ || !scope.epoch || !scope.save || !frame || !instance) return 0;
    bs::ZoneShape shape{};
    shape.structSize = sizeof(shape);
    shape.kind = static_cast<unsigned>(bs::ZoneShapeKind::Sphere);
    shape.radius = radius;
    std::copy_n(center, 3, shape.center);
    if (!region_.regionId || std::memcmp(&shape, &region_.spec.shape, sizeof(shape))) {
        bs::InteractionRegionSpec spec{};
        spec.structSize = sizeof(spec);
        spec.anchorKind = static_cast<unsigned>(bs::RegionAnchorKind::Player);
        std::memcpy(spec.provider, provider, sizeof(provider));
        std::memcpy(spec.key, key, sizeof(key));
        spec.shape = shape;
        spec.maxSampleAgeSeconds = spec.maxContinuityGapSeconds = .15;
        if (regions_->UpsertRegion(writer_, &spec, region_.revision, &region_) != bs::Status::Ok) return 0;
    }
    const bs::RegionEvaluationFrame sample{sizeof(sample), 0, scope.epoch, scope.save, frame, time};
    bs::RegionEvaluation batch[2]{};
    std::uint32_t count = 0;
    for (unsigned i = 0; i != wrists.size(); ++i) {
        if (!wrists[i]) continue;
        auto& row = batch[count++];
        row.structSize = sizeof(row);
        row.regionId = region_.regionId;
        row.expectedRegionRevision = region_.revision;
        row.anchor.structSize = sizeof(row.anchor);
        row.anchor.kind = region_.spec.anchorKind;
        row.anchor.valid = 1;
        row.anchor.source = Source("attached-bag-art", instance, sample);
        row.anchor.world = bag;
        row.probe.structSize = sizeof(row.probe);
        row.probe.valid = 1;
        row.probe.source = Source("player-wrist", i + 1, sample);
        std::copy(wrists[i]->begin(), wrists[i]->end(), row.probe.world);
    }
    auto view = View();
    if (regions_->Publish(writer_, &sample, batch, count, &view) != bs::Status::Ok) return 0;
    if (regions_->CopyObservations(writer_, view.revision, rows_.get(), bs::kMaxRegionObservations, &view) != bs::Status::Ok ||
        view.count > bs::kMaxRegionObservations) return 0;
    std::uint32_t inside = 0;
    for (unsigned i = 0; i != view.count; ++i) {
        const auto& row = rows_[i];
        // A shared snapshot also contains observations published by other writers.
        if (row.regionId != region_.regionId || row.regionRevision != region_.revision ||
            row.sequence != view.sequence || row.evaluated.frame != frame ||
            row.evaluated.epoch != scope.epoch || row.evaluated.saveGeneration != scope.save ||
            row.state != static_cast<unsigned>(bs::RegionSpatialState::Ready) ||
            row.probe.source.identity < 1 || row.probe.source.identity > 2) continue;
        inside |= 1u << (row.probe.source.identity - 1);
    }
    return inside;
}
}
#endif
