// SPDX-License-Identifier: MIT
#pragma once
#include "core/Filter.h"
#include <mutex>

namespace SKSE { class SerializationInterface; }

namespace BodyPouches::Prototype {
enum class AssignmentLoadResult { Empty, Loaded, Imported, Invalid };

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
