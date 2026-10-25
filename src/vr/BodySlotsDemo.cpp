#include "vr/BodySlotsDemo.h"

#include <Windows.h>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace BodyPouches::VR {
namespace bs = body_slots;
namespace {
using Demo = BodySlotsDemo;
bool text(const char* value, std::size_t size, bool identity = true) noexcept {
    bool ended = false;
    if (!value[0]) return false;
    for (std::size_t i = 0; i < size; ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (ended) { if (c) return false; continue; }
        if (!c) { ended = true; continue; }
        if (c < 32 || c > 126) return false;
        if (identity && !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == '/')) return false;
    }
    return ended;
}
bool definition(const bs::Definition& d) noexcept {
    if (d.structSize != sizeof(d) || (d.flags & ~bs::kEnabled) || d.reserved ||
        !text(d.provider, 64) || !text(d.key, 64) || !text(d.anchor, 96, false) ||
        !text(d.referenceRig, 64) || !text(d.kind, 32) ||
        std::strcmp(d.referenceRig, "skyrimvr-humanoid-v1") ||
        !d.handMask || (d.handMask & ~bs::Both) || !d.contentMask ||
        (d.contentMask & ~bs::AllContent) || d.scaleMode != bs::AnchorScale) return false;
    for (float v : d.position) if (!std::isfinite(v) || std::abs(v) > 1000000.f) return false;
    for (float v : d.regionCenter) if (!std::isfinite(v) || std::abs(v) > 1000000.f) return false;
    if (!std::isfinite(d.radius) || d.radius <= 0 || d.radius > 1000000.f) return false;
    for (float v : d.rotation) if (!std::isfinite(v)) return false;
    for (unsigned i = 0; i < 3; ++i) for (unsigned j = i; j < 3; ++j) {
        double dot = 0;
        for (unsigned k = 0; k < 3; ++k) dot += double(d.rotation[i * 3 + k]) * d.rotation[j * 3 + k];
        if (std::abs(dot - (i == j ? 1. : 0.)) > 0.0001) return false;
    }
    const auto* r = d.rotation;
    const double det = double(r[0]) * (double(r[4]) * r[8] - double(r[5]) * r[7]) -
        double(r[1]) * (double(r[3]) * r[8] - double(r[5]) * r[6]) +
        double(r[2]) * (double(r[3]) * r[7] - double(r[4]) * r[6]);
    return std::abs(det - 1.) <= 0.0001;
}
bool view(const bs::InstanceView& v) noexcept {
    return v.structSize == sizeof(v) && v.version == bs::kInstanceApiVersion && !v.reserved &&
        v.saveGeneration && v.viewRevision && v.registryRevision && v.configurationRevision && v.count <= bs::kMaxInstances;
}
bool sameKey(const bs::InstanceKey& a, const bs::InstanceKey& b) noexcept {
    return !std::strcmp(a.ownerProvider, b.ownerProvider) && !std::strcmp(a.equipmentKey, b.equipmentKey) &&
        !std::strcmp(a.slotKey, b.slotKey);
}
bool target(const bs::InstanceKey& key) noexcept {
    return !std::strcmp(key.ownerProvider, Demo::kEquipmentProvider) &&
        !std::strcmp(key.equipmentKey, Demo::kEquipmentKey) && !std::strcmp(key.slotKey, Demo::kSlotKey);
}
bool record(const bs::ConfiguredInstanceRecord& row) noexcept {
    const auto& r = row.instance;
    if (r.structSize != sizeof(r) || (r.flags & ~bs::kInstanceActive) || r.reserved || row.reserved ||
        (r.customizationMask & ~bs::AllCustomizationFields) || (row.instanceCustomizationMask & ~bs::AllCustomizationFields) ||
        !text(r.key.ownerProvider, 64) || !text(r.key.equipmentKey, 64) || !text(r.key.slotKey, 64) ||
        !text(r.definitionProvider, 64) || !text(r.definitionKey, 64) || !r.instanceHandle || !r.instanceRevision ||
        !r.pinnedDefinitionRevision || r.binding != static_cast<unsigned>(bs::InstanceBinding::BodyAnchor)) return false;
    if (r.templateState == static_cast<unsigned>(bs::InstanceTemplateState::Current)) {
        return r.currentDefinitionRevision == r.pinnedDefinitionRevision && definition(r.effective) &&
            !std::strcmp(r.definitionProvider, r.effective.provider) && !std::strcmp(r.definitionKey, r.effective.key);
    }
    if (r.templateState != static_cast<unsigned>(bs::InstanceTemplateState::Missing) &&
        r.templateState != static_cast<unsigned>(bs::InstanceTemplateState::Changed)) return false;
    const bs::Definition zero{};
    return !r.customizationMask && !row.instanceCustomizationMask && !std::memcmp(&r.effective, &zero, sizeof(zero)) &&
        (r.templateState == static_cast<unsigned>(bs::InstanceTemplateState::Missing) ?
            r.currentDefinitionRevision == 0 : r.currentDefinitionRevision && r.currentDefinitionRevision != r.pinnedDefinitionRevision);
}
bool valid(const bs::Api* a) noexcept {
    const auto caps = bs::kRegistryCapability | bs::kSnapshotCapability | bs::kContentFilterCapability;
    return a && a->structSize == sizeof(*a) && a->version == bs::kApiVersion && (a->capabilities & caps) == caps &&
        a->RegisterClient && a->ReleaseClient && a->UpsertDefinition && a->RemoveDefinition && a->CopyDefinitions &&
        a->EvaluateEligibility && a->ReplaceDefinitions && a->RegisterObserver;
}
bool valid(const bs::InstanceApi* a) noexcept {
    const auto caps = bs::kRuntimeInstancesCapability | bs::kBodyAnchorInstancesCapability;
    return a && a->structSize == sizeof(*a) && a->version == bs::kInstanceApiVersion && (a->capabilities & caps) == caps &&
        a->UpsertInstance && a->RemoveInstance && a->CopyEffectiveInstances && a->EvaluateInstanceEligibility;
}
bool valid(const bs::InstanceConfigurationApi* a) noexcept {
    return a && a->structSize == sizeof(*a) && a->version == bs::kInstanceConfigurationApiVersion &&
        (a->capabilities & bs::kConfiguredInstancesCapability) && a->ApplyCustomization && a->ClearCustomization &&
        a->CopyConfiguredInstances && a->CopyCustomizations && a->EvaluateConfiguredEligibility;
}
bool valid(const bs::InstanceActivationApi* a) noexcept {
    return a && a->structSize == sizeof(*a) && a->version == bs::kInstanceActivationApiVersion &&
        (a->capabilities & bs::kAtomicInstanceActivationCapability) && a->ApplyActivationChanges;
}
}

struct BodySlotsDemo::Owner {
    CopiedState copy;
    bs::Api base{};
    bs::InstanceApi instances{};
    bs::InstanceConfigurationApi configured{};
    bs::InstanceActivationApi activation{};
    std::uint64_t writer{};
    unsigned registrations{}; // At most initial registration plus one lost-client recovery.
    bs::ConfiguredInstanceRecord row{};
};
struct BodySlotsDemo::Buffers {
    std::array<bs::SlotRecord, bs::kMaxDefinitions> definitions{};
    std::array<bs::ConfiguredInstanceRecord, bs::kMaxInstances> instances{};
};
BodySlotsDemo::BodySlotsDemo() : owner_(std::make_unique<Owner>()) {}
BodySlotsDemo::~BodySlotsDemo() = default; // Provider stays loaded; no teardown-order DLL calls.
bool BodySlotsDemo::Token::Valid() const noexcept {
    return equipmentEpoch && saveGeneration && viewRevision && instanceHandle && instanceRevision;
}
bool operator==(const BodySlotsDemo::Token& a, const BodySlotsDemo::Token& b) noexcept {
    return a.equipmentEpoch == b.equipmentEpoch && a.saveGeneration == b.saveGeneration &&
        a.viewRevision == b.viewRevision && a.instanceHandle == b.instanceHandle && a.instanceRevision == b.instanceRevision;
}
void BodySlotsDemo::Commit(const Owner& candidate) noexcept { *owner_ = candidate; }
bool BodySlotsDemo::Publish(const Owner& candidate, std::uint64_t expectedEpoch) noexcept {
    if (owner_->copy.equipmentEpoch != expectedEpoch || owner_->copy.exhausted) return false;
    Commit(candidate); return true;
}
bool BodySlotsDemo::Advance(Owner& candidate) noexcept {
    candidate.copy.available = false; candidate.copy.token = {};
    if (candidate.copy.exhausted || candidate.copy.equipmentEpoch == (std::numeric_limits<std::uint64_t>::max)()) {
        candidate.copy.exhausted = true; candidate.copy.reason = Reason::Exhausted;
        candidate.copy.status = bs::Status::LimitExceeded; return false;
    }
    ++candidate.copy.equipmentEpoch; return true;
}
void BodySlotsDemo::Fail(Owner& candidate, bs::Status status, Reason reason) noexcept {
    candidate.copy.available = false; candidate.copy.token = {};
    candidate.copy.status = status; candidate.copy.reason = reason;
    if (status == bs::Status::Unauthorized && candidate.writer) {
        candidate.writer = 0; candidate.copy.connected = false; Advance(candidate);
    }
}
void BodySlotsDemo::Describe(Owner& candidate) noexcept {
    auto& out = candidate.copy; const auto& r = candidate.row.instance;
    if (!r.instanceHandle) {
        out.requestedActive = out.effectiveEnabled = out.available = false; out.token = {};
        out.pinnedTemplateRevision = out.currentTemplateRevision = 0;
        out.templateState = static_cast<unsigned>(bs::InstanceTemplateState::Missing);
        out.status = bs::Status::NotFound; out.reason = Reason::InstanceMissing; return;
    }
    out.requestedActive = (r.flags & bs::kInstanceActive) != 0;
    out.effectiveEnabled = r.templateState == static_cast<unsigned>(bs::InstanceTemplateState::Current) && (r.effective.flags & bs::kEnabled);
    out.pinnedTemplateRevision = r.pinnedDefinitionRevision; out.currentTemplateRevision = r.currentDefinitionRevision;
    out.templateState = r.templateState;
    out.token = {out.equipmentEpoch, out.view.saveGeneration, out.view.viewRevision, r.instanceHandle, r.instanceRevision};
    out.available = out.connected && out.observed && out.worn && out.requestedActive && out.effectiveEnabled &&
        (r.effective.contentMask & bs::Potion) && (r.effective.handMask & bs::Both);
    out.status = bs::Status::Ok;
    if (!out.observed) out.reason = Reason::EquipmentUnobserved;
    else if (!out.worn) out.reason = Reason::Unworn;
    else if (r.templateState != static_cast<unsigned>(bs::InstanceTemplateState::Current))
        out.reason = r.templateState == static_cast<unsigned>(bs::InstanceTemplateState::Missing) ? Reason::TemplateMissing : Reason::TemplateChanged;
    else out.reason = out.available ? Reason::Ready : Reason::Ineligible;
}
bool BodySlotsDemo::Connect() noexcept {
    if (owner_->copy.exhausted) return false;
    if (owner_->copy.connected) return true;
    auto next = *owner_; const auto epoch = next.copy.equipmentEpoch;
    try {
        const auto module = GetModuleHandleA("BodySlots.dll");
        const auto query = module ? reinterpret_cast<bs::QueryInterface>(GetProcAddress(module, "BodySlots_QueryInterface")) : nullptr;
        const auto instanceQuery = module ? reinterpret_cast<bs::QueryInstanceInterface>(GetProcAddress(module, "BodySlots_QueryInstanceInterface")) : nullptr;
        const auto configuredQuery = module ? reinterpret_cast<bs::QueryInstanceConfigurationInterface>(GetProcAddress(module, "BodySlots_QueryInstanceConfigurationInterface")) : nullptr;
        const auto activationQuery = module ? reinterpret_cast<bs::QueryInstanceActivationInterface>(GetProcAddress(module, "BodySlots_QueryInstanceActivationInterface")) : nullptr;
        if (!query || !instanceQuery || !configuredQuery || !activationQuery) {
            Fail(next, bs::Status::Unavailable, Reason::Disconnected); Publish(next, epoch); return false;
        }
        const auto* base = query(bs::kApiVersion, sizeof(bs::Api));
        const auto* instances = instanceQuery(bs::kInstanceApiVersion, sizeof(bs::InstanceApi));
        const auto* configured = configuredQuery(bs::kInstanceConfigurationApiVersion, sizeof(bs::InstanceConfigurationApi));
        const auto* activation = activationQuery(bs::kInstanceActivationApiVersion, sizeof(bs::InstanceActivationApi));
        if (!valid(base) || !valid(instances) || !valid(configured) || !valid(activation)) {
            Fail(next, bs::Status::UnsupportedVersion, Reason::BadInterface); Publish(next, epoch); return false;
        }
        if (next.registrations >= 2) { Fail(next, bs::Status::LimitExceeded, Reason::SourceUnavailable); Publish(next, epoch); return false; }
        if (!buffers_) buffers_ = std::make_unique<Buffers>(); // Preflight before acquiring any lease.
        next.base = *base; next.instances = *instances; next.configured = *configured; next.activation = *activation;
        ++next.registrations;
        std::uint64_t writer = 0;
        const auto status = next.base.RegisterClient(kEquipmentProvider, &writer);
        if (status != bs::Status::Ok || !writer) {
            Fail(next, status == bs::Status::Ok ? bs::Status::InternalError : status, Reason::SourceUnavailable);
            Publish(next, epoch); return false;
        }
        next.writer = writer; next.copy.connected = true;
        Fail(next, bs::Status::Ok, Reason::EquipmentUnobserved);
        return Publish(next, epoch);
    } catch (...) { Fail(next, bs::Status::InternalError, Reason::SourceUnavailable); Publish(next, epoch); return false; }
}
bool BodySlotsDemo::ReadConfigured(Owner& next) noexcept {
    try {
        bs::InstanceView header{sizeof(bs::InstanceView), bs::kInstanceApiVersion, 0, 0, 0, 0, 0, 0};
        const auto status = next.configured.CopyConfiguredInstances(next.writer, 0, 0,
            buffers_->instances.data(), bs::kMaxInstances, &header);
        if (status != bs::Status::Ok) { Fail(next, status, Reason::SourceUnavailable); return false; }
        if (!view(header)) { Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false; }
        next.copy.view = header; next.row = {};
        bool found = false;
        for (unsigned i = 0; i < header.count; ++i) {
            const auto& row = buffers_->instances[i];
            if (!record(row)) { Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false; }
            for (unsigned j = 0; j < i; ++j) if (row.instance.instanceHandle == buffers_->instances[j].instance.instanceHandle ||
                sameKey(row.instance.key, buffers_->instances[j].instance.key)) {
                Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false;
            }
            if (!target(row.instance.key)) continue;
            if (found || std::strcmp(row.instance.definitionProvider, kTemplateProvider) || std::strcmp(row.instance.definitionKey, kTemplateKey)) {
                Fail(next, bs::Status::Conflict, Reason::InstanceChanged); return false;
            }
            next.row = row; found = true;
        }
        Describe(next); return true;
    } catch (...) { Fail(next, bs::Status::InternalError, Reason::SourceUnavailable); return false; }
}
bool BodySlotsDemo::CreateInactive(Owner& next) noexcept {
    try {
        std::uint32_t count = 0; std::uint64_t revision = 0;
        const auto status = next.base.CopyDefinitions(next.writer, 0, buffers_->definitions.data(), bs::kMaxDefinitions, &count, &revision);
        if (status != bs::Status::Ok) { Fail(next, status, Reason::SourceUnavailable); return false; }
        if (!revision || count > bs::kMaxDefinitions) { Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false; }
        const bs::SlotRecord* selected = nullptr;
        for (unsigned i = 0; i < count; ++i) {
            const auto& row = buffers_->definitions[i];
            if (!definition(row.definition) || !row.definitionRevision || !row.slotId) { Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false; }
            if (std::strcmp(row.definition.provider, kTemplateProvider) || std::strcmp(row.definition.key, kTemplateKey)) continue;
            if (selected) { Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false; }
            selected = &row;
        }
        if (!selected) { Fail(next, bs::Status::NotFound, Reason::TemplateMissing); return false; }
        bs::InstanceSpec spec{}; spec.structSize = sizeof(spec);
        std::memcpy(spec.equipmentKey, kEquipmentKey, std::strlen(kEquipmentKey) + 1);
        std::memcpy(spec.slotKey, kSlotKey, std::strlen(kSlotKey) + 1);
        std::memcpy(spec.definitionProvider, kTemplateProvider, std::strlen(kTemplateProvider) + 1);
        std::memcpy(spec.definitionKey, kTemplateKey, std::strlen(kTemplateKey) + 1);
        spec.definitionRevision = selected->definitionRevision; spec.binding = static_cast<unsigned>(bs::InstanceBinding::BodyAnchor);
        bs::InstanceRecord committed{}; committed.structSize = sizeof(committed);
        const auto created = next.instances.UpsertInstance(next.writer, &spec, next.copy.view.saveGeneration, 0, &committed);
        if (created != bs::Status::Ok) { Fail(next, created, Reason::SourceUnavailable); return false; }
        const bs::ConfiguredInstanceRecord checked{committed, 0, 0};
        if (!record(checked) || !target(committed.key) || committed.flags ||
            std::strcmp(committed.definitionProvider, kTemplateProvider) || std::strcmp(committed.definitionKey, kTemplateKey)) {
            Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false;
        }
        const auto scope = next.copy.view.saveGeneration;
        if (!ReadConfigured(next)) return false;
        if (next.copy.view.saveGeneration != scope || next.row.instance.instanceHandle != committed.instanceHandle) {
            Fail(next, bs::Status::Conflict, Reason::ScopeChanged); return false;
        }
        return true;
    } catch (...) { Fail(next, bs::Status::InternalError, Reason::SourceUnavailable); return false; }
}
bool BodySlotsDemo::SetActive(Owner& next, bool active) noexcept {
    const auto& r = next.row.instance;
    if (bool(r.flags & bs::kInstanceActive) == active) return true;
    try {
        const auto prior = next.copy.view; const auto handle = r.instanceHandle;
        const bs::ActivationChange change{sizeof(bs::ActivationChange), active ? bs::kInstanceActive : 0, handle, r.instanceRevision};
        bs::InstanceView committed{sizeof(bs::InstanceView), bs::kInstanceApiVersion, 0, 0, 0, 0, 0, 0};
        const auto status = next.activation.ApplyActivationChanges(next.writer, &change, 1,
            prior.saveGeneration, prior.viewRevision, &committed);
        if (status != bs::Status::Ok) { Fail(next, status, Reason::SourceUnavailable); return false; }
        if (!view(committed) || committed.saveGeneration != prior.saveGeneration || committed.viewRevision != prior.viewRevision + 1 ||
            committed.registryRevision != prior.registryRevision || committed.configurationRevision != prior.configurationRevision || committed.count != prior.count) {
            Fail(next, bs::Status::InternalError, Reason::BadSnapshot); return false;
        }
        if (!ReadConfigured(next)) return false;
        if (next.copy.view.saveGeneration != prior.saveGeneration || next.row.instance.instanceHandle != handle ||
            bool(next.row.instance.flags & bs::kInstanceActive) != active) {
            Fail(next, bs::Status::Conflict, Reason::InstanceChanged); return false;
        }
        return true;
    } catch (...) { Fail(next, bs::Status::InternalError, Reason::SourceUnavailable); return false; }
}
bool BodySlotsDemo::Reconcile(bool observed, bool worn) noexcept {
    if (owner_->copy.exhausted) return false;
    if (!owner_->copy.connected && !Connect()) return false;
    auto next = *owner_;
    if (next.copy.observed != observed || next.copy.worn != (observed && worn)) Advance(next);
    next.copy.observed = observed; next.copy.worn = observed && worn;
    Fail(next, bs::Status::Unavailable, observed ? Reason::SourceUnavailable : Reason::EquipmentUnobserved);
    Commit(next); // Pending observations cannot grant old admission during provider calls.
    const auto epoch = next.copy.equipmentEpoch;
    if (next.copy.exhausted) return false;
    if (!ReadConfigured(next)) { Publish(next, epoch); return false; }
    if (!next.row.instance.instanceHandle && !CreateInactive(next)) { Publish(next, epoch); return false; }
    const bool current = next.row.instance.templateState == static_cast<unsigned>(bs::InstanceTemplateState::Current);
    const bool desired = observed && worn && current;
    if (!SetActive(next, desired)) { Publish(next, epoch); return false; }
    Describe(next);
    const bool reconciled = observed && (!worn || current);
    return Publish(next, epoch) && reconciled;
}
void BodySlotsDemo::InvalidateEquipment() noexcept {
    auto next = *owner_;
    if (Advance(next)) { next.copy.status = bs::Status::Unavailable; next.copy.reason = Reason::EquipmentInvalidated; }
    Commit(next);
}
void BodySlotsDemo::Reset() noexcept {
    auto next = *owner_;
    if (Advance(next)) {
        next.row = {}; next.copy.observed = next.copy.worn = next.copy.requestedActive = next.copy.effectiveEnabled = false;
        next.copy.view = {}; next.copy.pinnedTemplateRevision = next.copy.currentTemplateRevision = 0;
        next.copy.templateState = static_cast<unsigned>(bs::InstanceTemplateState::Missing);
        next.copy.status = bs::Status::Unavailable; next.copy.reason = Reason::Reset;
    }
    Commit(next);
}
BodySlotsDemo::CopiedState BodySlotsDemo::Copy() const noexcept { return owner_->copy; }
BodySlotsDemo::Token BodySlotsDemo::CurrentToken() const noexcept { return owner_->copy.available ? owner_->copy.token : Token{}; }
bool BodySlotsDemo::Allows(bool isLeft, const Token* captured) noexcept {
    if (!owner_->copy.available || owner_->copy.exhausted || (captured && (!captured->Valid() || *captured != owner_->copy.token))) return false;
    auto next = *owner_; const auto epoch = next.copy.equipmentEpoch; const auto prior = next.copy.token;
    if (!ReadConfigured(next)) { Publish(next, epoch); return false; }
    if (next.copy.view.saveGeneration != prior.saveGeneration || !next.row.instance.instanceHandle) {
        Fail(next, bs::Status::Conflict, Reason::ScopeChanged); Advance(next); Publish(next, epoch); return false;
    }
    if (next.row.instance.instanceHandle != prior.instanceHandle || next.row.instance.instanceRevision != prior.instanceRevision) {
        Fail(next, bs::Status::Conflict, Reason::InstanceChanged); Publish(next, epoch); return false;
    }
    Describe(next);
    if (!next.copy.available || (captured && *captured != next.copy.token)) {
        if (captured && *captured != next.copy.token) Fail(next, bs::Status::Conflict, Reason::StaleToken);
        Publish(next, epoch); return false;
    }
    try {
        std::uint32_t allowed = 0;
        const auto status = next.configured.EvaluateConfiguredEligibility(next.writer, next.row.instance.instanceHandle,
            next.copy.view.saveGeneration, next.copy.view.viewRevision, isLeft ? bs::Left : bs::Right, bs::Potion, &allowed);
        if (status != bs::Status::Ok || allowed > 1) {
            Fail(next, status == bs::Status::Ok ? bs::Status::InternalError : status,
                allowed > 1 ? Reason::BadSnapshot : Reason::SourceUnavailable);
            Publish(next, epoch); return false;
        }
        next.copy.reason = allowed ? Reason::Ready : Reason::Ineligible;
        return Publish(next, epoch) && allowed == 1;
    } catch (...) { Fail(next, bs::Status::InternalError, Reason::SourceUnavailable); Publish(next, epoch); return false; }
}
const char* BodySlotsDemo::ReasonName(Reason reason) noexcept {
    switch (reason) {
    case Reason::Disconnected: return "disconnected";
    case Reason::Ready: return "ready";
    case Reason::EquipmentInvalidated: return "equipment_invalidated";
    case Reason::EquipmentUnobserved: return "equipment_unobserved";
    case Reason::Unworn: return "unworn";
    case Reason::SourceUnavailable: return "source_unavailable";
    case Reason::BadInterface: return "bad_interface";
    case Reason::BadSnapshot: return "bad_snapshot";
    case Reason::TemplateMissing: return "template_missing";
    case Reason::TemplateChanged: return "template_changed";
    case Reason::InstanceMissing: return "instance_missing";
    case Reason::ScopeChanged: return "scope_changed";
    case Reason::InstanceChanged: return "instance_changed";
    case Reason::StaleToken: return "stale_token";
    case Reason::Ineligible: return "ineligible";
    case Reason::Reset: return "reset";
    case Reason::Exhausted: return "exhausted";
    }
    return "unknown";
}
}
