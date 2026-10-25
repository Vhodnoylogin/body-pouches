// Real consumer class + actual isolated VRIK service DLL. Never loads VRIK.dll.
#include "vr/NativeSuspension.h"
#include <Windows.h>
#include <chrono>
#include <iostream>
#include <thread>

namespace ns = vrik_native_suspension;
namespace {
const ns::Api* real{};
std::uint64_t lastHandle{};
bool refuseRenew{};
int checks{}, failures{};
void Check(bool value, const char* text) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL " << text << '\n'; }
}
#define CHECK_REAL(x) Check(bool(x), #x)
ns::Status VNS_CALL acquire(std::uint64_t client, std::uint32_t slot, std::uint32_t ttl,
    std::uint64_t generation, ns::Claim* output) {
    const auto result = real->AcquireClaim(client, slot, ttl, generation, output);
    if (result == ns::Status::Ok) { lastHandle = output->handle; }
    return result;
}
ns::Status VNS_CALL renew(std::uint64_t client, std::uint64_t handle, std::uint32_t ttl,
    std::uint64_t generation, ns::Claim* output) {
    return refuseRenew ? ns::Status::InternalError : real->RenewClaim(client, handle, ttl, generation, output);
}
ns::Snapshot Snapshot() {
    ns::Snapshot result{};
    result.structSize = sizeof(result); result.version = ns::kVersion;
    CHECK_REAL(real->CopySnapshot(&result) == ns::Status::Ok);
    return result;
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) { std::cerr << "Expected isolated NativeSuspensionFixture.dll path\n"; return 2; }
    // Prevent accidentally passing the full engine-dependent plugin.
    const std::wstring path = argv[1];
    const auto slash = path.find_last_of(L"/\\");
    if (path.substr(slash == std::wstring::npos ? 0 : slash + 1) != L"NativeSuspensionFixture.dll") { return 2; }
    const auto module = LoadLibraryW(argv[1]);
    if (!module) { return 3; }
    const auto query = reinterpret_cast<ns::QueryInterface>(GetProcAddress(module, "VRIK_NativeSuspension_QueryInterface"));
    real = query ? query(ns::kVersion, sizeof(ns::Api)) : nullptr;
    if (!real) { FreeLibrary(module); return 4; }
    ns::Api proxy = *real;
    proxy.AcquireClaim = acquire; proxy.RenewClaim = renew;
    BodyPouches::VR::NativeSuspension first, second;
    CHECK_REAL(first.Attach(&proxy, true));
    CHECK_REAL(second.Attach(real, true));
    CHECK_REAL(first.Set(13, true));
    const auto initialHandle = lastHandle;
    CHECK_REAL(second.Set(13, true));
    auto snapshot = Snapshot();
    CHECK_REAL(snapshot.activeClaimCount == 2);
    CHECK_REAL(snapshot.activeClaimsBySlot[12] == 2);
    CHECK_REAL(first.Maintain());
    CHECK_REAL(lastHandle == initialHandle);
    CHECK_REAL(first.Set(13, false));
    snapshot = Snapshot();
    CHECK_REAL(snapshot.activeClaimCount == 1 && snapshot.activeClaimsBySlot[12] == 1);
    CHECK_REAL(first.Maintain());
    CHECK_REAL(Snapshot().activeClaimCount == 1);
    CHECK_REAL(second.Set(13, false));
    CHECK_REAL(Snapshot().activeClaimCount == 0);
    CHECK_REAL(first.Set(12, true));
    const auto expiringHandle = lastHandle;
    refuseRenew = true;
    CHECK_REAL(!first.Maintain());
    CHECK_REAL(Snapshot().activeClaimCount == 1);
    refuseRenew = false;
    CHECK_REAL(first.Maintain());
    CHECK_REAL(lastHandle == expiringHandle);
    // Caller-selected TTL is2000ms. This is real producer-clock expiry, not a fake reset export.
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    CHECK_REAL(Snapshot().activeClaimCount == 0);
    CHECK_REAL(first.Maintain());
    CHECK_REAL(lastHandle != expiringHandle);
    CHECK_REAL(Snapshot().activeClaimsBySlot[11] == 1);
    CHECK_REAL(first.Set(12, false));
    CHECK_REAL(Snapshot().activeClaimCount == 0);
    CHECK_REAL(!first.Set(0, true) && !first.Set(15, true));
    FreeLibrary(module);
    std::cout << checks << " checks, " << failures << " failures; isolated service only, no game\n";
    return failures ? 1 : 0;
}
