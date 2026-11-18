// SPDX-License-Identifier: MIT
#pragma once
#include "core/Filter.h"
#include <mutex>
#include <optional>

namespace SKSE { class SerializationInterface; }

namespace BodyPouches::Prototype {
enum class AssignmentLoadResult { Empty, Loaded, Imported, Invalid };

// Pure preflight shared by offer policy and receipt application. A successful
// store must not discover only afterwards that its assignment cannot be saved.
[[nodiscard]] std::optional<Core::Filter> MakeAssignment(const Core::Item&);

// The bag's logical filter is independent of Framework placement and activation.
// Runtime calls AssignIfUnset only after a factual, successful store receipt.
class AssignmentStore final {
public:
    [[nodiscard]] Core::Filter Copy() const;
    void Clear();
    [[nodiscard]] bool AssignIfUnset(const Core::Item&);
    [[nodiscard]] bool Save(SKSE::SerializationInterface*) const;
    [[nodiscard]] AssignmentLoadResult Load(SKSE::SerializationInterface*, int legacySlot = 13);

private:
    mutable std::mutex mutex_;
    Core::Filter filter_;
};
}
