# From potion pouches to reusable body slots: a proposal for Shizof

[Русская версия](vrik-integration-proposal-ru.md) · [Full concept](concept-en.md) · [Implementation status](implementation-status.md)

Shizof, thank you for VRIK and your help with Body Pouches. Its release demonstrated how much a small mod can achieve by using VRIK and HIGGS. This proposal takes the next step: let other mods provide wearable bags and other holders without each implementing another body-slot system.

This brief selects one reviewable boundary from the existing [VRIK responsibilities](concept-en.md#vrik-body-hands-zones-and-slots) and [separate slot service](concept-en.md#separate-slot-mod-and-offline-authoring) sections. The full concept remains maintained only here in Body Pouches.

## The result we want to show

The player equips a potion bag. Its usable slot appears where the bag hangs on the body. The player reaches into that bag and takes a potion through the familiar VRIK/HIGGS interaction. Removing the bag disables its slot. Another mod could use the same services for an ammunition holder or a different wearable container, supplying its own item rules and appearance.

![Target player workflow: equipping a bag enables its slot; removing it disables the slot. This is an illustration, not a game screenshot.](images/wearable-slot-workflow.svg)

Today Body Pouches uses one of VRIK's existing holsters. The proposed shared service would let an item bring its own slot, with placement and customization managed once. This is the existing concept applied to one concrete example.

## What is actually ready

| Part | Present result | What this establishes |
|---|---|---|
| [Body Pouches 0.2.0](https://www.nexusmods.com/skyrimspecialedition/mods/194796) | Released native-holster potion pouch: assign, draw, drink, return and save | Existing gameplay, with the released mod's documented limitations |
| Body Slots | Separate built plugin and SDK; definitions, per-instance settings and activation; uploaded to an unpublished Nexus draft | A mod-author foundation checked outside the game; it does not itself add playable holsters to released VRIK |
| [Private wearable demonstration source and diff](https://github.com/Vhodnoylogin/body-pouches/pull/1) | One equip item controls a real Body Slots instance; a private installation archive is prepared | Both build modes and offline logic/ESP checks pass; actual equipment, visuals and interaction still need game validation |
| [Morphbench Slots experimental build](https://github.com/Vhodnoylogin/morphbench/releases/tag/v0.8.0-slots.1) | Separate prerelease for authoring slot placement outside the game | Authoring/export checks pass; this is separate from the stable Nexus release |

The wearable demonstration deliberately reuses **native holster13 in released VRIK** and existing HIGGS interaction. It prepares a small demonstration without distributing a reconstructed VRIK DLL. It does not yet demonstrate arbitrary new slots, binding to an equipment model, or several interchangeable copies of a bag. The figure above shows the target workflow; **the new wearable build is not game-validated proof yet**.

## Why keep the slot manager separate

| Participant | Responsibility in this example |
|---|---|
| Body Slots | Store slot descriptions, each bag's logical slot instance, placement and saved adjustments; enable or disable an instance when requested by its owner |
| VRIK | Supply the calibrated body, map body attachment points, interpret where the virtual hands are, and retain its agreed presentation responsibilities |
| Body Pouches | Observe whether its bag is worn and define potion selection, assignment and gameplay behavior |
| HIGGS | Retain physical possession and handling of the potion |
| Morphbench | Let the author place and configure default slots outside the game |

The equipment provider supplies stable identities for individual bags. A holder's appearance and the preview of its contents remain separate, with one declared owner for displaying the holder. The [technical boundary diagram](images/vrik-integration-boundary.svg) shows these relationships. The complete responsibilities remain in the linked concept.

## Start with an optional read-only bridge

The first VRIK change would let the separate service find its slots relative to VRIK's calibrated body. VRIK would read the slot definitions, map their named attachment points after its body update, and report each slot's position and availability. Definitions would be copied when needed, outside the per-hand hot path. An unavailable attachment point would leave its slot unavailable; an absent Body Slots provider would preserve the existing native path.

This first bridge supplies body-relative spatial information. New-slot interaction and rendering would be agreed separately. It therefore does not require moving all existing holster behavior at once.

The [private draft MR](https://github.com/Vhodnoylogin/vrik-reconstruction/pull/1) makes that first boundary inspectable. Its isolated `f392…` prototype has this measured diff:

| Scope | Change |
|---|---|
| Existing `main.cpp` | 8 added lines |
| Existing `vrikbody.cpp` | 11 added lines |
| Entire first prototype | 10 files; 506 additions; 0 removals |

The other files introduce adapter code and documentation. These numbers describe **only the first read-only prototype**, not the effort or risk of full integration, interaction or rendering.

The MR is private; its code diff is visible only to people with repository access. It uses reconstructed source, which is not authoritative maintainer source. No closed-source excerpts are reproduced here or proposed for public distribution. Its independently defined adapter boundary can be adapted to your implementation and conventions; accepting the reconstruction is not a prerequisite for discussing it.

## What I would like to discuss

I would welcome your judgement on the ownership split and a suitable optional read-only integration point first. We can then agree separately on lifecycle, content/presentation observations and any interaction requests, preserving the original fourteen holsters and existing consumers. The proposal does not assign implementation or maintenance commitments to you.
