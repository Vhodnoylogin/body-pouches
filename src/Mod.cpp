#include "Mod.h"

#include "Loc.h"
#include "Paths.h"
#include "core/SlotPlan.h"
#include "game/Body.h"
#include "game/Forms.h"
#include "vr/Controller.h"

#include <SKSE/SKSE.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <span>
#include <string>

namespace BodyPouches
{
	namespace
	{
		const char* HandName(bool a_isLeft) { return a_isLeft ? "left" : "right"; }

		// Milliseconds since the mod started. Only differences are ever used.
		std::int64_t Now()
		{
			using namespace std::chrono;
			static const auto start = steady_clock::now();
			return duration_cast<milliseconds>(steady_clock::now() - start).count();
		}

		// Where a bottle is put so that the hand can close on it. The first node that
		// exists wins; the VR wand nodes are tried first because in VR they are where
		// the controller actually is, and the skeleton hands follow them with a lag.
		constexpr std::array kLeftNodes{ "LeftWandNode", "NPC L Hand [LHnd]", "NPC L Finger02 [LF02]" };
		constexpr std::array kRightNodes{ "RightWandNode", "NPC R Hand [RHnd]", "NPC R Finger02 [RF02]" };

		// How often the pouch is asked what it should show. It costs a walk of the
		// inventory; a bottle drawn or put away asks at once, so this only has to catch
		// what changes by other means - a potion bought, found, or drunk from the menu.
		constexpr std::int64_t kDisplayEvery = 1000;

		// How long to give HIGGS before asking whether the hand really closed on the
		// bottle. Long enough for a frame or two, short enough to be about that bottle.
		constexpr std::int64_t kGrabCheckMs = 200;

		// The grip, as the VR controllers number their buttons. Shared draws also
		// observe its real edge; exclusive draws remain VRIK's callback.
		constexpr int kGripButton = 2;

		constexpr auto kHeadBone = "NPC Head [Head]";
	}

	Mod& Mod::GetSingleton()
	{
		static Mod instance;
		return instance;
	}

	void Mod::Connect()
	{
		const bool vrik = _vrik.Connect();
		const bool higgs = _higgs.Connect();
		if (!vrik || !higgs) {
			// Both are needed and neither can be replaced: VRIK owns the places on the
			// body, HIGGS owns the hands. Without either, the mod does nothing at all
			// rather than half of something. Said out loud, because from here on the log
			// falls silent and silence on its own explains nothing.
			Loc::Warn(Keys::kConnectGaveUp, vrik, higgs);
			return;
		}

		if (!_vrik.Subscribe(&Mod::OnHolsterAttempt)) {
			Loc::Error(Keys::kVrikNoCallback);
			return;
		}

		Loc::Info(Keys::kVrikSubscribed);

		_higgs.OnConsumed(&Mod::OnConsumed);
		_higgs.OnStashed(&Mod::OnStashed);
		_higgs.OnDropped(&Mod::OnDropped);
		_higgs.OnGrabbed(&Mod::OnGrabbed);
		Loc::Info(Keys::kHiggsSubscribed);

		// The frame, and with it the end of this mod having a thread of its own.
		_higgs.OnBeforeFrame(&Mod::OnBeforeFrame);
		_higgs.OnFrame(&Mod::OnFrame);
	}

	void Mod::OnDataLoaded()
	{
		_settings = LoadSettings();
		Loc::SetLevel(_settings.logLevel);
		Loc::Load(Paths::LangDir(), _settings.language);
		BuildPouches(_settings);
		WatchInput();

		// The picture. Our plugin has one art, so one pouch can show what it holds: the
		// first one a slot was given over to. A shared slot goes on showing whatever
		// VRIK keeps there - painting a bottle over the player's own sword would take
		// away something they put there themselves.
		for (const auto& setting : _settings.pouches) {
			if (Settings::ModeFromText(setting.mode) != Core::Mode::Exclusive) {
				continue;
			}
			if (_picture.Slot() != 0) {
				Loc::Info(Keys::kPictureOnlyOne, setting.slot, _picture.Slot());
				continue;
			}
			if (_picture.Load(setting.slot) && Working()) {
				// Once for the whole game: VRIK keeps this in its DLL's memory, and after
				// a load its scripts give it back only their own arts, never anyone else's.
				_vrik.SetArt(setting.slot, _picture.Art());
				Loc::Info(Keys::kPictureGiven, setting.slot);
			}
		}
		_dataLoaded = true;
	}

	void Mod::OnBeforeFrame()
	{
		auto& mod = GetSingleton();
		// A missing post callback cannot carry an old release into another update.
		mod.CancelSharedDrop(true);
		mod.CancelSharedDrop(false);
		++mod._higgsUpdate;
		mod._inHiggsUpdate = true;
	}

	void Mod::OnFrame()
	{
		auto& mod = GetSingleton();
		if (!mod.Working() || mod._loading) {
			mod.CancelSharedDrop(true);
			mod.CancelSharedDrop(false);
			mod._sharedPresses[0].Clear();
			mod._sharedPresses[1].Clear();
			mod._sharedButtons[0].Clear();
			mod._sharedButtons[1].Clear();
			mod._inHiggsUpdate = false;
			return;
		}
		++mod._callbackFrame;
		mod._frameGeneration = mod._worldGeneration;

		if (!mod._frameSeen) {
			mod._frameSeen = true;
			mod.FirstFrame();
		}

		mod.ResolveSharedDrops();
		mod.PollSharedButtons();
		mod.ResolveSharedPresses();
		mod._inHiggsUpdate = false;
		mod.NoteReach();
		mod.UpdateSharedWithdrawals();
		mod.RefreshDisplay();
	}

	Diagnostics::State Mod::ReadState()
	{
		Diagnostics::State result;
		std::scoped_lock guard(_lock);
		result.loadGeneration = _worldGeneration;
		result.callbackFrame = _callbackFrame;
		if (!_dataLoaded || !Working()) {
			return result;
		}
		if (_loading || !_frameSeen || _frameGeneration != _worldGeneration) {
			result.reason = "loading_or_no_callback_in_current_generation";
			return result;
		}
		const auto* player = RE::PlayerCharacter::GetSingleton();
		const auto* cell = player != nullptr ? player->GetParentCell() : nullptr;
		auto* ui = RE::UI::GetSingleton();
		if (player == nullptr || player->Get3D() == nullptr || cell == nullptr || !cell->IsAttached() ||
			ui == nullptr || ui->GameIsPaused() || ui->IsMenuOpen("Main Menu") || ui->IsMenuOpen("Loading Menu")) {
			result.reason = "gameplay_world_unavailable";
			return result;
		}
		const auto* editorId = cell->GetFormEditorID();
		if (editorId == nullptr || std::string_view(editorId) == "VRPlayroom01" ||
			ui->IsMenuOpen("Fader Menu") || ui->IsMenuOpen("RaceSex Menu")) {
			result.reason = "startup_world_or_transition";
			return result;
		}
		if (_settings.language.size() > 128 || Loc::Language().size() > 128) {
			result.reason = "state_string_exceeds_128_bytes";
			return result;
		}
		result.settings.mayEnableSlots = _settings.mayEnableSlots;
		// Report applied localization/logger state, including their fallback behavior.
		result.settings.language = Loc::Language();
		const auto level = spdlog::level::to_string_view(spdlog::get_level());
		result.settings.logLevel = std::string(level.data(), level.size());
		result.pouches = _pouches.Snapshot();
		result.bodySlots = Diagnostics::ReadBodySlots(_vrik.Ready(),
			[this](int a_slot) { return _vrik.IsSuspended(a_slot); });
		if (const auto* setting = RE::GetINISetting("bLeftHandedMode:VRInput"); setting != nullptr) {
			result.inputLeftHanded = setting->GetBool();
		}
		result.available = true;
		result.reason.clear();
		for (const bool isLeft : { true, false }) {
			const bool secondary = IsSecondaryHand(isLeft);
			const int shared = SharedCandidate(isLeft);
			result.handReach.push_back({ isLeft, secondary, _vrik.SlotInReach(secondary),
				_vrik.CanBeHolstered(secondary), shared, SharedDistance(shared, isLeft),
				SharedRadius(), _sharedWithdrawal[isLeft ? 0 : 1].Slot() });
		}
		return result;
	}

	void Mod::FirstFrame()
	{
		Loc::Info(Keys::kFrameAlive);

		// A new game raises no message this plugin ever receives: run 7 started one and
		// the slot stayed dead for twelve minutes, until the first reach VRIK let through.
		// Suspension lasts until the game is closed, so once, here, is enough.
		if (!_slotsArranged) {
			Loc::Info(Keys::kSlotsFirstFrame);
			ApplySlots();
		}

		DescribeSlots();
		_picture.Reset(Now());
	}

	void Mod::NoteReach()
	{
		const auto now = Now();

		for (int hand = 0; hand < 2; ++hand) {
			const bool secondary = hand == 1;

			// A bottle handed to this hand and never taken. The other ending - the hand did
			// close on it - arrives as an event of its own and clears this, so what is left
			// here when the time is up is a failure and nothing else.
			if (_pendingGrab[hand].Expired(now)) {
				_pendingGrab[hand].Clear();
				_grabReference[hand].reset();
				Loc::Warn(Keys::kGrabFailed, HandName(IsLeftHand(secondary)), _drawnFrom[hand]);
			}

			const int slot = _vrik.SlotInReach(secondary);
			if (slot == _reach[hand]) {
				continue;  // only changes are worth a line; this runs every frame
			}
			_reach[hand] = slot;
			Loc::Info(Keys::kReachChanged, HandName(IsLeftHand(secondary)), slot,
				_vrik.CanBeHolstered(secondary));
		}
	}

	void Mod::RefreshDisplay()
	{
		if (!_picture.Ready()) {
			return;
		}

		const auto now = Now();
		const int  slot = _picture.Slot();

		if (now >= _displayAt) {
			_displayAt = now + kDisplayEvery;

			// The picture says "there is a bottle here to take", so it is only ever what
			// the pack really holds for this pouch - up while there is one, down the moment
			// there is not, and nothing at all for a pouch nobody has set up yet.
			std::string model;
			std::string what;
			{
				std::scoped_lock guard(_lock);
				const auto shown = _pouches.Display(slot, _pack);
				auto*      form = shown.anything ? Game::Lookup(shown.item) : nullptr;
				if (auto* potion = form != nullptr ? form->As<RE::AlchemyItem>() : nullptr; potion != nullptr) {
					const char* path = potion->GetModel();
					const char* name = potion->GetName();
					model = path != nullptr ? path : "";
					what = name != nullptr ? name : "";
				}
			}
			_picture.Want(model, what);
		}

		_picture.Tick(now, _vrik.IsDisplayed(slot));
	}

	int Mod::PouchAt(bool a_isLeft, int a_nativeSlot, bool a_sharedPhase)
	{
		const int slot = a_nativeSlot;
		const auto phase = a_sharedPhase ? "post_vrik_same_update" : "higgs_dropped_native";
		if (slot == 0) {
			// The pre-VRIK hand/body transforms are not a coherent shared-slot test.
			// Read them for diagnosis only; decide from the same update's post hook.
			const int shared = a_sharedPhase ? SharedCandidate(a_isLeft) : 0;
			Loc::Info(Keys::kDropReachProbe, HandName(a_isLeft), slot, shared, phase);
			for (const auto& pouch : _pouches.Snapshot()) {
				if (pouch.mode != Core::Mode::Shared) { continue; }
				std::string_view reason;
				const auto distance = SharedDistance(pouch.slot, a_isLeft, &reason);
				Loc::Info(Keys::kDropSharedProbe, HandName(a_isLeft), pouch.slot,
					distance.value_or(-1), distance.has_value(), SharedRadius(), reason, phase);
			}
			if (shared != 0) {
				const auto distance = SharedDistance(shared, a_isLeft);
				if (distance) { Loc::Info(Keys::kSharedDrop, shared, HandName(a_isLeft), *distance, SharedRadius()); }
			}
			return shared;
		}
		std::scoped_lock guard(_lock);
		const int chosen = _pouches.Find(slot) != nullptr ? slot : 0;
		Loc::Info(Keys::kDropReachProbe, HandName(a_isLeft), slot, chosen, phase);
		return chosen;
	}

	void Mod::CancelSharedDrop(bool a_isLeft)
	{
		auto& release = _sharedReleases[a_isLeft ? 0 : 1];
		if (release.event.Active()) {
			const auto pending = _queuedDrops.find(release.handle.native_handle());
			if (pending != _queuedDrops.end() && pending->second == release.serial) {
				_queuedDrops.erase(pending);
			}
		}
		release = {};
	}

	void Mod::SettleOutsideDrop(bool a_isLeft)
	{
		std::scoped_lock guard(_lock);
		if (const auto slot = _pouches.SlotOfHand(a_isLeft); slot.has_value()) {
			Loc::Info(Keys::kPouchLost, *slot);
			_pouches.NoteSettled(a_isLeft);
		}
	}

	void Mod::ResolveSharedDrops()
	{
		for (const bool isLeft : { true, false }) {
			auto& stored = _sharedReleases[isLeft ? 0 : 1];
			if (!stored.event.Active()) { continue; }
			const auto release = stored;
			const bool current = _inHiggsUpdate && stored.event.Take(_worldGeneration, _higgsUpdate);
			stored = {};
			const auto pending = _queuedDrops.find(release.handle.native_handle());
			if (pending == _queuedDrops.end() || pending->second != release.serial) { continue; }
			if (!current) {
				_queuedDrops.erase(pending);
				continue;
			}
			// A native offer that appears after VRIK owns arbitration. Never redirect
			// a deferred shared release into an exclusive or somebody else's slot.
			const int native = _vrik.SlotInReach(IsSecondaryHand(isLeft));
			if (native != 0) {
				Loc::Info(Keys::kDropReachProbe, HandName(isLeft), native, 0, "post_vrik_native_priority");
			}
			const int slot = native == 0 ? PouchAt(isLeft, native, true) : 0;
			Loc::Info(Keys::kSharedResolved, HandName(isLeft), release.handle.native_handle(),
				_higgsUpdate, _worldGeneration, slot);
			if (slot != 0) {
				QueueStowDropped(slot, isLeft, release.handle, release.serial);
			} else {
				_queuedDrops.erase(pending);
				SettleOutsideDrop(isLeft);
			}
		}
	}

	void Mod::QueueStowDropped(int a_slot, bool a_isLeft, RE::ObjectRefHandle a_handle, std::uint64_t a_serial)
	{
		if (auto* tasks = SKSE::GetTaskInterface(); tasks != nullptr) {
			tasks->AddTask([a_slot, a_isLeft, a_handle, a_serial, generation = _worldGeneration]() {
				auto& current = GetSingleton();
				if (!current._loading && generation == current._worldGeneration) {
					const auto pending = current._queuedDrops.find(a_handle.native_handle());
					if (pending == current._queuedDrops.end() || pending->second != a_serial) { return; }
					current._queuedDrops.erase(pending);
					const auto object = a_handle.get();
					current.StowDropped(a_slot, a_isLeft, object.get());
				}
			});
		} else {
			_queuedDrops.erase(a_handle.native_handle());
		}
	}

	bool Mod::SharedWorldReady(std::string_view* a_reason)
	{
		const auto fail = [a_reason](std::string_view reason) {
			if (a_reason) { *a_reason = reason; }
			return false;
		};
		if (a_reason) { *a_reason = "ready"; }
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* ui = RE::UI::GetSingleton();
		const auto* cell = player != nullptr ? player->GetParentCell() : nullptr;
		if (_loading || !_frameSeen || _frameGeneration != _worldGeneration) { return fail("loading_or_callback_generation"); }
		if (player == nullptr) { return fail("player_missing"); }
		if (player->Get3D(true) == nullptr || player->Get3D(false) == nullptr) { return fail("player_3d_missing"); }
		if (player->AsActorState()->IsSwimming()) { return fail("swimming"); }
		if (player->IsOnMount()) { return fail("mounted"); }
		if (player->AsActorState()->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal) { return fail("sitting_or_sleeping"); }
		if (cell == nullptr || !cell->IsAttached()) { return fail("cell_unattached"); }
		if (ui == nullptr || ui->GameIsPaused()) { return fail("ui_missing_or_paused"); }
		if (ui->IsMenuOpen("Main Menu") || ui->IsMenuOpen("Loading Menu") || ui->IsMenuOpen("Fader Menu") ||
			ui->IsMenuOpen("RaceSex Menu")) { return fail("transition_menu"); }
		const auto* id = cell->GetFormEditorID();
		if (id == nullptr || std::string_view(id) == "VRPlayroom01") { return fail("startup_cell"); }
		if (!(_vrik.GetSetting("enableHolsters") > .5)) { return fail("holsters_disabled"); }
		if (!(_vrik.GetSetting("displayHolsters") > .5)) { return fail("holsters_display_disabled"); }
		return true;
	}

	double Mod::SharedRadius()
	{
		const double radius = _vrik.GetSetting("slotActivationDistance") * _vrik.GetSetting("bodySize");
		return std::isfinite(radius) && radius > 0 && radius < 1000 ? radius : 0;
	}

	std::optional<double> Mod::SharedDistance(int a_slot, bool a_isLeft, std::string_view* a_reason)
	{
		const auto fail = [a_reason](std::string_view reason) -> std::optional<double> {
			if (a_reason) { *a_reason = reason; }
			return std::nullopt;
		};
		if (!SharedWorldReady(a_reason)) { return std::nullopt; }
		const auto* pouch = _pouches.Find(a_slot);
		if (pouch == nullptr || pouch->PouchMode() != Core::Mode::Shared) { return fail("not_shared_pouch"); }
		if (!_vrik.IsDetectable(a_slot)) { return fail("slot_not_detectable"); }
		if (_vrik.GetSetting(std::format("visibleSlot{}", a_slot).c_str()) <= .5) { return fail("slot_not_visible"); }
		const double allowedHand = _vrik.GetSetting(std::format("handSlot{}", a_slot).c_str());
		if (!std::isfinite(allowedHand) || (allowedHand != 0 && allowedHand != 1 && allowedHand != 2) ||
			(a_isLeft ? allowedHand == 2 : allowedHand == 1)) { return fail("hand_policy"); }
		const auto bone = Game::Body::Bone(a_slot);
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* root = player->Get3D(true);
		auto* hand = root->GetObjectByName(a_isLeft ? "NPC L Hand [LHnd]" : "NPC R Hand [RHnd]");
		if (!bone || !std::isfinite(bone->scale) || bone->scale <= 0) { return fail("slot_bone_unavailable"); }
		if (hand == nullptr) { return fail("hand_node_unavailable"); }
		const auto centre = Game::Body::ToWorld(*bone, _vrik.SlotPosition(a_slot));
		const double distance = hand->world.translate.GetDistance(centre);
		if (!std::isfinite(distance) || distance < 0) { return fail("distance_nonfinite"); }
		if (a_reason) { *a_reason = "measured"; }
		return distance;
	}

	int Mod::SharedCandidate(bool a_isLeft)
	{
		const double radius = SharedRadius();
		if (radius <= 0) { return 0; }
		int selected = 0;
		double nearest = radius;
		// Snapshot is sorted by slot, so equal distances deterministically use the
		// lower slot. No previous-frame reach is accepted by a drop callback.
		for (const auto& pouch : _pouches.Snapshot()) {
			const auto distance = SharedDistance(pouch.slot, a_isLeft);
			if (distance && *distance <= radius && (selected == 0 || *distance < nearest)) {
				selected = pouch.slot;
				nearest = *distance;
			}
		}
		return selected;
	}

	bool Mod::SharedDrawAllowed(bool a_isLeft)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		return SharedWorldReady() && player != nullptr && player->GetEquippedObject(a_isLeft) == nullptr &&
			!_higgs.IsHolding(a_isLeft) && _higgs.CanGrab(a_isLeft) &&
			!_drawQueued[IsSecondaryHand(a_isLeft) ? 1 : 0] &&
			!_pendingGrab[IsSecondaryHand(a_isLeft) ? 1 : 0].Active() &&
			_vrik.SlotInReach(IsSecondaryHand(a_isLeft)) == 0;
	}

	void Mod::PollSharedButtons()
	{
		const bool ready = SharedWorldReady();
		for (const bool left : { true, false }) {
			const int hand = left ? 0 : 1;
			const auto button = ready ? VR::ReadControllerButton(left,
				_vrik.GetSetting("useTriggersInstead") > .5) : VR::ControllerButton{};
			const auto edge = _sharedButtons[hand].Sample(button.available, button.context, button.down);
			if (edge == Core::ButtonEdges::Edge::Reset || edge == Core::ButtonEdges::Edge::Up) { SharedButton(left, false); }
			else if (edge == Core::ButtonEdges::Edge::Down) { SharedButton(left, true); }
		}
	}

	void Mod::SharedButton(bool a_isLeft, bool a_down)
	{
		const int hand = a_isLeft ? 0 : 1;
		_sharedWithdrawal[hand].Clear();
		_sharedPresses[hand].Clear();
		if (!a_down || !SharedWorldReady()) { return; }
		const auto pouches = _pouches.Snapshot();
		if (std::none_of(pouches.begin(), pouches.end(),
			[](const auto& pouch) { return pouch.mode == Core::Mode::Shared; })) { return; }
		// Input events may precede HIGGS's before-update callback. Resolve this real
		// edge in that next update, or the current update if already inside it.
		const auto target = _higgsUpdate + (_inHiggsUpdate ? 0 : 1);
		_sharedPresses[hand].Arm(_worldGeneration, target);
		_sharedPressAt[hand] = Now();
		Loc::Info(Keys::kSharedPress, HandName(a_isLeft), target, _worldGeneration);
	}

	void Mod::ResolveSharedPresses()
	{
		for (const bool isLeft : { true, false }) {
			const int hand = isLeft ? 0 : 1;
			if (!_sharedPresses[hand].Active()) { continue; }
			const auto now = Now();
			const bool current = _sharedPresses[hand].Take(_worldGeneration, _higgsUpdate);
			if (!_inHiggsUpdate || !current || now < _sharedPressAt[hand] ||
				now - _sharedPressAt[hand] > 100 || !SharedDrawAllowed(isLeft)) { continue; }
			const int slot = SharedCandidate(isLeft);
			const auto distance = SharedDistance(slot, isLeft);
			if (distance) { _sharedWithdrawal[hand].Press(slot, *distance, SharedRadius(), now); }
			Loc::Info(Keys::kSharedPressResolved, HandName(isLeft), _higgsUpdate,
				_worldGeneration, _sharedWithdrawal[hand].Slot());
		}
	}

	void Mod::UpdateSharedWithdrawals()
	{
		for (int index = 0; index < 2; ++index) {
			auto& gesture = _sharedWithdrawal[index];
			if (!gesture.Slot()) { continue; }
			const bool isLeft = index == 0;
			const auto distance = SharedDistance(gesture.Slot(), isLeft);
			const int slot = gesture.Update(SharedDrawAllowed(isLeft) && distance.has_value(),
				distance.value_or(-1), SharedRadius(), Now());
			if (slot != 0) {
				Loc::Info(Keys::kSharedDraw, slot, HandName(isLeft));
				HandleHolsterAttempt(slot, IsSecondaryHand(isLeft), false);
			}
		}
	}

	bool Mod::IsSecondaryHand(bool a_isLeft)
	{
		// The way back from IsLeftHand, and by the same setting.
		static const bool leftHanded = [] {
			const auto* setting = RE::GetINISetting("bLeftHandedMode:VRInput");
			return setting != nullptr && setting->GetBool();
		}();
		return leftHanded ? !a_isLeft : a_isLeft;
	}

	void Mod::OnGameLoading()
	{
		_loading = true;
		++_worldGeneration;
		_inHiggsUpdate = false;
		CancelSharedDrop(true);
		CancelSharedDrop(false);
		_queuedDrops.clear();
		std::scoped_lock guard(_lock);
		for (int hand = 0; hand < 2; ++hand) {
			_pendingGrab[hand].Clear();
			_sharedWithdrawal[hand].Clear();
			_sharedPresses[hand].Clear();
			_sharedButtons[hand].Clear();
			_drawQueued[hand] = false;
			_grabReference[hand].reset();
			_reach[hand] = 0;
			_drawnFrom[hand] = 0;
		}
		_pouches.NoteSettled(false);
		_pouches.NoteSettled(true);
		_displayAt = 0;
	}

	void Mod::OnGameLoaded()
	{
		// Also invalidate here for a new game or a missed pre-load notification.
		OnGameLoading();
		_loading = false;
		// Said out loud on purpose. A run of this mod once ended with a log that stopped
		// after the settings were read, and nothing in it could tell "the message never
		// arrived" from "it arrived and every step of it quietly did nothing".
		Loc::Info(Keys::kGameLoaded, _settings.pouches.size());
		ApplySlots();
		// Again here and not only at the first frame: that frame may come in the start
		// cave, where the player has no body yet to measure the slot against.
		DescribeSlots();
		_picture.Reset(Now());
	}

	void Mod::BuildPouches(const Settings& a_settings)
	{
		std::scoped_lock guard(_lock);
		_pouches.Clear();
		for (const auto& setting : a_settings.pouches) {
			_pouches.Set(Core::Pouch(setting.slot, Settings::ModeFromText(setting.mode)));
			Loc::Info(Keys::kPouchConfigured, setting.slot, setting.mode);
		}
	}

	void Mod::ApplySlots()
	{
		if (!Working()) {
			Loc::Warn(Keys::kNotArranging, _vrik.Ready(), _higgs.Ready());
			return;
		}
		std::scoped_lock guard(_lock);

		// What to do with each slot is decided in the core, which can be tried without
		// VRIK and without the game; all that happens here is the doing of it. The three
		// steps are independent on purpose: a slot that is off and that we are allowed
		// to switch on is switched on AND then taken over, in that order.
		for (const auto& setting : _settings.pouches) {
			const auto* pouch = _pouches.Find(setting.slot);
			if (pouch == nullptr) {
				continue;
			}

			Core::SlotFacts facts;
			facts.slot = setting.slot;
			facts.detectable = _vrik.SeeSlot(setting.slot).detectable;
			facts.exclusive = pouch->PouchMode() == Core::Mode::Exclusive;
			facts.maySwitchOn = _settings.mayEnableSlots;

			const auto plan = Core::PlanFor(facts);
			if (plan.switchOn) {
				_vrik.SwitchOn(setting.slot);
			}
			if (plan.complain) {
				// VRIK's setting, arranged by whoever arranged it. We say what is wrong
				// and where it is fixed, and leave it alone.
				Loc::Warn(Keys::kSlotOff, setting.slot);
			}
			if (plan.suspend) {
				// So that VRIK detects the hand but neither draws a weapon from the slot
				// nor holsters one into it. A suspended slot is also one an empty hand
				// sees at all: without this, VRIK looks straight past a free hand at an
				// empty slot and the pouch raises nothing.
				_vrik.SetSuspended(setting.slot, true);
			}
		}

		_slotsArranged = true;
	}

	void Mod::DescribeSlots()
	{
		auto*       player = RE::PlayerCharacter::GetSingleton();
		const float feet = player != nullptr ? player->GetPositionZ() : 0.0f;
		const auto  head = Game::Body::BoneNamed(kHeadBone);

		for (const auto& setting : _settings.pouches) {
			const char* boneName = Game::Body::BoneOf(setting.slot);
			const auto  bone = Game::Body::Bone(setting.slot);

			// Where VRIK has the slot, in its own numbers - said even without a body, since
			// they are what shows whether VRIK read a moved slot at all - and, with a body,
			// as heights a person can picture: "a hand below the head" says more than three
			// coordinates do.
			const auto pos = _vrik.SlotPosition(setting.slot);
			if (!bone) {
				Loc::Warn(Keys::kBoneMissing, setting.slot, boneName != nullptr ? boneName : "?",
					pos.x, pos.y, pos.z);
				continue;
			}
			const auto centre = Game::Body::ToWorld(*bone, pos);
			Loc::Info(Keys::kSlotWhere, setting.slot, boneName, pos.x, pos.y, pos.z, centre.z - feet,
				head ? head->translate.z - centre.z : 0.0f, bone->scale);

			// Which way the bone's own axes point. The numbers in vrikslots.ini are along
			// these, and which of them runs down the body is up to the skeleton, not VRIK.
			const auto x = Game::Body::Axis(*bone, 0);
			const auto y = Game::Body::Axis(*bone, 1);
			const auto z = Game::Body::Axis(*bone, 2);
			Loc::Info(Keys::kBoneAxes, boneName, x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z);
		}

		if (player != nullptr) {
			const float yaw = player->GetAngleZ();
			Loc::Info(Keys::kFacing, std::sin(yaw), std::cos(yaw));
		}
	}

	void Mod::Measure(bool a_isLeft)
	{
		RE::NiPoint3 hand;
		const char*  node = HandAt(a_isLeft, hand);
		if (node == nullptr) {
			return;
		}

		auto*       player = RE::PlayerCharacter::GetSingleton();
		const float feet = player != nullptr ? player->GetPositionZ() : 0.0f;

		for (const auto& setting : _settings.pouches) {
			const auto bone = Game::Body::Bone(setting.slot);
			if (!bone) {
				continue;
			}
			// The numbers to write into vrikslots.ini to put the pouch exactly where the
			// hand is now, beside where the pouch is.
			const auto here = Game::Body::ToSlot(*bone, hand);
			const auto centre = Game::Body::ToWorld(*bone, _vrik.SlotPosition(setting.slot));
			Loc::Info(Keys::kSqueezeMeasured, HandName(a_isLeft), setting.slot, here.x, here.y, here.z,
				hand.GetDistance(centre), hand.z - feet, centre.z - feet, node);
		}
	}

	bool Mod::IsLeftHand(bool a_secondaryHand)
	{
		// bLeftHandedMode swaps which controller is the primary one. Read once: it is a
		// launcher setting and does not change while the game runs.
		static const bool leftHanded = [] {
			const auto* setting = RE::GetINISetting("bLeftHandedMode:VRInput");
			return setting != nullptr && setting->GetBool();
		}();
		return leftHanded ? !a_secondaryHand : a_secondaryHand;
	}

	const char* Mod::HandAt(bool a_isLeft, RE::NiPoint3& a_out)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* root = player != nullptr ? player->Get3D() : nullptr;
		if (root == nullptr) {
			return nullptr;
		}

		for (const char* name : (a_isLeft ? std::span<const char* const>(kLeftNodes) : std::span<const char* const>(kRightNodes))) {
			if (auto* node = root->GetObjectByName(name); node != nullptr) {
				a_out = node->world.translate;
				return name;
			}
		}
		return nullptr;
	}

	bool Mod::HandPosition(bool a_isLeft, RE::NiPoint3& a_out)
	{
		const char* node = HandAt(a_isLeft, a_out);
		if (node == nullptr) {
			Loc::Warn(Keys::kHandNotFound, HandName(a_isLeft));
			return false;
		}
		Loc::Info(Keys::kHandNode, HandName(a_isLeft), node);
		return true;
	}

	bool Mod::Draw(int a_slot, bool a_isLeft, const Core::FormKey& a_item)
	{
		const int hand = IsSecondaryHand(a_isLeft) ? 1 : 0;
		if (_loading || _pendingGrab[hand].Active() || _higgs.IsHolding(a_isLeft) ||
			!_higgs.CanGrab(a_isLeft)) {
			return false;
		}
		// The queue may run after another action changed the hand, stock or filter.
		{
			std::scoped_lock guard(_lock);
			Core::Reach reach;
			reach.slot = a_slot;
			reach.leftHand = a_isLeft;
			reach.handCanHold = true;
			const auto current = _pouches.Decide(reach, _pack);
			if (current.act != Core::Act::Draw || !(current.item == a_item)) {
				return false;
			}
		}
		Loc::Info(Keys::kDrawStart, a_slot, HandName(a_isLeft));

		auto* object = Game::PlayerPack::Held(a_item);
		if (object == nullptr) {
			Loc::Warn(Keys::kItemNotFound, a_item.plugin, a_item.localId);
			return false;
		}

		RE::NiPoint3 where;
		if (!HandPosition(a_isLeft, where)) {
			return false;
		}

		auto*        player = RE::PlayerCharacter::GetSingleton();
		if (player == nullptr) {
			return false;
		}
		RE::NiPoint3 rotation{};

		// One call does both halves of what we need: the potion leaves the inventory and
		// a reference to it appears in the world, which is the only thing HIGGS can put
		// in a hand. Dropping it is not a metaphor - from here on it is an ordinary
		// bottle lying in the world that happens to be held.
		const auto handle = player->RemoveItem(
			object, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr, &where, &rotation);

		const auto dropped = handle.get();
		if (!dropped) {
			Loc::Warn(Keys::kDropFailed);
			return false;
		}

		{
			std::scoped_lock guard(_lock);
			_pouches.NoteDrawn(a_isLeft, a_slot, a_item);
		}

		_drawnFrom[hand] = a_slot;
		_grabReference[hand] = handle;
		_pendingGrab[hand].Arm(handle.native_handle(), Now() + kGrabCheckMs);
		// Arm before calling HIGGS so a synchronous callback is correlated too.
		_higgs.Grab(dropped.get(), a_isLeft);
		Loc::Info(Keys::kGrabAsked, HandName(a_isLeft));
		_displayAt = 0;  // the last bottle of its kind may just have left the pack

		Loc::Info(Keys::kPouchDrawn, a_slot, HandName(a_isLeft));
		return true;
	}

	void Mod::StowDropped(int a_slot, bool a_isLeft, RE::TESObjectREFR* a_object)
	{
		if (_loading || a_object == nullptr || a_object->IsDeleted() || a_object->IsDisabled() ||
			_higgs.Held(false) == a_object || _higgs.Held(true) == a_object) {
			return;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player == nullptr) {
			return;
		}

		auto* base = a_object->GetBaseObject();
		if (base == nullptr) {
			Loc::Warn(Keys::kHeldNoBase, HandName(a_isLeft), a_object->GetFormID());
			return;
		}

		const auto* name = base->GetName();
		Loc::Info(Keys::kHeldIs, HandName(a_isLeft), name != nullptr ? name : "",
			RE::FormTypeToString(base->GetFormType()), base->GetFormID(), a_object->GetFormID());

		auto* potion = base->As<RE::AlchemyItem>();
		if (potion == nullptr) {
			// Not ours: it stays on the ground, exactly where it was let go of.
			Loc::Info(Keys::kHeldNotPotion, HandName(a_isLeft));
			return;
		}

		// A reference in the world is not always one bottle. Asked once, here, and used
		// both for the decision and for the picking up.
		const int count = std::max(1, a_object->extraList.GetCount());

		auto item = Game::Describe(potion);
		item.count = count;

		bool taken = false;
		{
			std::scoped_lock guard(_lock);
			const auto decision = _pouches.Offer(a_slot, a_isLeft, item);
			if (decision.act == Core::Act::Assign) {
				if (_pouches.Assign(a_slot, item)) {
					Loc::Info(Keys::kPouchAssigned, a_slot);
					taken = true;
				}
			} else if (decision.act == Core::Act::Stow) {
				taken = true;
			}
		}

		if (!taken) {
			Loc::Info(Keys::kDropNotOurs, HandName(a_isLeft), a_slot);
			return;
		}

		// THE ENGINE ALREADY KNOWS HOW TO PICK A THING UP, AND THIS USED NOT TO USE IT.
		// What stood here was AddObjectToContainer for one, then Disable and SetDelete on
		// the reference - which looks like the same thing and is not. A reference in the
		// world can stand for more than one bottle, so a pile of five went into the pack
		// as one and the other four were deleted: the mod took the player's potions away.
		// PickUpObject counts, keeps whatever else the reference carried, and makes the
		// sound the game makes when anything else is picked up.
		player->PickUpObject(a_object, count, false, true);

		{
			std::scoped_lock guard(_lock);
			_pouches.NoteSettled(a_isLeft);
		}

		_drawnFrom[IsSecondaryHand(a_isLeft) ? 1 : 0] = 0;
		_displayAt = 0;  // a pouch just set up has something to show at once

		Loc::Info(Keys::kDropTaken, a_slot, HandName(a_isLeft), count);
	}

	void Mod::WatchInput()
	{
		if (_watchingInput) {
			return;
		}
		auto* manager = RE::BSInputDeviceManager::GetSingleton();
		if (manager == nullptr) {
			return;
		}
		manager->AddEventSink(&_input);
		_watchingInput = true;
		Loc::Info(Keys::kInputWatch, kGripButton);

		// Which hand VRIK's "secondary" is taken to mean here, and the setting it is read
		// from. Said out loud because the reading is ours and not VRIK's, and nobody can
		// check it unless what was read is in the log beside what was done with it.
		const auto* setting = RE::GetINISetting("bLeftHandedMode:VRInput");
		Loc::Info(Keys::kHandedness, setting != nullptr && setting->GetBool(),
			HandName(IsLeftHand(true)));
	}

	RE::BSEventNotifyControl Mod::Input::ProcessEvent(RE::InputEvent* const* a_event,
		RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (a_event == nullptr) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto& mod = GetSingleton();
		if (!mod.Working()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		for (auto* event = *a_event; event != nullptr; event = event->next) {
			auto* button = event->AsButtonEvent();
			if (button == nullptr) {
				continue;
			}

			bool isLeft = false;
			switch (event->GetDevice()) {
			case RE::INPUT_DEVICE::kVRLeft:
				isLeft = true;
				break;
			case RE::INPUT_DEVICE::kVRRight:
				isLeft = false;
				break;
			default:
				continue;  // keyboard, mouse, gamepad: not a hand
			}

			// Every press carries its real id into the log: out loud at any slot of VRIK's,
			// quietly everywhere else, because everywhere else is every press in the game.
			const int id = static_cast<int>(button->GetIDCode());
			// Shared gestures use original runtime edges, not VRIK-filtered/replayed events.
			if (!button->IsDown()) { continue; }
			const int reached = mod._reach[IsSecondaryHand(isLeft) ? 1 : 0];
			if (reached != 0) {
				Loc::Info(Keys::kButtonAtSlot, HandName(isLeft), id, reached);
			} else {
				Loc::Debug(Keys::kButtonElsewhere, HandName(isLeft), id);
			}

			// The grip of an empty hand is measured wherever the hand is: the place the
			// player reaches for is exactly the place the pouch is not yet. Out of the
			// input handler and onto the game queue, like everything that reads the body.
			if (id == kGripButton && !mod._higgs.IsHolding(isLeft)) {
				if (auto* tasks = SKSE::GetTaskInterface(); tasks != nullptr) {
					tasks->AddTask([isLeft]() { GetSingleton().Measure(isLeft); });
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	bool Mod::OnHolsterAttempt(int a_slot, bool a_secondaryHand, bool a_handOccupied)
	{
		// Before anything is decided, so that the log answers the first question any run
		// asks: was this callback called at all? Said at info and not at debug on
		// purpose - a line that only appears when somebody remembered to raise the log
		// level is a line that is missing from the run that needed it.
		Loc::Info(Keys::kHolsterOffered, a_slot, a_secondaryHand, a_handOccupied);
		auto& mod = GetSingleton();
		// Native VRIK selection owns arbitration, including an unrelated weapon slot.
		mod._sharedWithdrawal[IsLeftHand(a_secondaryHand) ? 0 : 1].Clear();
		mod._sharedPresses[IsLeftHand(a_secondaryHand) ? 0 : 1].Clear();
		mod.CancelSharedDrop(IsLeftHand(a_secondaryHand));
		return mod.HandleHolsterAttempt(a_slot, a_secondaryHand, a_handOccupied);
	}

	bool Mod::HandleHolsterAttempt(int a_slot, bool a_secondaryHand, bool a_handOccupied)
	{
		auto& mod = *this;
		if (!mod.Working() || mod._loading) {
			Loc::Debug(Keys::kIdleHere);
			return true;  // not our business: let VRIK do what it always did
		}

		const bool isLeft = IsLeftHand(a_secondaryHand);

		Core::Reach reach;
		reach.slot = a_slot;
		reach.leftHand = isLeft;
		reach.handOccupied = a_handOccupied;
		reach.handCanHold = mod._higgs.CanGrab(isLeft);

		Core::Decision decision;
		{
			std::scoped_lock guard(mod._lock);
			decision = mod._pouches.Decide(reach, mod._pack);
		}

		// The whole point of the core is in this one value, so it is said before anything
		// is done with it: act, reason, hand, and the bool VRIK is about to be given.
		Loc::Info(Keys::kDecision, decision.slot, Core::Name(decision.act),
			Core::Name(decision.reason), HandName(isLeft), decision.LetVrikAct());

		// VRIK is waiting for an answer this very frame, so the decision is made here
		// and the doing is put on the game's task queue. Removing an item from the
		// inventory and placing a reference is not work to start inside somebody else's
		// callback.
		if (decision.act == Core::Act::Draw) {
			const int hand = a_secondaryHand ? 1 : 0;
			if (mod._drawQueued[hand] || mod._pendingGrab[hand].Active()) { return false; }
			if (auto* tasks = SKSE::GetTaskInterface(); tasks != nullptr) {
				mod._drawQueued[hand] = true;
				tasks->AddTask([slot = decision.slot, isLeft, item = decision.item,
					generation = mod._worldGeneration]() {
					auto& current = GetSingleton();
					if (!current._loading && generation == current._worldGeneration) {
						current._drawQueued[IsSecondaryHand(isLeft) ? 1 : 0] = false;
						current.Draw(slot, isLeft, item);
					}
				});
				Loc::Info(Keys::kTaskQueued, decision.slot);
			} else {
				Loc::Error(Keys::kNoTasks, decision.slot);
			}
		} else if (decision.act == Core::Act::Refuse && decision.reason == Core::Reason::NothingInPack) {
			// An empty hand pulled out of the pouch and came back with nothing: worth its
			// own line, because to the player it looks exactly like a pouch that is broken.
			Loc::Info(Keys::kPouchEmpty, decision.slot);
		}

		return decision.LetVrikAct();
	}

	void Mod::OnConsumed(bool a_isLeft, ::TESForm*)
	{
		Loc::Info(Keys::kHiggsEvent, "consumed", HandName(a_isLeft));

		auto& mod = GetSingleton();
		mod._sharedWithdrawal[a_isLeft ? 0 : 1].Clear();
		mod._sharedPresses[a_isLeft ? 0 : 1].Clear();
		mod.CancelSharedDrop(a_isLeft);
		std::scoped_lock guard(mod._lock);
		if (const auto slot = mod._pouches.SlotOfHand(a_isLeft); slot.has_value()) {
			Loc::Info(Keys::kPouchConsumed, *slot);
			mod._pouches.NoteSettled(a_isLeft);
		}
	}

	void Mod::OnStashed(bool a_isLeft, ::TESForm*)
	{
		Loc::Info(Keys::kHiggsEvent, "stashed", HandName(a_isLeft));

		auto& mod = GetSingleton();
		mod._sharedWithdrawal[a_isLeft ? 0 : 1].Clear();
		mod._sharedPresses[a_isLeft ? 0 : 1].Clear();
		mod.CancelSharedDrop(a_isLeft);
		std::scoped_lock guard(mod._lock);
		if (const auto slot = mod._pouches.SlotOfHand(a_isLeft); slot.has_value()) {
			Loc::Info(Keys::kPouchReturned, *slot);
			mod._pouches.NoteSettled(a_isLeft);
			mod._displayAt = 0;  // the bottle is back in the pack, and may be the only one
		}
	}

	void Mod::OnGrabbed(bool a_isLeft, ::TESObjectREFR* a_refr)
	{
		auto&     mod = GetSingleton();
		mod._sharedWithdrawal[a_isLeft ? 0 : 1].Clear();
		mod._sharedPresses[a_isLeft ? 0 : 1].Clear();
		mod.CancelSharedDrop(a_isLeft);
		const int hand = IsSecondaryHand(a_isLeft) ? 1 : 0;

		// This fires for everything the player picks up, so it is only worth a word when
		// a bottle of ours is waiting to be told whether it was taken.
		auto* object = reinterpret_cast<RE::TESObjectREFR*>(a_refr);
		if (object != nullptr) {
			// Regrabbing cancels its earlier release, even if the player drops it
			// elsewhere again before the queued pickup has had a turn.
			mod._queuedDrops.erase(object->GetHandle().native_handle());
		}
		if (mod._loading || object == nullptr ||
			!mod._pendingGrab[hand].Confirm(object->GetHandle().native_handle(), Now())) {
			Loc::Debug(Keys::kHiggsEvent, "grabbed", HandName(a_isLeft));
			return;
		}

		mod._grabReference[hand].reset();
		Loc::Info(Keys::kGrabConfirmed, HandName(a_isLeft), mod._drawnFrom[hand]);
	}

	void Mod::OnDropped(bool a_isLeft, ::TESObjectREFR* a_refr)
	{
		Loc::Info(Keys::kHiggsEvent, "dropped", HandName(a_isLeft));

		auto& mod = GetSingleton();
		mod._sharedWithdrawal[a_isLeft ? 0 : 1].Clear();
		mod._sharedPresses[a_isLeft ? 0 : 1].Clear();
		if (!mod.Working() || mod._loading) {
			Loc::Info(Keys::kDropGuard, mod.Working(), mod._loading, a_refr != nullptr);
			return;
		}

		mod.CancelSharedDrop(a_isLeft);
		const int native = mod._vrik.SlotInReach(IsSecondaryHand(a_isLeft));
		// Native VRIK reach keeps its existing callback-time path. The shared spatial
		// fallback is resolved after VRIK in this exact update, never from cached reach.
		if (const int slot = mod.PouchAt(a_isLeft, native, false); slot != 0 && a_refr != nullptr) {
			Loc::Info(Keys::kDropAtPouch, HandName(a_isLeft), slot);
			const auto handle = reinterpret_cast<RE::TESObjectREFR*>(a_refr)->GetHandle();
			const auto serial = ++mod._dropSerial;
			mod._queuedDrops[handle.native_handle()] = serial;
			mod.QueueStowDropped(slot, a_isLeft, handle, serial);
			return;
		}
		const auto pouches = mod._pouches.Snapshot();
		const bool hasShared = std::any_of(pouches.begin(), pouches.end(),
			[](const auto& pouch) { return pouch.mode == Core::Mode::Shared; });
		if (native == 0 && hasShared && mod._inHiggsUpdate && a_refr != nullptr) {
			auto& release = mod._sharedReleases[a_isLeft ? 0 : 1];
			release.handle = reinterpret_cast<RE::TESObjectREFR*>(a_refr)->GetHandle();
			release.serial = ++mod._dropSerial;
			release.event.Arm(mod._worldGeneration, mod._higgsUpdate);
			mod._queuedDrops[release.handle.native_handle()] = release.serial;
			Loc::Info(Keys::kSharedRelease, HandName(a_isLeft), release.handle.native_handle(),
				mod._higgsUpdate, mod._worldGeneration);
			return;
		}
		mod.SettleOutsideDrop(a_isLeft);
	}
}
