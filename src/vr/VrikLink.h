#pragma once

#include "vrikinterface001.h"
#include "vr/NativeSuspension.h"

#include <RE/Skyrim.h>

#include <array>
#include <optional>

namespace BodyPouches::VR
{
	// Raw configuration and a mode-specific prerequisite verdict. A positive verdict
	// is not evidence of a current hand, active suspension or a committed action.
	struct SlotView
	{
		// small, medium, large, ranged, shield, torch - in that order.
		std::array<double, 6> allows{};
		double                visible{};
		bool                  detectable{};
	};

	// Our side of VRIK.
	//
	// VRIK owns slot placement, weapon hover and display. Its filtered hover omits
	// empty shared slots for nonweapon hands; Mod supplies that narrow spatial path
	// from loaded VRIK settings without changing the slots. Three public hooks arrived
	// in build 80700:
	//
	//   AddHolsterAttemptCallback  - tell me when a hand reaches for a slot, and let my
	//                                answer decide whether you act or I do
	//   SetSlotSuspended           - for a slot given over to pouches entirely
	//   VrikSetSlotForArt          - what to draw there
	//
	// An older VRIK has none of them, and then this mod does nothing at all rather than
	// half of something.
	class VrikLink
	{
	public:
		// The build that first had the pouch functions. Anything older and the v-table
		// ends before the ones we need - calling them would not fail, it would jump
		// into whatever lies past the end.
		static constexpr unsigned int kRequiredBuild = 80700;

		[[nodiscard]] bool Connect();
		[[nodiscard]] bool Ready() const noexcept { return _api != nullptr && _build >= kRequiredBuild; }
		[[nodiscard]] unsigned int Build() const noexcept { return _build; }

		bool Subscribe(vrikPluginApi::IVrikInterface001::HolsterAttemptCallback a_callback);

		bool SetSuspended(int a_slot, bool a_suspended);
		[[nodiscard]] bool IsSuspended(int a_slot);
		// Renew only this consumer's desired claims; load/reset uses fresh handles.
		void MaintainSuspensions() noexcept;

		// Whether the slot's art is on the slot's bone and was put in place this frame.
		[[nodiscard]] bool IsDisplayed(int a_slot);

		// What VRIK draws in the slot: this art, whenever the game plays it on the player.
		// VRIK hangs the art's model on the slot's bone the moment the model is set up and
		// keeps it in the middle of the slot. Once per game is enough - VRIK keeps it in
		// its DLL's memory - and the art has to be a record, which is why the mod has a
		// plugin (game/Picture.h).
		//
		// VrikSetSlotWeaponType is not the way, whatever its name suggests: for anything
		// but a weapon, a shield or a torch it records "empty", and it never draws.
		void SetArt(int a_slot, RE::BGSArtObject* a_art);

		// Where VRIK has the slot: posXN, posYN and posZN, an offset in the axes of the
		// slot's bone (game/Body.h). As VRIK read them when the game started.
		[[nodiscard]] RE::NiPoint3 SlotPosition(int a_slot);
		// Current correction used by VRIK's same-update reach; Final is next-frame.
		[[nodiscard]] std::optional<RE::NiPoint3> CurrentHandOffset();

		// VRIK's own settings, the ones that live in vrikslots.ini and vrik.ini.
		//
		// IN MEMORY ONLY. The contract says it plainly - "files are not saved
		// automatically" - and saveSettings(), the call that would write them out, is
		// deliberately not wrapped here. A mod that quietly rewrote somebody's VRIK
		// configuration on disk would be doing the very thing we refuse to do by hand.
		[[nodiscard]] double GetSetting(const char* a_name);
		void                 SetSetting(const char* a_name, double a_value);

		// Configuration prerequisite for the requested pouch mode, not live reach.
		// Exclusive pouches suspend native holstering and need no weapon category.
		[[nodiscard]] bool IsDetectable(int a_slot, bool a_exclusive = false);

		// The same reading, said out loud. One line per slot, with every value it was
		// decided from.
		SlotView SeeSlot(int a_slot, bool a_exclusive = false);

		// Which slot VRIK thinks this hand has reached, and whether it would holster
		// there. VRIK answers these whether or not it decides to raise a holster
		// attempt, which is why they are worth asking: a slot that never raises an
		// attempt but does show up here is a slot VRIK sees the hand at and chooses to
		// do nothing about. Zero means no slot.
		[[nodiscard]] int  SlotInReach(bool a_secondaryHand);
		[[nodiscard]] bool CanBeHolstered(bool a_secondaryHand);

		// Switch a slot on. NOT called unless the player has asked for it in our own
		// settings: VRIK's configuration belongs to VRIK and to whoever set it, and a
		// mod that quietly changes it - even in memory, even reversibly - leaves that
		// person looking at one thing in their menu and getting another in the game.
		// Returns false when the slot was already on, so the caller can stay quiet.
		bool SwitchOn(int a_slot, bool a_exclusive = false);

	private:
		// The reading itself, silent: SeeSlot says it, IsDetectable only asks.
		[[nodiscard]] SlotView Read(int a_slot, bool a_exclusive);

		vrikPluginApi::IVrikInterface001* _api{ nullptr };
		unsigned int                      _build{ 0 };
		NativeSuspension                  _nativeSuspension;
	};
}
