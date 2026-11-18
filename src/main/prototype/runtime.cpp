// SPDX-License-Identifier: MIT
// Body Pouches 2: gameplay policy only; Framework owns geometry/presentation,
// HIGGS owns input/grants/physical inventory operations. No controller hook.
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "potion_policy.h"
#include "core/Assignments.h"
#include "game/Forms.h"
#include "higgsinterface001.h"
#include "body_slots_equipment_api.h"
#include "body_slots_content_observation_api.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>

namespace BodyPouches::Prototype {
namespace bs = body_slots;
namespace sdk = body_slots::examples;
constexpr char kProvider[] = "BodyPouches2";
constexpr char kEquipment[] = "single-potion-bag";
constexpr char kSlot[] = "potions";
constexpr std::uint32_t kAssignment = 0x42503241; // BP2A, not legacy ASGN
std::uint64_t NowNs() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
template<std::size_t N> void Text(char (&out)[N], const char* value) { strncpy_s(out, value, _TRUNCATE); }
template<class T> const T* Framework(const char* name) {
    const auto module = GetModuleHandleW(L"BodySlots.dll");
    const auto query = module ? reinterpret_cast<const T* (BS_CALL*)(std::uint32_t, std::uint32_t)>(
        GetProcAddress(module, name)) : nullptr;
    const auto* table = query ? query(1, sizeof(T)) : nullptr;
    return table && table->structSize == sizeof(T) && table->version == 1 ? table : nullptr;
}
template<class Query, class T> bool Higgs(const char* name, std::uint32_t version, T& table) {
    const auto module = GetModuleHandleW(L"higgs_vr.dll");
    const auto query = module ? reinterpret_cast<Query>(GetProcAddress(module, name)) : nullptr;
    table = {};
    return query && query(version, sizeof(table), &table) == HIGGS_HAND_OK &&
        table.structSize == sizeof(table) && table.abiVersion == version;
}
bool Drinkable(RE::AlchemyItem* potion) {
    return potion && potion->GetPlayable() && !potion->IsFood() && !potion->IsPoison();
}
Potion Describe(RE::AlchemyItem* potion) {
    Potion result;
    result.item = Game::Describe(potion);
    result.form = potion->GetFormID();
    const auto* model = potion->GetModel();
    if (model && strlen(model) < 192) result.model = model;
    return result;
}

// One owner for mutable integration state. Callbacks only read the immutable
// policy snapshot; serialization changes only its separately locked assignment.
class PouchApp final {
public:
    static PouchApp& Get() { static PouchApp app; return app; }
    void Connect();
    void Tick();
    void Suspend() noexcept { loading_ = true; policy_.store(nullptr); cancel_ = true; }
    void Resume() noexcept { loading_ = false; refresh_ = true; }
    void Save(SKSE::SerializationInterface*);
    void Load(SKSE::SerializationInterface*);
    void Revert() {
        Suspend();
        std::lock_guard lock(assignmentMutex_);
        filter_ = {};
    }
private:
    struct Buffers {
        std::array<bs::InstanceRecord, bs::kMaxInstances> instances;
        std::array<bs::RuntimeRecord, bs::kMaxInstances> runtime;
    };
    const bs::Api* registry_{};
    const bs::InstanceApi* instances_{};
    const bs::EquipmentApi* equipment_{};
    const bs::RuntimeApi* runtime_{};
    const bs::BehaviorApi* behavior_{};
    const bs::GameApi* game_{};
    const bs::ContentObservationApi* content_{};
    sdk::InventoryProviderTables tables_;
    sdk::OrdinaryInventoryProvider inventory_;
    std::uint64_t writer_{}, higgsClient_{}, scope_{}, epoch_{}, operationScope_{}, processedTransfer_{}, processedEpoch_{};
    std::uint64_t refreshedAt_{}, definitionRevision_{};
    bs::InstanceRecord instance_{};
    bs::InstanceView view_{};
    std::uint32_t bagForm_{};
    bool attached_{}, connected_{}, worn_{}, failStopped_{};
    std::unique_ptr<Buffers> buffers_ = std::make_unique<Buffers>();
    std::atomic<bool> loading_{false}, cancel_{false}, refresh_{true};
    std::atomic<std::shared_ptr<const PolicySnapshot>> policy_;
    std::mutex assignmentMutex_;
    Core::Filter filter_;
    bs::SlotVisualSpec lastVisual_{};
    sdk::InventoryProviderStage lastStage_ = sdk::InventoryProviderStage::Detached;
    Core::Filter Assignment() { std::lock_guard lock(assignmentMutex_); return filter_; }
    static bool Policy(void*, const bs::SourceActionContext&, sdk::OrdinaryItemPlan&);
    bool ReadInstances();
    bool BindBag();
    bool Attach();
    bool Retire();
    void Completed(const sdk::InventoryProviderProgress&);
    std::shared_ptr<PolicySnapshot> Observe(const HIGGS_HandSnapshot&, const bs::RuntimeRecord&);
    void Present(const PolicySnapshot&, const bs::RuntimeRecord&);
};

void Phase() noexcept {
    try { PouchApp::Get().Tick(); }
    catch (const std::exception& e) { PouchApp::Get().Suspend(); spdlog::error("prototype suspended: {}", e.what()); }
    catch (...) { PouchApp::Get().Suspend(); spdlog::error("prototype suspended: unexpected exception"); }
}
bool PouchApp::Policy(void* context, const bs::SourceActionContext& input, sdk::OrdinaryItemPlan& plan) {
    const auto& app = *static_cast<PouchApp*>(context);
    if (app.loading_.load()) return false;
    const auto snapshot = app.policy_.load();
    return snapshot && PlanPotion(*snapshot, input, NowNs(), plan);
}
void PouchApp::Connect() {
    if (connected_) return;
    registry_ = Framework<bs::Api>("BodySlots_QueryInterface");
    instances_ = Framework<bs::InstanceApi>("BodySlots_QueryInstanceInterface");
    equipment_ = Framework<bs::EquipmentApi>("BodySlots_QueryEquipmentInterface");
    runtime_ = Framework<bs::RuntimeApi>("BodySlots_QueryRuntimeInterface");
    behavior_ = Framework<bs::BehaviorApi>("BodySlots_QueryBehaviorInterface");
    game_ = Framework<bs::GameApi>("BodySlots_QueryGameInterface");
    content_ = Framework<bs::ContentObservationApi>("BodySlots_QueryContentObservationInterface");
    if (!registry_ || !instances_ || !equipment_ || !runtime_ || !behavior_ || !game_ || !content_ ||
        !Higgs<HIGGS_QueryHandInterfaceFn>("HIGGS_QueryHandInterface", HIGGS_HAND_ABI_VERSION, tables_.hands) ||
        !Higgs<HIGGS_QueryHandRoutingInterfaceFn>("HIGGS_QueryHandRoutingInterface", HIGGS_HAND_ROUTING_ABI_VERSION, tables_.routing) ||
        !Higgs<HIGGS_QueryInputRoutesInterfaceFn>("HIGGS_QueryInputRoutesInterface", 1, tables_.routes)) {
        spdlog::error("Body Pouches 2 unavailable: compatible Framework/HIGGS services required"); return;
    }
    if (registry_->RegisterClient(kProvider, &writer_) != bs::Status::Ok) return;
    HIGGS_HandClientInfo client{};
    client.structSize = sizeof(client);
    Text(client.name, kProvider);
    if (tables_.hands.RegisterClient(&client, &higgsClient_) != HIGGS_HAND_OK) return;
    bs::Definition definition{};
    definition.structSize = sizeof(definition); definition.flags = bs::kEnabled;
    Text(definition.provider, kProvider); Text(definition.key, kSlot);
    Text(definition.anchor, "NPC Pelvis [Pelv]");
    Text(definition.referenceRig, "skyrimvr-humanoid-v1"); Text(definition.kind, "potion-bag");
    definition.position[0] = 18; definition.position[2] = -4;
    definition.rotation[0] = definition.rotation[4] = definition.rotation[8] = 1;
    definition.radius = 10; definition.handMask = bs::Both; definition.contentMask = bs::Potion;
    bs::SlotRecord record{};
    if (registry_->UpsertDefinition(writer_, &definition, 0, &record) != bs::Status::Ok) return;
    definitionRevision_ = record.definitionRevision;
    tables_.slots = runtime_; tables_.behaviors = behavior_; tables_.game = game_;
    struct Request { void* (*get)(unsigned int){}; } request;
    const auto* bus = SKSE::GetMessagingInterface();
    if (!bus || !bus->Dispatch(0xF9279A57, &request, sizeof(request), "HIGGS") || !request.get) return;
    const auto phase = static_cast<HiggsPluginAPI::IHiggsInterface001*>(request.get(1));
    if (!phase) return;
    phase->AddPostVrikPostHiggsCallback(Phase);
    connected_ = true;
    spdlog::info("Body Pouches 2 connected; no native VRIK holster or controller hooks");
}
bool PouchApp::ReadInstances() {
    view_ = {}; view_.structSize = sizeof(view_); view_.version = 1;
    if (instances_->CopyEffectiveInstances(writer_, 0, 0, buffers_->instances.data(), bs::kMaxInstances, &view_) != bs::Status::Ok)
        return false;
    instance_ = {};
    for (std::uint32_t i = 0; i < view_.count; ++i) {
        const auto& row = buffers_->instances[i];
        if (!strcmp(row.key.ownerProvider, kProvider) && !strcmp(row.key.equipmentKey, kEquipment) && !strcmp(row.key.slotKey, kSlot)) {
            instance_ = row; break;
        }
    }
    return view_.saveGeneration != 0;
}
bool PouchApp::BindBag() {
    if (instance_.instanceHandle) return true;
    bs::EquipmentSlotEntry entry{};
    auto& spec = entry.instance;
    spec.structSize = sizeof(spec); spec.flags = worn_ ? bs::kInstanceActive : 0;
    Text(spec.equipmentKey, kEquipment); Text(spec.slotKey, kSlot);
    Text(spec.definitionProvider, kProvider); Text(spec.definitionKey, kSlot);
    spec.definitionRevision = definitionRevision_;
    spec.binding = static_cast<std::uint32_t>(bs::InstanceBinding::BodyAnchor);
    entry.binding.structSize = sizeof(entry.binding);
    entry.binding.kind = static_cast<std::uint32_t>(bs::RuntimeBindingKind::BodyNode);
    entry.binding.requiredEquipmentFormId = bagForm_;
    bs::InstanceRecord committed{}; committed.structSize = sizeof(committed);
    bs::InstanceView result{}; result.structSize = sizeof(result); result.version = 1;
    if (equipment_->BindSlotSet(writer_, &entry, 1, view_.saveGeneration, view_.viewRevision, &committed, 1, &result) != bs::Status::Ok)
        return false;
    instance_ = committed; view_ = result; lastVisual_ = {};
    return true;
}
bool PouchApp::Attach() {
    if (attached_) return true;
    // Capabilities become available only after HIGGS installs its native backend.
    if (!Higgs<HIGGS_QueryInventoryInterfaceFn>("HIGGS_QueryInventoryInterface", HIGGS_INVENTORY_VERSION, tables_.inventory) ||
        !Higgs<HIGGS_QueryTransactionsInterfaceFn>("HIGGS_QueryTransactionsInterface", HIGGS_TRANSACTIONS_VERSION, tables_.transactions)) return false;
    sdk::InventoryProviderConfig config;
    config.writer = writer_; config.instance = instance_.instanceHandle; config.higgsClient = higgsClient_;
    config.storeEdge = HIGGS_INPUT_UP; config.timeoutMs = 1500;
    config.policy = Policy; config.policyContext = this;
    const auto status = inventory_.Attach(tables_, config);
    attached_ = status == bs::Status::Ok;
    if (attached_) { refresh_ = true; spdlog::info("potion inventory behavior attached to instance {}", instance_.instanceHandle); }
    else if (inventory_.Detach() != bs::Status::Ok) {
        failStopped_ = true;
        spdlog::error("partial behavior registration could not drain; prototype disabled");
    }
    return attached_;
}
bool PouchApp::Retire() {
    policy_.store(nullptr);
    if (!attached_) return true;
    inventory_.Cancel();
    const auto progress = inventory_.Tick();
    if (inventory_.Detach() != bs::Status::Ok) {
        if (progress.stage == sdk::InventoryProviderStage::RecoveryRequired && !failStopped_)
            spdlog::error("unsettled operation retained across lifecycle; no new transfers or automatic refund");
        failStopped_ = progress.stage == sdk::InventoryProviderStage::RecoveryRequired;
        return false;
    }
    attached_ = false; operationScope_ = 0; return true;
}
std::shared_ptr<PolicySnapshot> PouchApp::Observe(const HIGGS_HandSnapshot& hands, const bs::RuntimeRecord& runtime) {
    auto snapshot = std::make_shared<PolicySnapshot>();
    snapshot->epoch = hands.epoch; snapshot->save = view_.saveGeneration; snapshot->instance = instance_.instanceHandle;
    snapshot->filter = Assignment(); snapshot->worn = worn_;
    snapshot->available = runtime.availability == static_cast<std::uint32_t>(bs::RuntimeAvailability::Available);
    auto* player = RE::PlayerCharacter::GetSingleton();
    for (auto& [object, entry] : player->GetInventory([](RE::TESBoundObject& o) { return o.Is(RE::FormType::AlchemyItem); })) {
        auto* potion = object ? object->As<RE::AlchemyItem>() : nullptr;
        if (entry.first <= 0 || !Drinkable(potion)) continue;
        auto candidate = Describe(potion); candidate.item.count = entry.first;
        std::int64_t plain = entry.first;
        std::uint32_t ordinal = 0, firstExtended = 0;
        if (entry.second && entry.second->extraLists) for (const auto* extra : *entry.second->extraLists) {
            if (!extra) continue;
            if (++ordinal > 4096) { plain = -1; break; }
            const auto count = extra->GetCount();
            if (count <= 0) { plain = -1; break; }
            plain -= count;
            if (!firstExtended) firstExtended = ordinal;
        }
        if (plain < 0 || (!plain && !firstExtended)) continue;
        candidate.ordinal = plain > 0 ? 0 : firstExtended;
        snapshot->stock.push_back(std::move(candidate));
    }
    for (std::uint32_t hand = 1; hand <= 2; ++hand) {
        HIGGS_ItemIdentity item{}; item.structSize = sizeof(item);
        if (!tables_.transactions.CopyHeldItem || tables_.transactions.CopyHeldItem(higgsClient_, hand, &item) != HIGGS_HAND_OK ||
            item.epoch != hands.epoch || item.kind != HIGGS_ITEM_WORLD_REFERENCE || !item.worldRefId || !item.quantity ||
            (item.validFields & (HIGGS_ITEM_BASE_KNOWN | HIGGS_ITEM_REFERENCE_KNOWN | HIGGS_ITEM_QUANTITY_KNOWN | HIGGS_ITEM_NATIVE_OBSERVED)) !=
            (HIGGS_ITEM_BASE_KNOWN | HIGGS_ITEM_REFERENCE_KNOWN | HIGGS_ITEM_QUANTITY_KNOWN | HIGGS_ITEM_NATIVE_OBSERVED)) continue;
        auto* potion = RE::TESForm::LookupByID<RE::AlchemyItem>(item.baseFormId);
        if (!Drinkable(potion)) continue;
        snapshot->held[hand - 1] = {Describe(potion), item.worldRefId, item.quantity};
    }
    snapshot->capturedNs = NowNs();
    return snapshot;
}
void PouchApp::Present(const PolicySnapshot& snapshot, const bs::RuntimeRecord& row) {
    const auto* next = NextPotion(snapshot);
    const auto count = MatchingCount(snapshot);
    bs::ContentObservation observation{}; observation.structSize = sizeof(observation);
    observation.epoch = row.epoch; observation.saveGeneration = view_.saveGeneration;
    observation.instanceHandle = instance_.instanceHandle; observation.expectedContentVersion = row.contentVersion;
    observation.contentIdentity = next && count ? next->form : 0; observation.quantity = next ? count : 0;
    std::uint64_t version = 0;
    // Conflict during a transfer is expected. Never overwrite its factual receipt.
    if (content_->Publish(writer_, &observation, &version) != bs::Status::Ok) return;
    bs::SlotVisualSpec visual{}; visual.structSize = sizeof(visual);
    Text(visual.holderModel, "Plants\\CoinBagLarge.nif");
    if (next) Text(visual.previewModel, next->model.c_str());
    visual.contentVersion = version;
    if (memcmp(&visual, &lastVisual_, sizeof(visual)) && runtime_->SetVisual(writer_, instance_.instanceHandle, &visual) == bs::Status::Ok)
        lastVisual_ = visual;
}
void PouchApp::Completed(const sdk::InventoryProviderProgress& progress) {
    if (progress.stage != lastStage_) {
        spdlog::info("inventory stage {}: transfer {}, HIGGS {}, Framework {}", static_cast<unsigned>(progress.stage),
            progress.transfer, progress.handStatus, static_cast<unsigned>(progress.slotStatus));
        lastStage_ = progress.stage;
    }
    if (progress.stage != sdk::InventoryProviderStage::Completed || !progress.transfer ||
        (processedTransfer_ == progress.transfer && processedEpoch_ == progress.receipt.epoch)) return;
    processedTransfer_ = progress.transfer; processedEpoch_ = progress.receipt.epoch; refresh_ = true;
    const auto& receipt = progress.receipt;
    constexpr auto required = HIGGS_TX_DOMAIN_ATTESTED | HIGGS_TX_NATIVE_RELEASE_CONFIRMED;
    if (loading_ || operationScope_ != scope_ || receipt.epoch != epoch_ || receipt.stage != HIGGS_TX_COMPLETED ||
        receipt.disposition != HIGGS_DISPOSITION_INVENTORY || (receipt.receiptFlags & required) != required ||
        (receipt.receiptFlags & (HIGGS_TX_COMPENSATION_CONFIRMED | HIGGS_TX_MUTATION_UNCERTAIN)) ||
        !(receipt.original.validFields & HIGGS_ITEM_BASE_KNOWN)) return;
    auto* potion = RE::TESForm::LookupByID<RE::AlchemyItem>(receipt.original.baseFormId);
    if (!Drinkable(potion)) return;
    const auto assignment = Core::Filter::FromItem(Game::Describe(potion));
    std::lock_guard lock(assignmentMutex_);
    if (!filter_.IsSet()) { filter_ = assignment; spdlog::info("pouch assigned by factual stored potion {:08X}", potion->GetFormID()); }
}
void PouchApp::Tick() {
    if (!connected_) return;
    if (loading_ || cancel_.exchange(false)) {
        Retire(); return;
    }
    HIGGS_HandSnapshot hands{}; hands.structSize = sizeof(hands);
    bs::GameIntegration game{}; game.structSize = sizeof(game);
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->Get3D(false) || tables_.hands.CopySnapshot(higgsClient_, &hands) != HIGGS_HAND_OK ||
        !hands.worldReady || game_->CopyIntegration(&game) != bs::Status::Ok || !game.connected || !ReadInstances()) {
        policy_.store(nullptr);
        if (attached_) { inventory_.Cancel(); inventory_.Tick(); }
        return;
    }
    if (epoch_ != hands.epoch || scope_ != view_.saveGeneration) {
        if (!Retire()) return;
        epoch_ = hands.epoch; scope_ = view_.saveGeneration; refresh_ = true; lastVisual_ = {};
    }
    if (failStopped_) { policy_.store(nullptr); return; }
    if (!bagForm_) {
        auto* data = RE::TESDataHandler::GetSingleton();
        auto* bag = data ? data->LookupForm<RE::TESObjectARMO>(0x803, "BodyPouches.esp") : nullptr;
        if (!bag) { policy_.store(nullptr); return; }
        bagForm_ = bag->GetFormID();
    }
    const auto* worn = player->GetWornArmor(bagForm_, true);
    const bool equipped = worn && worn->GetFormID() == bagForm_;
    const bool changed = worn_ != equipped;
    worn_ = equipped;
    if (!worn_) {
        policy_.store(nullptr);
        if (attached_) inventory_.Cancel();
    }
    if (!BindBag()) return;
    if (((instance_.flags & bs::kInstanceActive) != 0) != worn_) {
        bs::InstanceView result{}; result.structSize = sizeof(result); result.version = 1;
        if (equipment_->SetSlotSetActive(writer_, kEquipment, worn_ ? 1 : 0, view_.saveGeneration, view_.viewRevision, &result) != bs::Status::Ok) return;
        view_ = result;
    }
    if (!Attach()) return;
    const auto before = inventory_.CopyProgress();
    if (before.stage == sdk::InventoryProviderStage::Queued) operationScope_ = scope_;
    const auto progress = inventory_.Tick();
    Completed(progress);
    if (progress.stage == sdk::InventoryProviderStage::RecoveryRequired) { policy_.store(nullptr); return; }
    if (!worn_) return;
    const auto now = NowNs();
    // Refresh metadata/stock at 10 Hz. Geometry and visuals keep following the
    // body at Framework's own rate. Offers expire after 150 ms without a refresh.
    if (!changed && !refresh_.exchange(false) && now - refreshedAt_ < 100000000) return;
    bs::RuntimeView runtimeView{}; runtimeView.structSize = sizeof(runtimeView); runtimeView.version = 1;
    if (runtime_->CopyRuntime(writer_, buffers_->runtime.data(), bs::kMaxInstances, &runtimeView) != bs::Status::Ok ||
        runtimeView.epoch != hands.epoch || runtimeView.saveGeneration != scope_) { policy_.store(nullptr); return; }
    for (std::uint32_t i = 0; i < runtimeView.count; ++i) {
        const auto& row = buffers_->runtime[i];
        if (row.configured.instance.instanceHandle != instance_.instanceHandle) continue;
        auto snapshot = Observe(hands, row);
        Present(*snapshot, row);
        refreshedAt_ = now; policy_.store(std::move(snapshot)); return;
    }
    policy_.store(nullptr);
}
void PouchApp::Save(SKSE::SerializationInterface* skse) {
    const auto filter = Assignment();
    const auto bytes = Core::EncodeAssignments(filter.IsSet() ? Core::Assignments{{1, filter}} : Core::Assignments{});
    if (!skse || !bytes || !skse->WriteRecord(kAssignment, 1, bytes->data(), static_cast<std::uint32_t>(bytes->size())))
        spdlog::error("Body Pouches 2 assignment save failed");
}
void PouchApp::Load(SKSE::SerializationInterface* skse) {
    Revert();
    Core::Assignments saved;
    bool seen = false;
    std::uint32_t type{}, version{}, length{};
    if (!skse) return;
    while (skse->GetNextRecordInfo(type, version, length)) {
        if (type != kAssignment) continue;
        if (seen || version != 1 || length > Core::kAssignmentBytes) return;
        seen = true;
        std::vector<std::uint8_t> bytes(length);
        if (skse->ReadRecordData(bytes.data(), length) != length) return;
        const auto decoded = Core::DecodeAssignments(bytes);
        if (!decoded || decoded->size() > 1 || (!decoded->empty() && decoded->front().slot != 1)) return;
        saved = *decoded;
    }
    auto resolved = Core::ResolveAssignments(saved, [skse](const Core::FormKey& key, bool effect) -> std::optional<Core::FormKey> {
        auto current = key;
        if (key.plugin.empty() && (!skse->ResolveFormID(key.localId, current.localId) || !current.localId)) return std::nullopt;
        auto* form = Game::Lookup(current);
        if (!form || (effect ? !form->As<RE::EffectSetting>() : !Drinkable(form->As<RE::AlchemyItem>()))) return std::nullopt;
        return current;
    });
    std::lock_guard lock(assignmentMutex_);
    filter_ = resolved.empty() ? Core::Filter{} : resolved.front().filter;
}
void Message(SKSE::MessagingInterface::Message* message) noexcept {
    try {
        if (!message) return;
        auto& app = PouchApp::Get();
        switch (message->type) {
        case SKSE::MessagingInterface::kPostPostLoad: app.Connect(); break;
        case SKSE::MessagingInterface::kPreLoadGame: app.Suspend(); break;
        case SKSE::MessagingInterface::kPostLoadGame:
            if (message->dataLen == 1 && reinterpret_cast<std::uintptr_t>(message->data) == 1) app.Resume();
            break;
        case SKSE::MessagingInterface::kNewGame: app.Revert(); app.Resume(); break;
        default: break;
        }
    } catch (...) { PouchApp::Get().Suspend(); }
}
void Save(SKSE::SerializationInterface* skse) noexcept { try { PouchApp::Get().Save(skse); } catch (...) { spdlog::error("assignment save exception"); } }
void Load(SKSE::SerializationInterface* skse) noexcept { try { PouchApp::Get().Load(skse); } catch (...) { PouchApp::Get().Revert(); } }
void Revert(SKSE::SerializationInterface*) noexcept { PouchApp::Get().Revert(); }
}
extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* skse, SKSE::PluginInfo* info) {
    info->infoVersion = SKSE::PluginInfo::kVersion; info->name = "BodyPouches"; info->version = 200;
    return skse && !skse->IsEditor() && skse->RuntimeVersion() == REL::Version{1, 4, 15, 0};
}
extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    if (auto path = SKSE::log::log_directory()) {
        *path /= "BodyPouches.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("BodyPouches2", std::move(sink));
        log->flush_on(spdlog::level::info); spdlog::set_default_logger(std::move(log));
    }
    spdlog::info("Body Pouches 2 prototype: untested Framework consumer");
    auto* serialization = SKSE::GetSerializationInterface();
    const auto* messages = SKSE::GetMessagingInterface();
    if (!serialization || !messages) return false;
    serialization->SetUniqueID(0x42504348); // Existing plugin identity; independent BP2A record.
    serialization->SetSaveCallback(BodyPouches::Prototype::Save);
    serialization->SetLoadCallback(BodyPouches::Prototype::Load);
    serialization->SetRevertCallback(BodyPouches::Prototype::Revert);
    return messages->RegisterListener("SKSE", BodyPouches::Prototype::Message);
}
