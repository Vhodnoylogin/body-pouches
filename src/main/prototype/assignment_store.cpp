// SPDX-License-Identifier: MIT
#include "assignment_store.h"
#include "core/Assignments.h"
#include "game/Forms.h"
#include <SKSE/SKSE.h>
#include <algorithm>
#include <utility>

namespace BodyPouches::Prototype {
namespace {
constexpr std::uint32_t kCurrentRecord = 0x42503241; // BP2A under the existing BPCH plugin identity.
constexpr std::uint32_t kCurrentVersion = 1;
constexpr int kCurrentIndex = 1; // Codec index, not a VRIK slot.

struct Candidate {
    bool seen{}, invalid{};
    Core::Assignments assignments;
};

void Read(Candidate& candidate, SKSE::SerializationInterface& skse,
    std::uint32_t version, std::uint32_t length, bool current) {
    if (candidate.seen || version != kCurrentVersion || length > Core::kAssignmentBytes) {
        candidate.seen = true;
        candidate.invalid = true;
        return;
    }
    candidate.seen = true;
    std::vector<std::uint8_t> bytes(length);
    if (skse.ReadRecordData(bytes.data(), length) != length) {
        candidate.invalid = true;
        return;
    }
    auto decoded = Core::DecodeAssignments(bytes);
    if (!decoded || (current && (decoded->size() > 1 ||
        (!decoded->empty() && decoded->front().slot != kCurrentIndex)))) {
        candidate.invalid = true;
        return;
    }
    candidate.assignments = std::move(*decoded);
}

std::optional<Core::FormKey> Resolve(SKSE::SerializationInterface& skse,
    const Core::FormKey& key, bool effect) {
    auto current = key;
    if (key.plugin.empty() && (!skse.ResolveFormID(key.localId, current.localId) || !current.localId))
        return std::nullopt;
    auto* form = Game::Lookup(current);
    if (!form) return std::nullopt;
    if (effect) {
        if (!form->As<RE::EffectSetting>()) return std::nullopt;
    } else {
        auto* potion = form->As<RE::AlchemyItem>();
        if (!potion || !potion->GetPlayable() || potion->IsFood() || potion->IsPoison())
            return std::nullopt;
    }
    // Normalize the loaded identity; plugin/local IDs do not depend on load order.
    return Game::KeyOf(form);
}
}

std::optional<Core::Filter> MakeAssignment(const Core::Item& item) {
    if (item.harmful || !item.key.IsSet()) return std::nullopt;
    auto filter = Core::Filter::FromItem(item);
    if (!Core::EncodeAssignments({{kCurrentIndex, filter}})) return std::nullopt;
    return filter;
}

Core::Filter AssignmentStore::Copy() const {
    std::lock_guard lock(mutex_);
    return filter_;
}

void AssignmentStore::Clear() {
    std::lock_guard lock(mutex_);
    filter_ = {};
}

bool AssignmentStore::AssignIfUnset(const Core::Item& item) {
    auto next = MakeAssignment(item);
    if (!next) return false;
    std::lock_guard lock(mutex_);
    if (filter_.IsSet()) return false;
    filter_ = std::move(*next);
    return true;
}

bool AssignmentStore::Save(SKSE::SerializationInterface* skse) const {
    const auto filter = Copy();
    const auto bytes = Core::EncodeAssignments(filter.IsSet()
        ? Core::Assignments{{kCurrentIndex, filter}} : Core::Assignments{});
    // Always write the empty current record too: it explicitly overrides legacy ASGN.
    return skse && bytes && skse->WriteRecord(kCurrentRecord, kCurrentVersion,
        bytes->data(), static_cast<std::uint32_t>(bytes->size()));
}

AssignmentLoadResult AssignmentStore::Load(SKSE::SerializationInterface* skse, int legacySlot) {
    Clear();
    if (!skse) return AssignmentLoadResult::Invalid;
    Candidate current, legacy;
    std::uint32_t type{}, version{}, length{};
    while (skse->GetNextRecordInfo(type, version, length)) {
        if (type == kCurrentRecord) Read(current, *skse, version, length, true);
        else if (type == Core::kAssignmentRecord) Read(legacy, *skse, version, length, false);
    }

    // Only decide after consuming the stream: a later duplicate invalidates an
    // earlier valid record. A present BP2A never falls back to ASGN, even if empty.
    const auto& selected = current.seen ? current : legacy;
    if (!current.seen && legacySlot == 0) return AssignmentLoadResult::Empty;
    if (!selected.seen) return AssignmentLoadResult::Empty;
    if (selected.invalid) return AssignmentLoadResult::Invalid;
    const int index = current.seen ? kCurrentIndex : legacySlot;
    if (index < 1 || index > 14) return AssignmentLoadResult::Invalid;
    const auto found = std::find_if(selected.assignments.begin(), selected.assignments.end(),
        [index](const auto& assignment) { return assignment.slot == index; });
    if (found == selected.assignments.end()) return AssignmentLoadResult::Empty;

    const auto resolved = Core::ResolveAssignments({*found},
        [skse](const Core::FormKey& key, bool effect) { return Resolve(*skse, key, effect); });
    if (resolved.size() != 1) return AssignmentLoadResult::Invalid;
    {
        std::lock_guard lock(mutex_);
        filter_ = resolved.front().filter;
    }
    return current.seen ? AssignmentLoadResult::Loaded : AssignmentLoadResult::Imported;
}
}
