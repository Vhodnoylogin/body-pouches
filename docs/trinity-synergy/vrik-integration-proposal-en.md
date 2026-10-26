# Shared services and Body Slots: a proposal for Shizof

[Русская версия](vrik-integration-proposal-ru.md) · [Full concept](concept-en.md) · [Implementation status](implementation-status.md)

Shizof, thank you for VRIK and your help with Body Pouches. Releasing it showed what a small mod can do by using existing frameworks. It also exposed a recurring difficulty: third-party mods still have to recreate slot management, input coordination or parts of another mod's internal behavior when an interface cannot express what they need.

## The idea in one minute

Keep each framework responsible for its own domain and let other mods ask it for observations and actions. A potion mod should define which potion to offer; it should be able to use the shared body, slot and physical-hand services instead of building them again.

| Participant | Proposed responsibility |
|---|---|
| HIGGS | Controller input coordination, physical hands, grabbing and possession |
| VRIK | The calibrated player body, virtual-hand positions, body attachment points, spatial slot interaction and its agreed presentation responsibilities |
| PLANCK | NPC physical animation, ragdoll lifetime and physical hits |
| Body Slots | Create and configure slots; store definitions, individual instances and saved adjustments |
| A gameplay mod | Its items, selection rules and gameplay, using those services |

These are the existing concept's proposed boundaries, not a claim that the complete shared API already exists. A second stage would move address handling and reusable engine operations into CommonLib; gameplay ownership stays with the mods.

## A concrete target: a wearable potion bag

Equip a bag: its slot becomes available where the bag hangs. Reach into it and take a potion through the familiar VRIK/HIGGS interaction. Remove the bag: its slot becomes unavailable. Another mod could supply a different holder and item rules through the same services.

![Target workflow: the bag enables its slot when worn. Illustration, not a game screenshot.](images/wearable-slot-workflow.svg)

Body Slots would store the slot description, placement and customization. The equipment mod would say which particular bag is worn. VRIK would bind the slot to the actual body and interpret the hand's reach. HIGGS would handle the physical potion. A holder's model and the preview of its contents remain separate, with one declared owner for displaying the holder.

## What is ready, and what is not

- **Released:** Body Pouches 0.2.0 uses an existing VRIK holster for potion assignment, drawing, drinking, return and saved selection.
- **Built and checked outside the game:** a separate Body Slots plugin and SDK, with definitions, instances, saved placement and activation. Runtime/SDK archives are in an unpublished Nexus draft.
- **Prepared demonstration:** one wearable item controls a real Body Slots instance through released VRIK's existing holster13. Its actual equipment, visible model and potion interaction still need game validation. It does not yet demonstrate arbitrary new slots or equipment-model binding.
- **Available separately:** [Morphbench Slots experimental build](https://github.com/Vhodnoylogin/morphbench/releases/tag/v0.8.0-slots.1) for authoring default placement outside the game.

The [0.8.8 / PLANCK update assessment](upstream-update-2026-10-26.md) separates current compatibility evidence from those remaining game checks.

## A small first boundary to discuss

Would an optional bridge make sense through which VRIK reads slot definitions, maps named attachment points after updating its body, and reports positions and availability? Definitions would be copied outside the per-hand hot path. Missing Body Slots would preserve the existing native path. This first step would supply spatial observations; new-slot interaction and rendering would be agreed separately, preserving the original fourteen holsters and existing users.

The [first limited MR](https://github.com/Vhodnoylogin/vrik-reconstruction/pull/1) illustrates that boundary. The [complete current-work review](https://github.com/Vhodnoylogin/vrik-reconstruction/pull/2) now puts VRIK and the full Body Slots history in separate directories in one private repository. Both require repository access. The VRIK work uses a private reconstruction based on 0.8.7; it has not been rebased to 0.8.8 and is not authoritative maintainer source. The independently defined boundary can be adapted to your implementation without accepting that reconstruction.

## Your feedback would decide the next step

1. Does keeping slot definitions and saved customization in a separate optional mod fit your view of VRIK?
2. Is the proposed body-attachment bridge a sensible first integration point, or would you prefer another boundary or existing API?
3. Which parts of generic slot interaction and presentation should remain in VRIK, and what would you change before we proceed?

I can contribute AI-assisted code analysis and implementation drafts for your review at no financial cost to you. This proposal does not assume an implementation or maintenance commitment from you. The [full concept](concept-en.md) remains the single maintained description of the broader design; this page is its short introduction.
