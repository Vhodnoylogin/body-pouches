// Compile exact adapter bodies against a loader/table boundary double. The only
// private inspection injects unreachable epoch exhaustion; no production setter.
#include <memory>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#define private public
#include "../src/vr/BodySlotsDemo.cpp"
#undef private

namespace {
namespace bs = body_slots;
using Demo = BodyPouches::VR::BodySlotsDemo;
unsigned checks = 0, registrations = 0, reads = 0, activations = 0, definitionsWritten = 0;
#define CHECK(v) do { ++checks; if (!(v)) throw std::runtime_error("line " + std::to_string(__LINE__) + ": " #v); } while(false)
bool advertised = true, hasInstance = false, unauthorized = false, malformed = false;
bool throwRead = false;
std::uint64_t writer = 0, revision = 1;
bs::Status readStatus = bs::Status::Ok, registerStatus = bs::Status::Ok, activationStatus = bs::Status::Ok;
std::function<void()> readHook, eligibilityHook;
bs::ConfiguredInstanceRecord stored{};
bs::Definition templ() {
    bs::Definition d{}; d.structSize = sizeof(d); d.flags = bs::kEnabled;
    std::strcpy(d.provider, Demo::kTemplateProvider); std::strcpy(d.key, Demo::kTemplateKey);
    std::strcpy(d.anchor, "NPC Spine1 [Spn1]"); std::strcpy(d.referenceRig, "skyrimvr-humanoid-v1");
    std::strcpy(d.kind, "potion-bag"); d.rotation[0] = d.rotation[4] = d.rotation[8] = 1;
    d.radius = 10; d.handMask = bs::Both; d.contentMask = bs::Potion; return d;
}
bs::InstanceView header() { return {sizeof(bs::InstanceView), 1, 1, revision, 1, 1, hasInstance ? 1u : 0u, 0}; }
bs::Status BS_CALL reg(const char* provider, std::uint64_t* out) {
    CHECK(!std::strcmp(provider, Demo::kEquipmentProvider)); ++registrations;
    if (registerStatus != bs::Status::Ok) return registerStatus;
    unauthorized = false; *out = writer = 700 + registrations; return bs::Status::Ok;
}
bs::Status BS_CALL release(std::uint64_t) { return bs::Status::Ok; }
bs::Status BS_CALL upsertDef(std::uint64_t, const bs::Definition*, std::uint64_t, bs::SlotRecord*) { ++definitionsWritten; return bs::Status::Unauthorized; }
bs::Status BS_CALL removeDef(std::uint64_t, const char*, std::uint64_t) { ++definitionsWritten; return bs::Status::Unauthorized; }
bs::Status BS_CALL definitions(std::uint64_t client, std::uint64_t expected, bs::SlotRecord* out, std::uint32_t capacity, std::uint32_t* count, std::uint64_t* rev) {
    CHECK(client == writer && !expected && capacity == bs::kMaxDefinitions);
    *out = {templ(), 1, 1}; *count = 1; *rev = 1; return bs::Status::Ok;
}
bs::Status BS_CALL eligible(std::uint64_t, std::uint64_t, std::uint64_t, std::uint32_t, std::uint32_t, std::uint32_t*) { return bs::Status::Unauthorized; }
bs::Status BS_CALL replaceDef(std::uint64_t, const bs::Definition*, std::uint32_t, std::uint64_t) { ++definitionsWritten; return bs::Status::Unauthorized; }
bs::Status BS_CALL observer(std::uint64_t*) { return bs::Status::Unauthorized; }
bs::Status BS_CALL upsert(std::uint64_t client, const bs::InstanceSpec* spec, std::uint64_t scope, std::uint64_t expected, bs::InstanceRecord* out) {
    CHECK(client == writer && scope == 1 && !expected && !spec->flags);
    CHECK(!std::strcmp(spec->definitionProvider, Demo::kTemplateProvider) && !std::strcmp(spec->definitionKey, Demo::kTemplateKey));
    stored = {}; auto& r = stored.instance; r.structSize = sizeof(r);
    std::strcpy(r.key.ownerProvider, Demo::kEquipmentProvider); std::strcpy(r.key.equipmentKey, Demo::kEquipmentKey); std::strcpy(r.key.slotKey, Demo::kSlotKey);
    std::strcpy(r.definitionProvider, Demo::kTemplateProvider); std::strcpy(r.definitionKey, Demo::kTemplateKey);
    r.instanceHandle = 3001; r.instanceRevision = 1; r.pinnedDefinitionRevision = r.currentDefinitionRevision = 1;
    r.binding = static_cast<unsigned>(bs::InstanceBinding::BodyAnchor); r.effective = templ();
    hasInstance = true; ++revision; *out = r; return bs::Status::Ok;
}
bs::Status BS_CALL remove(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t) { return bs::Status::Unauthorized; }
bs::Status BS_CALL inherited(std::uint64_t, std::uint64_t, std::uint64_t, bs::InstanceRecord*, std::uint32_t, bs::InstanceView*) { return bs::Status::Unavailable; }
bs::Status BS_CALL instanceEligible(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t, std::uint32_t, std::uint32_t, std::uint32_t*) { return bs::Status::Unauthorized; }
bs::Status BS_CALL configured(std::uint64_t client, std::uint64_t scope, std::uint64_t view, bs::ConfiguredInstanceRecord* out, std::uint32_t capacity, bs::InstanceView* info) {
    ++reads; CHECK(client == writer && !scope && !view && capacity == bs::kMaxInstances);
    if (throwRead) throw std::runtime_error("injected provider exception");
    if (unauthorized) return bs::Status::Unauthorized;
    if (readStatus != bs::Status::Ok) return readStatus;
    *info = header(); if (hasInstance) *out = stored;
    if (malformed) info->count = bs::kMaxInstances + 1;
    if (readHook) { auto hook = std::move(readHook); readHook = {}; hook(); }
    return bs::Status::Ok;
}
bs::Status BS_CALL finalEligible(std::uint64_t client, std::uint64_t handle, std::uint64_t scope, std::uint64_t view, std::uint32_t hand, std::uint32_t content, std::uint32_t* allowed) {
    CHECK(client == writer && handle == stored.instance.instanceHandle && scope == 1 && view == revision && content == bs::Potion);
    *allowed = (stored.instance.flags & bs::kInstanceActive) && (stored.instance.effective.flags & bs::kEnabled) && (stored.instance.effective.handMask & hand);
    if (eligibilityHook) { auto hook = std::move(eligibilityHook); eligibilityHook = {}; hook(); }
    return bs::Status::Ok;
}
bs::Status BS_CALL edit(std::uint64_t, const bs::InstanceCustomization*, std::uint64_t, std::uint64_t, bs::ConfiguredInstanceRecord*) { return bs::Status::Unauthorized; }
bs::Status BS_CALL clear(std::uint64_t, const bs::InstanceCustomizationKey*, std::uint64_t, std::uint64_t) { return bs::Status::Unauthorized; }
bs::Status BS_CALL customizations(std::uint64_t, std::uint64_t, std::uint64_t, bs::InstanceCustomizationRecord*, std::uint32_t, bs::InstanceView*) { return bs::Status::Unauthorized; }
bs::Status BS_CALL activate(std::uint64_t client, const bs::ActivationChange* change, std::uint32_t count, std::uint64_t scope, std::uint64_t view, bs::InstanceView* out) {
    ++activations; CHECK(client == writer && count == 1 && scope == 1 && view == revision && change->expectedInstanceRevision == stored.instance.instanceRevision);
    if (activationStatus != bs::Status::Ok) return activationStatus;
    stored.instance.flags = change->flags; ++stored.instance.instanceRevision; ++revision; *out = header(); return bs::Status::Ok;
}
bs::Api base{sizeof(bs::Api), 1, 7, reg, release, upsertDef, removeDef, definitions, eligible, replaceDef, observer};
bs::InstanceApi instances{sizeof(bs::InstanceApi), 1, 3, upsert, remove, inherited, instanceEligible};
bs::InstanceConfigurationApi final{sizeof(bs::InstanceConfigurationApi), 1, 3, edit, clear, configured, customizations, finalEligible};
bs::InstanceActivationApi activation{sizeof(bs::InstanceActivationApi), 1, 1, activate};
const bs::Api* BS_CALL query(unsigned, unsigned) { return &base; }
const bs::InstanceApi* BS_CALL queryInstances(unsigned, unsigned) { return &instances; }
const bs::InstanceConfigurationApi* BS_CALL queryFinal(unsigned, unsigned) { return &final; }
const bs::InstanceActivationApi* BS_CALL queryActivation(unsigned, unsigned) { return &activation; }
void reset() {
    advertised = true; hasInstance = unauthorized = malformed = throwRead = false;
    writer = 0; revision = 1; registrations = reads = activations = definitionsWritten = 0;
    registerStatus = readStatus = activationStatus = bs::Status::Ok; stored = {}; readHook = eligibilityHook = {};
    base.capabilities = 7; base.version = 1; final.capabilities = 3; final.EvaluateConfiguredEligibility = finalEligible;
    activation.structSize = sizeof(activation); instances.version = 1;
}
}
HMODULE GetModuleHandleA(const char* name) { CHECK(!std::strcmp(name, "BodySlots.dll")); return advertised ? reinterpret_cast<void*>(1) : nullptr; }
FARPROC GetProcAddress(HMODULE, const char* name) {
    if (!std::strcmp(name, "BodySlots_QueryInterface")) return reinterpret_cast<FARPROC>(query);
    if (!std::strcmp(name, "BodySlots_QueryInstanceInterface")) return reinterpret_cast<FARPROC>(queryInstances);
    if (!std::strcmp(name, "BodySlots_QueryInstanceConfigurationInterface")) return reinterpret_cast<FARPROC>(queryFinal);
    if (!std::strcmp(name, "BodySlots_QueryInstanceActivationInterface")) return reinterpret_cast<FARPROC>(queryActivation);
    return nullptr;
}
int main() { try {
    for (unsigned bad = 0; bad < 6; ++bad) {
        reset(); if (bad == 0) advertised = false; if (bad == 1) base.capabilities = 0;
        if (bad == 2) instances.version = 2; if (bad == 3) final.capabilities = 0;
        if (bad == 4) final.EvaluateConfiguredEligibility = nullptr; if (bad == 5) --activation.structSize;
        Demo demo; CHECK(!demo.Connect() && registrations == 0 && !demo.Allows(true));
    }
    reset(); { Demo demo; registerStatus = bs::Status::LimitExceeded;
        for (unsigned i = 0; i < 8; ++i) CHECK(!demo.Reconcile(true, true)); CHECK(registrations == 2); }
    reset(); { Demo demo; CHECK(demo.Reconcile(true, true) && demo.Allows(true)); const auto token = demo.CurrentToken();
        for (auto error : {bs::Status::Unavailable, bs::Status::Conflict, bs::Status::InternalError, bs::Status::BufferTooSmall}) {
            readStatus = error; CHECK(!demo.Allows(true, &token) && !demo.CurrentToken().Valid()); readStatus = bs::Status::Ok;
            CHECK(demo.Reconcile(true, true) && demo.Allows(true)); }
        malformed = true; CHECK(!demo.Reconcile(true, true) && demo.Copy().reason == Demo::Reason::BadSnapshot);
        malformed = false; throwRead = true; CHECK(!demo.Reconcile(true, true)); throwRead = false;
        CHECK(demo.Reconcile(true, true));
        activationStatus = bs::Status::Conflict; CHECK(!demo.Reconcile(true, false) && !demo.Allows(true));
        activationStatus = bs::Status::Ok; CHECK(demo.Reconcile(true, false) && !stored.instance.flags);
        CHECK(demo.Reconcile(true, true));
        readHook = [&] { demo.InvalidateEquipment(); }; CHECK(!demo.Reconcile(true, true) && !demo.CurrentToken().Valid());
        CHECK(demo.Reconcile(true, true)); eligibilityHook = [&] { demo.InvalidateEquipment(); };
        CHECK(!demo.Allows(true) && !demo.CurrentToken().Valid()); CHECK(demo.Reconcile(true, true));
        const auto old = demo.CurrentToken(); unauthorized = true; CHECK(!demo.Allows(true) && !demo.Copy().connected);
        CHECK(demo.Reconcile(true, true) && registrations == 2 && !demo.Allows(true, &old));
        unauthorized = true; CHECK(!demo.Reconcile(true, true));
        for (unsigned i = 0; i < 8; ++i) CHECK(!demo.Reconcile(true, true)); CHECK(registrations == 2);
        CHECK(definitionsWritten == 0);
    }
    reset(); { Demo demo; CHECK(demo.Reconcile(true, true));
        demo.owner_->copy.equipmentEpoch = (std::numeric_limits<std::uint64_t>::max)();
        demo.InvalidateEquipment(); const auto before = reads;
        CHECK(demo.Copy().exhausted && !demo.CurrentToken().Valid()); demo.Reset();
        CHECK(demo.Copy().exhausted && !demo.Connect() && !demo.Reconcile(true, true) && !demo.Allows(true) && reads == before);
    }
    std::cout << "PASS " << checks << " demo adapter loader/failure/epoch checks; explicit table doubles only\n"; return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; } }
