// SPDX-License-Identifier: MIT
// Body Pouches 2: gameplay policy only; Framework owns geometry/presentation,
// HIGGS owns input/grants/physical inventory operations. No controller hook.
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "potion_policy.h"
#include "potion_observer.h"
#include "assignment_store.h"
#include "settings.h"
#include "higgsinterface001.h"
#include "body_slots_equipment_api.h"
#include "body_slots_content_observation_api.h"
#include "higgsphysics.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <optional>

namespace BodyPouches::Prototype {
namespace bs = body_slots;
namespace sdk = body_slots::examples;
constexpr char kProvider[] = "BodyPouches2";
constexpr char kEquipment[] = "single-potion-bag";
constexpr char kSlot[] = "potions";
std::uint64_t NowNs() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
template<std::size_t N> void Text(char (&out)[N], const char* value) { strncpy_s(out, value, _TRUNCATE); }
template<class T> const T* Framework(const char* name, std::uint32_t version = 1) {
    const auto module = GetModuleHandleW(L"BodySlots.dll");
    const auto query = module ? reinterpret_cast<const T* (BS_CALL*)(std::uint32_t, std::uint32_t)>(
        GetProcAddress(module, name)) : nullptr;
    const auto* table = query ? query(version, sizeof(T)) : nullptr;
    if (!table) return nullptr;
    if constexpr (requires { table->base; })
        return table->base.structSize == sizeof(T) && table->base.version == version ? table : nullptr;
    else
        return table->structSize == sizeof(T) && table->version == version ? table : nullptr;
}
template<class Query, class T> bool Higgs(const char* name, std::uint32_t version, T& table) {
    const auto module = GetModuleHandleW(L"higgs_vr.dll");
    const auto query = module ? reinterpret_cast<Query>(GetProcAddress(module, name)) : nullptr;
    table = {};
    return query && query(version, sizeof(table), &table) == HIGGS_HAND_OK &&
        table.structSize == sizeof(table) && table.abiVersion == version;
}
struct CapturedAssignment {
    AssignmentSnapshot assignment;
    std::uint64_t save{},epoch{};
    std::uint32_t form{},reference{},quantity{};
};
const CapturedAssignment* StoredAssignment(const sdk::InventoryProviderProgress& progress) noexcept {
    const auto& r=progress.receipt;
    constexpr auto flags=HIGGS_TX_DOMAIN_ATTESTED|HIGGS_TX_NATIVE_RELEASE_CONFIRMED;
    constexpr auto identity=HIGGS_ITEM_BASE_KNOWN|HIGGS_ITEM_REFERENCE_KNOWN|HIGGS_ITEM_QUANTITY_KNOWN;
    if(!progress.semanticContext || !progress.transfer || r.requestId!=progress.transfer ||
       r.stage!=HIGGS_TX_COMPLETED || r.disposition!=HIGGS_DISPOSITION_INVENTORY ||
       (r.receiptFlags&flags)!=flags || (r.receiptFlags&HIGGS_TX_COMPENSATION_CONFIRMED) ||
       sdk::TransferNeedsRecovery(r) || (r.original.validFields&identity)!=identity) return nullptr;
    const auto* captured=static_cast<const CapturedAssignment*>(progress.semanticContext.get());
    return captured->assignment.generation && captured->save==progress.saveGeneration &&
        captured->epoch==r.epoch && captured->form==r.original.baseFormId &&
        captured->reference==r.original.worldRefId && captured->quantity==r.original.quantity &&
        captured->quantity==r.quantity ? captured : nullptr;
}
// One owner for mutable integration state. Callbacks only read the immutable
// policy snapshot; serialization changes only its separately locked assignment.
class PouchApp final {
public:
    static PouchApp& Get() { static PouchApp app; return app; }
    void Connect();
    void Tick();
    void Suspend() noexcept { loading_ = true; policy_.store(nullptr); observer_.Invalidate(); cancel_ = true; }
    void Resume() noexcept { loading_ = false; refresh_ = true; }
    void Save(SKSE::SerializationInterface*);
    void Load(SKSE::SerializationInterface*);
    void Revert() {
        Suspend();
        assignments_.Clear();
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
    HIGGS_HapticsInterface haptics_{};
    sdk::OrdinaryInventoryProvider inventory_;
    std::uint64_t writer_{}, higgsClient_{}, scope_{}, epoch_{}, processedTransfer_{}, processedEpoch_{};
    std::uint64_t definitionRevision_{};
    bs::InstanceRecord instance_{};
    bs::InstanceView view_{};
    std::uint32_t bagForm_{};
    bool attached_{}, connected_{}, worn_{}, failStopped_{};
    std::unique_ptr<Buffers> buffers_ = std::make_unique<Buffers>();
    std::atomic<bool> loading_{false}, cancel_{false}, refresh_{true};
    std::atomic<std::shared_ptr<const PolicySnapshot>> policy_;
    AssignmentStore assignments_;
    PotionObserver observer_;
    PouchSettings settings_;
    std::uint64_t eventSubscription_{};
    struct EntryHint { bs::RuntimeEvent event; std::uint64_t capturedNs; };
    std::array<std::atomic<std::shared_ptr<const EntryHint>>, 2> entries_;
    std::atomic<std::uint64_t> feedbackInstance_{0};
    std::array<std::uint64_t, 2> hapticRequests_{};
    bs::SlotVisualSpec lastVisual_{};
    sdk::InventoryProviderStage lastStage_ = sdk::InventoryProviderStage::Detached;
    static bool Policy(void*, const bs::SourceActionContext&, sdk::OrdinaryItemPlan&);
    static void BS_CALL Event(void*, const bs::RuntimeEvent*) noexcept;
    void Feedback(const bs::RuntimeRecord&);
    void CancelFeedback();
    bool ReadInstances();
    bool BindBag();
    bool Attach();
    bool Retire();
    bs::Status Completed(const sdk::InventoryProviderProgress&);
    static bs::Status Settle(void* context,const sdk::InventoryProviderProgress& outcome) {
        return static_cast<PouchApp*>(context)->Completed(outcome);
    }
    std::shared_ptr<PolicySnapshot> Observe(const HIGGS_HandSnapshot&, const bs::RuntimeRecord&,
        const std::array<HIGGS_ItemIdentity, 2>&);
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
    if(!snapshot || !PlanPotion(*snapshot, input, NowNs(), plan)) return false;
    if(plan.transferKind==HIGGS_TRANSFER_HAND_TO_INVENTORY){
        plan.semanticContext=snapshot->storeContexts[input.context.input.physicalHand-1];
        if(!plan.semanticContext)return false;
    }
    return true;
}
void PouchApp::Connect() {
    if (connected_) return;
    settings_ = LoadPouchSettings();
    spdlog::set_level(spdlog::level::from_str(settings_.logLevel));
    if (!settings_.enabled) { spdlog::info("Body Pouches 2 disabled in configuration"); return; }
    registry_ = Framework<bs::Api>("BodySlots_QueryInterface");
    instances_ = Framework<bs::InstanceApi>("BodySlots_QueryInstanceInterface");
    equipment_ = Framework<bs::EquipmentApi>("BodySlots_QueryEquipmentInterface");
    runtime_ = Framework<bs::RuntimeApi>("BodySlots_QueryRuntimeInterface");
    behavior_ = Framework<bs::BehaviorApi>("BodySlots_QueryBehaviorInterface");
    game_ = Framework<bs::GameApi>("BodySlots_QueryGameInterface");
    tables_.behaviorOptions = Framework<bs::BehaviorApi2>("BodySlots_QueryBehaviorInterface", 2);
    tables_.gameServices = Framework<bs::GameApi2>("BodySlots_QueryGameInterface", 2);
    tables_.frameObservations = Framework<bs::GameApi3>("BodySlots_QueryGameInterface", 3);
    content_ = Framework<bs::ContentObservationApi>("BodySlots_QueryContentObservationInterface");
    if (!registry_ || !instances_ || !equipment_ || !runtime_ || !behavior_ || !game_ || !content_ ||
        !tables_.behaviorOptions || !tables_.gameServices || !tables_.frameObservations ||
        !Higgs<HIGGS_QueryHandInterfaceFn>("HIGGS_QueryHandInterface", HIGGS_HAND_ABI_VERSION, tables_.hands) ||
        !Higgs<HIGGS_QueryHandRoutingInterfaceFn>("HIGGS_QueryHandRoutingInterface", HIGGS_HAND_ROUTING_ABI_VERSION, tables_.routing) ||
        !Higgs<HIGGS_QueryInputRoutesInterfaceFn>("HIGGS_QueryInputRoutesInterface", 1, tables_.routes) ||
        !Higgs<HIGGS_QueryHeldReleaseInterfaceFn>("HIGGS_QueryHeldReleaseInterface", HIGGS_HELD_RELEASE_ABI_VERSION, tables_.releases) ||
        !Higgs<HIGGS_QueryAuthorizedDispatchInterfaceFn>("HIGGS_QueryAuthorizedDispatchInterface", 2, tables_.dispatch)) {
        spdlog::error("Body Pouches 2 unavailable: compatible Framework/HIGGS services required"); return;
    }
    if(!(tables_.behaviorOptions->base.capabilities&bs::kBehaviorExtendedOptions)||
       !(tables_.hands.capabilities&HIGGS_HAND_CAP_ACCEPTED_RECEIPTS)){
        spdlog::error("Body Pouches 2 requires Framework inventory-content contracts and HIGGS queued-result retention");return;
    }
    if (registry_->RegisterClient(kProvider, &writer_) != bs::Status::Ok) return;
    HIGGS_HandClientInfo client{};
    client.structSize = sizeof(client);
    Text(client.name, kProvider);
    if (tables_.hands.RegisterClient(&client, &higgsClient_) != HIGGS_HAND_OK) return;
    // Optional coordinated feedback. Missing hardware/service never blocks use.
    Higgs<HIGGS_QueryHapticsInterfaceFn>("HIGGS_QueryHapticsInterface", 1, haptics_);
    bs::Definition definition{};
    definition.structSize = sizeof(definition); definition.flags = bs::kEnabled;
    Text(definition.provider, kProvider); Text(definition.key, kSlot);
    Text(definition.anchor, settings_.anchor.c_str());
    Text(definition.referenceRig, "skyrimvr-humanoid-v1"); Text(definition.kind, "potion-bag");
    std::copy(settings_.position.begin(), settings_.position.end(), definition.position);
    std::copy(settings_.rotation.begin(), settings_.rotation.end(), definition.rotation);
    definition.radius = settings_.radius; definition.handMask = bs::Both; definition.contentMask = bs::Potion;
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
    if (!observer_.ConnectEvents())
        spdlog::warn("inventory events unavailable; using bounded stock refresh fallback");
    if (runtime_->Subscribe(writer_, Event, this, &eventSubscription_) != bs::Status::Ok)
        spdlog::warn("slot entry feedback unavailable");
    connected_ = true;
    spdlog::info("Body Pouches 2 connected; no native VRIK holster or controller hooks");
}
void BS_CALL PouchApp::Event(void* context, const bs::RuntimeEvent* event) noexcept {
    if (!context || !event || event->kind != static_cast<std::uint32_t>(bs::RuntimeEventKind::Enter) ||
        (event->physicalHand != bs::Left && event->physicalHand != bs::Right)) return;
    auto& app = *static_cast<PouchApp*>(context);
    // Retain copied scope, including an entry before the first policy snapshot.
    // No live integration state is read on the notifying thread.
    if (!app.loading_.load() && event->instanceHandle == app.feedbackInstance_.load()) {
        try { app.entries_[event->physicalHand - 1].store(std::make_shared<EntryHint>(EntryHint{*event, NowNs()})); }
        catch (...) {} // Feedback allocation failure cannot escape a provider callback.
    }
}
void PouchApp::Feedback(const bs::RuntimeRecord& row) {
    for (std::uint32_t hand = bs::Left; hand <= bs::Right; ++hand) {
        const auto entry = entries_[hand - 1].exchange(nullptr);
        if (!entry || !settings_.entryHaptics || !haptics_.RequestHaptic || !worn_ || loading_ ||
            row.availability != static_cast<std::uint32_t>(bs::RuntimeAvailability::Available) ||
            entry->event.epoch != row.epoch || entry->event.instanceHandle != instance_.instanceHandle ||
            !(row.insideHands & hand) || row.frame < entry->event.frame ||
            NowNs() < entry->capturedNs || NowNs() - entry->capturedNs > 150000000) continue;
        HIGGS_HapticRequest request{};
        request.structSize = sizeof(request); request.physicalHand = hand;
        request.epoch = row.epoch; request.contextKey = instance_.instanceHandle;
        request.durationMs = 30; request.startStrength = 0.2f;
        std::uint64_t id{};
        if (haptics_.RequestHaptic(higgsClient_, &request, &id) == HIGGS_HAND_OK)
            hapticRequests_[hand - 1] = id;
    }
}
void PouchApp::CancelFeedback() {
    feedbackInstance_.store(0);
    for (auto& entry : entries_) entry.store(nullptr);
    for (auto& id : hapticRequests_) {
        if (id && haptics_.CancelHaptic) haptics_.CancelHaptic(higgsClient_, id);
        id = 0;
    }
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
    config.storeOnHeldRelease = true; config.withdrawOnExit = true; config.timeoutMs = 1500;
    config.policy = Policy; config.policyContext = this;
    config.settle = Settle; config.settlementContext = this;
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
    CancelFeedback();
    if (!attached_) return true;
    inventory_.Cancel();
    const auto progress = inventory_.Tick();
    Completed(progress);
    if (inventory_.Detach() != bs::Status::Ok) {
        if (progress.stage == sdk::InventoryProviderStage::RecoveryRequired && !failStopped_)
            spdlog::error("unsettled operation retained across lifecycle; no new transfers or automatic refund");
        failStopped_ = progress.stage == sdk::InventoryProviderStage::RecoveryRequired;
        return false;
    }
    attached_ = false; failStopped_ = false; return true;
}
std::shared_ptr<PolicySnapshot> PouchApp::Observe(const HIGGS_HandSnapshot& hands,
    const bs::RuntimeRecord& runtime, const std::array<HIGGS_ItemIdentity, 2>& held) {
    auto snapshot = std::make_shared<PolicySnapshot>();
    snapshot->epoch = hands.epoch; snapshot->save = view_.saveGeneration; snapshot->instance = instance_.instanceHandle;
    auto assignment=assignments_.Snapshot();
    snapshot->filter=std::move(assignment.filter);snapshot->assignmentGeneration=assignment.generation;
    snapshot->worn = worn_;
    snapshot->available = runtime.availability == static_cast<std::uint32_t>(bs::RuntimeAvailability::Available);
    const auto now = NowNs();
    snapshot->stock = observer_.CopyStock(now, refresh_.exchange(false));
    snapshot->held = observer_.CopyHeld(held, hands.epoch, now);
    for(std::size_t hand=0;hand<snapshot->held.size();++hand){
        const auto& item=snapshot->held[hand];
        if(!snapshot->assignmentGeneration || !item.reference ||
           (!snapshot->filter.IsSet()&&!item.assignment)) continue;
        auto captured=std::make_shared<CapturedAssignment>();
        captured->assignment={item.assignment.value_or(Core::Filter{}),snapshot->assignmentGeneration};
        captured->save=snapshot->save;captured->epoch=snapshot->epoch;
        captured->form=item.potion.form;captured->reference=item.reference;captured->quantity=item.quantity;
        snapshot->storeContexts[hand]=std::move(captured);
    }
    snapshot->capturedNs = NowNs();
    return snapshot;
}
void PouchApp::Present(const PolicySnapshot& snapshot, const bs::RuntimeRecord& row) {
    if (!snapshot.stock) return; // Missing observation is not evidence of empty stock.
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
bs::Status PouchApp::Completed(const sdk::InventoryProviderProgress& progress) {
    if (progress.stage != lastStage_) {
        spdlog::info("inventory stage {}: transfer {}, HIGGS {}, Framework {}, consumer {}", static_cast<unsigned>(progress.stage),
            progress.transfer, progress.handStatus, static_cast<unsigned>(progress.slotStatus),static_cast<unsigned>(progress.settlementStatus));
        lastStage_ = progress.stage;
    }
    if (progress.stage != sdk::InventoryProviderStage::Completed || !progress.transfer ||
        (processedTransfer_ == progress.transfer && processedEpoch_ == progress.receipt.epoch)) return bs::Status::Ok;
    // Keep offers closed until this safe phase publishes policy containing the
    // settled assignment. Clearing helper work must not expose the old filter.
    policy_.store(nullptr);
    const auto& receipt = progress.receipt;
    constexpr auto required = HIGGS_TX_DOMAIN_ATTESTED | HIGGS_TX_NATIVE_RELEASE_CONFIRMED;
    const bool storage=receipt.stage==HIGGS_TX_COMPLETED&&receipt.disposition==HIGGS_DISPOSITION_INVENTORY&&
        (receipt.receiptFlags&required)==required&&
        !(receipt.receiptFlags&HIGGS_TX_COMPENSATION_CONFIRMED)&&!sdk::TransferNeedsRecovery(receipt)&&
        (receipt.original.validFields&HIGGS_ITEM_BASE_KNOWN);
    if(storage){
        const auto* captured=StoredAssignment(progress);
        if(!captured)return bs::Status::Conflict;
        // Revert/Load retires the consumer generation under the same lock used
        // for assignment and saving. A physical epoch change does not retire it.
        if(loading_ && assignments_.Snapshot().generation==captured->assignment.generation)
            return bs::Status::Unavailable;
        const std::optional<Core::Filter> filter=captured->assignment.filter.IsSet()
            ? std::optional<Core::Filter>(captured->assignment.filter) : std::nullopt;
        const auto applied=assignments_.ApplyCaptured(filter,captured->assignment.generation);
        if(applied==AssignmentApplyResult::Invalid)return bs::Status::Conflict;
        if(applied==AssignmentApplyResult::Applied)
            spdlog::info("pouch assigned by factual stored potion {:08X}",captured->form);
    }
    processedTransfer_ = progress.transfer; processedEpoch_ = progress.receipt.epoch; refresh_ = true;
    if (progress.releaseStatus != HIGGS_HAND_OK)
        spdlog::warn("delivered transfer {} settled; original grip continuation unavailable ({})",
            progress.transfer, progress.releaseStatus);
    return bs::Status::Ok;
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
        if (attached_) { inventory_.Cancel(); Completed(inventory_.Tick()); }
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
    if (worn_ != equipped) refresh_ = true;
    worn_ = equipped;
    if (!worn_) {
        policy_.store(nullptr);
        CancelFeedback();
        if (attached_) inventory_.Cancel();
    }
    if (!BindBag()) return;
    if (((instance_.flags & bs::kInstanceActive) != 0) != worn_) {
        bs::InstanceView result{}; result.structSize = sizeof(result); result.version = 1;
        if (equipment_->SetSlotSetActive(writer_, kEquipment, worn_ ? 1 : 0, view_.saveGeneration, view_.viewRevision, &result) != bs::Status::Ok) return;
        view_ = result;
    }
    if (!Attach()) return;
    feedbackInstance_.store(worn_ && settings_.entryHaptics ? instance_.instanceHandle : 0);
    const auto progress = inventory_.Tick();
    Completed(progress);
    if (progress.stage == sdk::InventoryProviderStage::RecoveryRequired ||
        progress.stage == sdk::InventoryProviderStage::Settling) { policy_.store(nullptr); return; }
    if (!worn_) return;
    // Eligibility follows every safe Framework frame, independently of stock.
    // Reuse immutable inventory observations; changed held identities are cheap
    // copied HIGGS facts and are described without enumerating inventory again.
    std::array<HIGGS_ItemIdentity, 2> heldNow{};
    for (std::uint32_t hand = 1; hand <= 2; ++hand) {
        HIGGS_ItemIdentity item{}; item.structSize = sizeof(item);
        if (tables_.transactions.CopyHeldItem(higgsClient_, hand, &item) == HIGGS_HAND_OK && item.epoch == hands.epoch)
            heldNow[hand - 1] = item;
    }
    bs::RuntimeView runtimeView{}; runtimeView.structSize = sizeof(runtimeView); runtimeView.version = 1;
    if (runtime_->CopyRuntime(writer_, buffers_->runtime.data(), bs::kMaxInstances, &runtimeView) != bs::Status::Ok ||
        runtimeView.epoch != hands.epoch || runtimeView.saveGeneration != scope_) { policy_.store(nullptr); return; }
    for (std::uint32_t i = 0; i < runtimeView.count; ++i) {
        const auto& row = buffers_->runtime[i];
        if (row.configured.instance.instanceHandle != instance_.instanceHandle) continue;
        Feedback(row);
        auto snapshot = Observe(hands, row, heldNow);
        Present(*snapshot, row);
        policy_.store(std::move(snapshot)); return;
    }
    policy_.store(nullptr);
}
void PouchApp::Save(SKSE::SerializationInterface* skse) {
    // Copy-only projection: never run provider dispatch/inventory work or change
    // live policy from a serialization callback. Unknown outcomes grant nothing.
    const auto progress=inventory_.CopyProgress();
    const auto* captured=StoredAssignment(progress);
    if (!assignments_.Save(skse,captured?&captured->assignment:nullptr))
        spdlog::error("Body Pouches 2 assignment save failed");
}
void PouchApp::Load(SKSE::SerializationInterface* skse) {
    Revert();
    const auto result = assignments_.Load(skse, settings_.importLegacySlot);
    if (result == AssignmentLoadResult::Imported)
        spdlog::info("imported released Body Pouches assignment from native slot {}", settings_.importLegacySlot);
    else if (result == AssignmentLoadResult::Invalid)
        spdlog::warn("invalid or missing assignment forms; bag left unassigned without fallback");
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
