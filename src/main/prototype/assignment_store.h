// SPDX-License-Identifier: MIT
#pragma once
#include "core/Filter.h"
#include <mutex>
#include <optional>

namespace SKSE { class SerializationInterface; }

namespace BodyPouches::Prototype {
enum class AssignmentLoadResult { Empty, Loaded, Imported, Invalid };
enum class AssignmentApplyResult { Applied, AlreadySet, StaleGeneration, Invalid };
struct AssignmentSnapshot {
    Core::Filter filter;
    std::uint64_t generation{};
};

// Pure preflight before an offer. Its result is captured with the accepted plan;
// completion must not rediscover that an assignment cannot be saved.
[[nodiscard]] std::optional<Core::Filter> MakeAssignment(const Core::Item&);

// The bag's logical filter is independent of Framework placement and activation.
// Runtime applies a captured assignment only after factual successful storage.
class AssignmentStore final {
public:
    [[nodiscard]] Core::Filter Copy() const;
    [[nodiscard]] AssignmentSnapshot Snapshot() const;
    void Clear();
    // Applies only a preflighted, captured logical outcome from this save.
    [[nodiscard]] AssignmentApplyResult ApplyCaptured(const AssignmentSnapshot&);
    // An already factual pending outcome may be serialized without changing live state.
    [[nodiscard]] bool Save(SKSE::SerializationInterface*, const AssignmentSnapshot* factual = nullptr) const;
    [[nodiscard]] AssignmentLoadResult Load(SKSE::SerializationInterface*, int legacySlot = 13);

private:
    mutable std::mutex mutex_;
    Core::Filter filter_;
    std::uint64_t generation_=1;
};
}
