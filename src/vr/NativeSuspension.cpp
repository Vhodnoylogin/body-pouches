#include "vr/NativeSuspension.h"

namespace BodyPouches::VR {
namespace ns = vrik_native_suspension;
namespace {
bool Valid(const ns::Api* api) noexcept {
    return api && api->structSize == sizeof(ns::Api) && api->version == ns::kVersion
        && api->RegisterClient && api->ReleaseClient && api->AcquireClaim
        && api->RenewClaim && api->ReleaseClaim && api->CopySnapshot;
}
bool EmptyOrReleased(ns::Status status) noexcept {
    return status == ns::Status::Ok || status == ns::Status::NotFound || status == ns::Status::Conflict;
}
}
bool NativeSuspension::Attach(const ns::Api* api, bool advertised) noexcept {
    if (_advertised) { return api == _api && EnsureClient(); }
    _advertised = advertised;
    _api = advertised && Valid(api) ? api : nullptr;
    return !_advertised || EnsureClient();
}
bool NativeSuspension::EnsureClient() noexcept {
    if (!_api) { return false; }
    if (_client) { return true; }
    try {
        std::uint64_t candidate{};
        if (_api->RegisterClient(&candidate) != ns::Status::Ok || !candidate) { return false; }
        _client = candidate;
        return true;
    } catch (...) { return false; }
}
bool NativeSuspension::MaintainOne(std::size_t index, const ns::Snapshot& snapshot) {
    auto& current = _claims[index];
    if (!_desired[index]) {
        if (!current.handle) { return true; }
        const auto status = _api->ReleaseClaim(_client, current.handle, current.generation);
        if (!EmptyOrReleased(status)) { return false; }
        current = {};
        return true;
    }
    ns::Claim candidate{};
    candidate.structSize = sizeof(candidate);
    ns::Status status = ns::Status::NotFound;
    if (current.handle && current.generation == snapshot.generation) {
        status = _api->RenewClaim(_client, current.handle, kTtlMillis, snapshot.generation, &candidate);
    }
    if (status == ns::Status::NotFound) {
        status = _api->AcquireClaim(_client, static_cast<std::uint32_t>(index + 1),
            kTtlMillis, snapshot.generation, &candidate);
    }
    if (status != ns::Status::Ok) { return false; }
    if (candidate.structSize != sizeof(candidate) || candidate.slot != index + 1 || !candidate.handle
        || candidate.generation != snapshot.generation || candidate.ttlMillis != kTtlMillis
        || candidate.reserved || candidate.expiresAtMicros <= snapshot.sampledAtMicros) { return false; }
    current = candidate;
    return true;
}
bool NativeSuspension::Set(int slot, bool suspended) noexcept {
    if (!_advertised || slot < 1 || slot > 14) { return false; }
    _desired[static_cast<std::size_t>(slot - 1)] = suspended;
    // The first claim is acquired before returning to the existing setup path.
    return Maintain();
}
bool NativeSuspension::Maintain() noexcept {
    try {
        if (!EnsureClient()) { return false; }
        ns::Snapshot snapshot{};
        snapshot.structSize = sizeof(snapshot); snapshot.version = ns::kVersion;
        if (_api->CopySnapshot(&snapshot) != ns::Status::Ok || !snapshot.generation
            || snapshot.structSize != sizeof(snapshot) || snapshot.version != ns::kVersion) { return false; }
        bool success = true;
        for (std::size_t i = 0; i < _claims.size(); ++i) {
            if (!MaintainOne(i, snapshot)) { success = false; }
        }
        return success;
    } catch (...) { return false; }
}
}
