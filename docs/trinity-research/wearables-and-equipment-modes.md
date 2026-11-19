# Wearable physics and effective equipment modes

Research checkpoint: 2 November 2026. These additional consumer cases extend the [twenty-mod review](consumer-contract-matrix.md). They assess mechanisms and required contracts, not compatibility or a port of the referenced releases. No game experiment was performed. The maintained concept remains [English](../trinity-synergy/concept-en.md) / [Russian](../trinity-synergy/concept-ru.md).

## Moving bags and scabbards

A wearable can supply a Body Slots set while equipped. Bind the access volume to a dedicated opening node and save customization in that node's local frame. Ordinary sway updates the observed transform, not persistent placement or anchor lifetime. A skinned/deforming mesh is not automatically one rigid interaction volume.

The inspected [FSMP bone implementation](https://github.com/DaymareOn/hdtSMP64/blob/e52ad960048c14e637d14fcbd65314ca017fc3b8/src/hdtSkyrimBone.cpp#L94) writes physical bone position/rotation to the scene node. Body Slots already samples current anchor transforms. This supports the proposed observation route, but their update ordering and actual model topology remain unqualified. A physics bone may live under the actor skeleton rather than the equipment clone: author the binding against the actual retained node, with unambiguous ownership, required worn item and invalidation on removal/replacement.

A swinging volume may enter a stationary hand. Entry is geometric evidence, not permission to draw. After an authorized action starts, small motion should not repeatedly cancel its content transfer; removal and replacement remain distinct lifecycle events. Holding the accessory with the other hand additionally needs an agreed HIGGS/accessory-physics endpoint or handoff. Two engines writing the same accessory transform is not a solution. No FSMP integration or grabbing bridge is delivered here.

## Ordinary cross-hand VRIK drawing

The downloaded original VRIK0.8.8 `_vrik_qust_system.psc` already handles an item that remains equipped opposite while sheathed: OnActivateSlot removes that equipment, then selects/equips the slot weapon for the requested hand. The release's `handSlotN`/Permit Hands settings restrict which hand may use the slot. Optional weapon swapping adds separate occupied-target rules. These findings come from original release Papyrus/configuration, not reconstructed C++ or a game run.

VRIK's ordinary one-handed path therefore does not require a VRIK modification merely to put away with one hand and draw with the other. Its form-based arrays/EquipItemEx do not establish exact UID/extra-list identity or a correlated compound receipt. The historical proxy adaptation's left-hand limitation must not be generalized to stock VRIK. Physical left/right also differs from primary/offhand under the global left-handed setting.

Our bounded HIGGS inventory executor now explicitly refuses opposite-worn cross-hand equipment, including a request reserving both hands, until it can prove old-hand removal, target-hand adoption and restoration of both on failure. An apparently empty sheathed hand is not proof of absent inventory equipment. Exact-list and ambiguous plain-item checks run before mutation admission and again after fresh resolution. The refusal prevents a destination-only grant from silently changing the other hand. It is not the requested compound transition implementation.

## Original two-handed weapons in one equipment slot

[Dynamic Two-Handed](https://www.nexusmods.com/skyrimspecialedition/mods/170549) points to [public source](https://github.com/vinymayan/Dynamic-Grip-NPC/tree/97db41fb02ffb7900be3a5bd95a7df27b55d1acf). Its [EquipInSlot](https://github.com/vinymayan/Dynamic-Grip-NPC/blob/97db41fb02ffb7900be3a5bd95a7df27b55d1acf/src/Hooks.cpp#L334) temporarily changes the original weapon form's equipment slot around a synchronous manager call, then restores the form. This provides a concrete alternative to creating a one-handed weapon proxy for that operation. The base weapon, effective equipment location and physical one/two-hand support must remain distinct.

The [public API](https://github.com/vinymayan/Dynamic-Grip-NPC/blob/97db41fb02ffb7900be3a5bd95a7df27b55d1acf/include/Dynamic2HAPI.h) accepts actor, weapon, extra data, grip and hand. Its boolean result is not our correlated exact-instance transition receipt. A shared equipment executor still needs complete postconditions, duplicate-copy handling, source/destination reservations, restoration and explicit unsupported outcomes. Temporary shared-form writes and reentrant engine calls need their own qualification. No attempt was made to build, install or establish compatibility of this mod with VR.

## Original shield in the physical right hand

Moving a two-handed weapon into a left weapon slot does not prove a working right-hand shield. Dynamic Two-Handed's public grip API is weapon-only; it does not supply a corresponding ARMO-shield operation. Existing NPC shield selection still uses the normal shield slot. Global left-handed mirroring is different from independent arbitrary equipment in the normal handedness mode.

The inspected CommonLib source models equip-slot methods for both weapons and armor, but armor also has independent biped/addon/worn-model behavior. HIGGS's shield node/offset selection and PLANCK's block/bash attribution use game offhand roles. A relocated visual is therefore insufficient evidence of correct shield collision and combat behavior.

The next mechanism experiment must use original instances and check both equipment locations and worn flags, exact extra data, first/third-person attachment, collision, block/bash/hit attribution, unequip and save/load. This remains unperformed and requires coordinated game authorization. Physically holding the original shield world reference with HIGGS is another possible route; it would need a separate block/bash executor and is not equivalent to native shield equipment. Neither route is currently proved to eliminate all proxies from Shields and 2H Weapons Unlocked.
