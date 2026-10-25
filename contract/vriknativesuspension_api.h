#pragma once
// Additive copied ABI; independent of Body Slots and official VRIK interface001.
#include <cstdint>
#include <type_traits>
#if defined(_WIN32)
#define VNS_CALL __cdecl
#else
#define VNS_CALL
#endif
namespace vrik_native_suspension {
constexpr std::uint32_t kVersion = 1, kSlots = 14, kMaxClients = 64, kMaxClaims = 128;
constexpr std::uint32_t kMinTtlMillis = 1, kMaxTtlMillis = 5000;
enum class Status : std::uint32_t {
    Ok = 0, InvalidArgument = 1, UnsupportedVersion = 2, NotFound = 3,
    Conflict = 4, Unauthorized = 6, LimitExceeded = 7, Unavailable = 8, InternalError = 9
};
struct Claim {
    std::uint32_t structSize;
    std::uint32_t slot; // native slot 1..14
    std::uint64_t handle;
    std::uint64_t generation;
    std::uint64_t expiresAtMicros; // producer steady-clock epoch, not wall time
    std::uint32_t ttlMillis;
    std::uint32_t reserved;
};
struct Snapshot {
    std::uint32_t structSize;
    std::uint32_t version;
    std::uint64_t generation;
    std::uint64_t mutationRevision; // expiry can change counts without a mutation
    std::uint64_t sampledAtMicros;
    std::uint32_t activeClaimCount;
    std::uint32_t occupiedSlotMask; // bit0 is native slot1; scoped claims only
    std::uint32_t activeClaimsBySlot[kSlots];
};
struct Api {
    std::uint32_t structSize;
    std::uint32_t version;
    // Process-lifetime clients; no callbacks, code/context pointers or namespace ownership.
    Status (VNS_CALL *RegisterClient)(std::uint64_t* client);
    Status (VNS_CALL *ReleaseClient)(std::uint64_t client);
    // Exact nonzero current generation. Handles are opaque and never reused.
    // Initialize Claim.structSize; all failures leave output untouched.
    Status (VNS_CALL *AcquireClaim)(std::uint64_t client, std::uint32_t slot,
        std::uint32_t ttlMillis, std::uint64_t generation, Claim* committed);
    Status (VNS_CALL *RenewClaim)(std::uint64_t client, std::uint64_t handle,
        std::uint32_t ttlMillis, std::uint64_t generation, Claim* committed);
    Status (VNS_CALL *ReleaseClaim)(std::uint64_t client, std::uint64_t handle, std::uint64_t generation);
    // Read-only public copy. Initialize exact structSize/version. Failure leaves it untouched.
    Status (VNS_CALL *CopySnapshot)(Snapshot*);
};
static_assert(sizeof(Claim) == 40, "Native suspension claim ABI changed");
static_assert(sizeof(Snapshot) == 96, "Native suspension snapshot ABI changed");
static_assert(sizeof(Api) == 56, "Native suspension table ABI changed (x64)");
static_assert(std::is_trivially_copyable<Snapshot>::value, "Copied ABI only");
using QueryInterface = const Api* (VNS_CALL *)(std::uint32_t version, std::uint32_t callerSize);
} // namespace vrik_native_suspension
// Export: VRIK_NativeSuspension_QueryInterface(1, sizeof(Api)). VRIK must stay loaded.
// A claim suppresses native holster actions and enables existing suspended-slot
// recognition. It does not reserve input, contents, inventory, a hand or a visual.
