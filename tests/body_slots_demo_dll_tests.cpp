// Actual public BodySlots.dll + production demo adapter. Observed/worn are explicit
// input doubles; this synthetic SKSE host never loads BodyPouches.dll or the game.
#include "vr/BodySlotsDemo.h"
#include <Windows.h>
#pragma warning(push)
#pragma warning(disable:4244 4201) // Existing public SDK headers, not adapter code.
#include "common/ITypes.h"
#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#pragma warning(pop)
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bs = body_slots;
using Demo = BodyPouches::VR::BodySlotsDemo;
static unsigned checks = 0;
#define CHECK(v) do { ++checks; if (!(v)) throw std::runtime_error("line " + std::to_string(__LINE__) + ": " #v); } while(false)
static SKSESerializationInterface::EventCallback reverted = nullptr;
static SKSEMessagingInterface messaging{};
static SKSESerializationInterface serialization{};
static void uniqueId(PluginHandle, UInt32) {}
static void callback(PluginHandle, SKSESerializationInterface::EventCallback) {}
static void onRevert(PluginHandle, SKSESerializationInterface::EventCallback fn) { reverted = fn; }
static bool writeRecord(UInt32, UInt32, const void*, UInt32) { return true; }
static bool nextRecord(UInt32*, UInt32*, UInt32*) { return false; }
static UInt32 readRecord(void*, UInt32) { return 0; }
static bool listen(PluginHandle, const char*, SKSEMessagingInterface::EventCallback) { return true; }
static PluginHandle handle() { return 42; }
static void* interfaceFor(UInt32 id) {
    if (id == kInterface_Messaging) return &messaging;
    return id == kInterface_Serialization ? &serialization : nullptr;
}
static void bind(HMODULE module) {
    messaging.interfaceVersion = SKSEMessagingInterface::kInterfaceVersion; messaging.RegisterListener = listen;
    serialization.version = SKSESerializationInterface::kVersion;
    serialization.SetUniqueID = uniqueId; serialization.SetSaveCallback = callback;
    serialization.SetLoadCallback = callback; serialization.SetRevertCallback = onRevert;
    serialization.WriteRecord = writeRecord; serialization.GetNextRecordInfo = nextRecord; serialization.ReadRecordData = readRecord;
    SKSEInterface skse{}; skse.runtimeVersion = RUNTIME_VR_VERSION_1_4_15; skse.skseVersion = PACKED_SKSE_VERSION;
    skse.QueryInterface = interfaceFor; skse.GetPluginHandle = handle;
    const auto load = reinterpret_cast<_SKSEPlugin_Load>(GetProcAddress(module, "SKSEPlugin_Load"));
    CHECK(load && load(&skse) && reverted);
}
static bs::InstanceView freshView() { return {sizeof(bs::InstanceView), 1, 0, 0, 0, 0, 0, 0}; }
int main(int argc, char** argv) { try {
    if (argc != 2) return 2;
    Demo demo;
    CHECK(!demo.Connect() && !demo.Allows(true) && !demo.CurrentToken().Valid());
    const auto module = LoadLibraryA(argv[1]); CHECK(module); bind(module);
    const auto query = reinterpret_cast<bs::QueryInterface>(GetProcAddress(module, "BodySlots_QueryInterface"));
    const auto finalQuery = reinterpret_cast<bs::QueryInstanceConfigurationInterface>(GetProcAddress(module, "BodySlots_QueryInstanceConfigurationInterface"));
    const auto configQuery = reinterpret_cast<bs::QueryConfigurationInterface>(GetProcAddress(module, "BodySlots_QueryConfigurationInterface"));
    CHECK(query && finalQuery && configQuery);
    const auto* api = query(1, sizeof(bs::Api)); const auto* final = finalQuery(1, sizeof(bs::InstanceConfigurationApi));
    const auto* config = configQuery(1, sizeof(bs::ConfigurationApi)); CHECK(api && final && config);
    CHECK(demo.Connect() && demo.Connect());
    CHECK(!demo.Reconcile(true, true) && demo.Copy().reason == Demo::Reason::TemplateMissing);
    std::uint64_t writer = 0, editor = 0;
    CHECK(api->RegisterClient(Demo::kTemplateProvider, &writer) == bs::Status::Ok);
    CHECK(config->RegisterCustomizer("body-pouches-demo-test-editor", &editor) == bs::Status::Ok);
    bs::Definition definition{}; definition.structSize = sizeof(definition); definition.flags = bs::kEnabled;
    std::strcpy(definition.provider, Demo::kTemplateProvider); std::strcpy(definition.key, Demo::kTemplateKey);
    std::strcpy(definition.anchor, "NPC Spine1 [Spn1]"); std::strcpy(definition.referenceRig, "skyrimvr-humanoid-v1");
    std::strcpy(definition.kind, "potion-bag"); definition.rotation[0] = definition.rotation[4] = definition.rotation[8] = 1;
    definition.radius = 12; definition.handMask = bs::Both; definition.contentMask = bs::Potion;
    bs::SlotRecord authored{}; CHECK(api->UpsertDefinition(writer, &definition, 0, &authored) == bs::Status::Ok);
    CHECK(demo.Reconcile(true, false)); CHECK(!demo.Copy().available && !demo.Copy().requestedActive);
    CHECK(!demo.Allows(true) && !demo.CurrentToken().Valid());
    CHECK(demo.Reconcile(true, true)); CHECK(demo.Copy().available && demo.Copy().requestedActive);
    CHECK(demo.Allows(true) && demo.Allows(false));
    const auto first = demo.CurrentToken(); CHECK(first.Valid());
    CHECK(demo.Reconcile(true, true) && demo.CurrentToken() == first); // no-op uses no revision/epoch.
    CHECK(demo.Allows(true, &first));
    auto unrelated = definition;
    std::strcpy(unrelated.key, "unrelated-definition");
    bs::SlotRecord other{};
    CHECK(api->UpsertDefinition(writer, &unrelated, 0, &other) == bs::Status::Ok);
    CHECK(!demo.Allows(true, &first)); // A global view change fences queued work conservatively.
    CHECK(demo.Reconcile(true, true) && demo.CurrentToken().instanceRevision == first.instanceRevision);
    CHECK(demo.CurrentToken().viewRevision != first.viewRevision && demo.Allows(true));
    demo.InvalidateEquipment(); demo.InvalidateEquipment();
    CHECK(!demo.Allows(true, &first) && !demo.CurrentToken().Valid());
    CHECK(demo.Reconcile(true, true)); const auto cycled = demo.CurrentToken();
    CHECK(cycled.Valid() && cycled.equipmentEpoch > first.equipmentEpoch && cycled.instanceRevision == first.instanceRevision);
    CHECK(!demo.Allows(true, &first) && demo.Allows(true, &cycled));
    CHECK(demo.Reconcile(true, false) && !demo.Allows(true, &cycled));
    CHECK(!demo.Reconcile(false, true) && !demo.Copy().observed && !demo.Copy().requestedActive);
    CHECK(demo.Reconcile(true, true)); const auto beforeEdit = demo.CurrentToken();
    bs::InstanceCustomization patch{}; patch.structSize = sizeof(patch); patch.fieldMask = bs::CustomizeEnabled;
    std::strcpy(patch.key.instance.ownerProvider, Demo::kEquipmentProvider);
    std::strcpy(patch.key.instance.equipmentKey, Demo::kEquipmentKey); std::strcpy(patch.key.instance.slotKey, Demo::kSlotKey);
    std::strcpy(patch.key.definitionProvider, Demo::kTemplateProvider); std::strcpy(patch.key.definitionKey, Demo::kTemplateKey);
    bs::ConfiguredInstanceRecord committed{}; committed.instance.structSize = sizeof(bs::InstanceRecord);
    auto state = demo.Copy();
    CHECK(final->ApplyCustomization(editor, &patch, state.view.saveGeneration, state.view.viewRevision, &committed) == bs::Status::Ok);
    CHECK(!demo.Allows(true, &beforeEdit));
    CHECK(demo.Reconcile(true, true) && !demo.Copy().available && demo.Copy().requestedActive);
    auto disabledView = demo.Copy().view;
    patch.enabled = 1;
    CHECK(final->ApplyCustomization(editor, &patch, disabledView.saveGeneration, disabledView.viewRevision, &committed) == bs::Status::Ok);
    CHECK(demo.Reconcile(true, true));
    CHECK(!demo.Allows(true, &beforeEdit));
    CHECK(demo.Allows(true)); const auto afterEdit = demo.CurrentToken();
    CHECK(afterEdit.instanceRevision == beforeEdit.instanceRevision && afterEdit.viewRevision != beforeEdit.viewRevision);
    demo.Reset(); CHECK(!demo.CurrentToken().Valid() && !demo.Allows(true, &afterEdit));
    CHECK(demo.Reconcile(true, false) && !demo.Copy().requestedActive);
    CHECK(demo.Reconcile(true, true)); const auto oldScope = demo.CurrentToken();
    reverted(&serialization); CHECK(!demo.Allows(true, &oldScope));
    CHECK(demo.Reconcile(true, true)); const auto recreated = demo.CurrentToken();
    CHECK(recreated.Valid() && recreated.saveGeneration != oldScope.saveGeneration && recreated.instanceHandle != oldScope.instanceHandle);
    CHECK(!demo.Allows(true, &oldScope) && demo.Allows(true, &recreated));
    CHECK(api->RemoveDefinition(writer, Demo::kTemplateKey, authored.definitionRevision) == bs::Status::Ok);
    CHECK(!demo.Reconcile(true, true) && !demo.Copy().available && !demo.Copy().requestedActive);
    CHECK(demo.Copy().reason == Demo::Reason::TemplateMissing && !demo.Allows(true));
    CHECK(demo.Reconcile(true, false)); // retained missing-template deactivation is not blocked.
    CHECK(api->UpsertDefinition(writer, &definition, 0, &authored) == bs::Status::Ok);
    CHECK(!demo.Reconcile(true, true) && demo.Copy().reason == Demo::Reason::TemplateChanged); // never silently repin.
    reverted(&serialization); CHECK(demo.Reconcile(true, true) && demo.Allows(true));
    definition.handMask = bs::Left;
    CHECK(api->UpsertDefinition(writer, &definition, authored.definitionRevision, &authored) == bs::Status::Ok);
    reverted(&serialization); CHECK(demo.Reconcile(true, true));
    CHECK(!demo.Allows(false) && demo.CurrentToken().Valid() && demo.Allows(true));
    std::uint64_t observer = 0; CHECK(api->RegisterObserver(&observer) == bs::Status::Ok);
    std::vector<bs::ConfiguredInstanceRecord> rows(bs::kMaxInstances);
    auto view = freshView(); CHECK(final->CopyConfiguredInstances(observer, 0, 0, rows.data(), bs::kMaxInstances, &view) == bs::Status::Ok);
    CHECK(view.count == 1 && rows[0].instance.flags == bs::kInstanceActive);
    CHECK(!std::strcmp(rows[0].instance.key.ownerProvider, Demo::kEquipmentProvider));
    CHECK(!std::strcmp(rows[0].instance.definitionProvider, Demo::kTemplateProvider));
    demo.Reconcile(true, false); view = freshView();
    CHECK(final->CopyConfiguredInstances(observer, 0, 0, rows.data(), bs::kMaxInstances, &view) == bs::Status::Ok && rows[0].instance.flags == 0);
    CHECK(api->ReleaseClient(observer) == bs::Status::Ok);
    CHECK(config->ReleaseCustomizer(editor) == bs::Status::Ok);
    CHECK(api->ReleaseClient(writer) == bs::Status::Ok);
    // Process owns BodySlots.dll callbacks. Never FreeLibrary a registered provider.
    std::cout << "PASS " << checks << " demo adapter/actual BodySlots DLL checks; observed-worn inputs simulated; game starts=0\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; } }
