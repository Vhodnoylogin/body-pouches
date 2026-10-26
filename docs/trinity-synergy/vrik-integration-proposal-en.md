# Body Slots: extensible slots with VRIK's interaction mechanics

[Русская версия](vrik-integration-proposal-ru.md) · [Full concept](concept-en.md) · [Implementation status](implementation-status.md)

The idea of Body Slots is to give VRIK a mod-defined set of slots, without its fixed fourteen-slot ceiling or fixed weapon-oriented types, while keeping the actual mechanics of interacting with a slot in VRIK.

## What Body Slots changes

Today a mod can configure VRIK's existing holsters, but it cannot create arbitrary new slots or define a new content/action type as a first-class part of that system. Body Pouches works around this by adapting an existing holster. Body Slots aims to remove that limitation: mods create slots and attach their own rules and responses to interaction, using VRIK's common body and hand interaction instead of implementing another holster system.

For example:

- A potion slot accepts only potions and asks its provider which bottle to offer.
- A food slot routes an interaction through a standardized HIGGS eating action rather than inventing another consumption mechanic.
- An equipment item adds its own slots when worn; removing it disables those slots.

The slot set is defined by installed mods rather than a fixed fourteen-entry table.

![Example of the target workflow: wearing a bag enables its slot; removing the bag disables it. Concept illustration, not a game screenshot.](images/wearable-slot-workflow.svg)

## The slot and the interaction have different owners

**Body Slots manages the slot itself:** creation and removal, content/action type and provider bindings, attachment to an equipped item, placement on the body, saved adjustments and other slot configuration. Slots can be authored through an API or prepared visually in Morphbench. Offline editing is a convenience, not the purpose of the architecture.

**VRIK keeps the interaction mechanics:** the calibrated body and virtual hands, mapping slots onto the actual body, detecting reach and interaction, and its agreed presentation work. It forwards the relevant interaction to the slot's registered behavior. The provider defines the gameplay response; HIGGS retains physical handling and the shared consumption mechanics it supplies. Registering a response does not give a consumer a second input, inventory or physics owner.

## What happens to VRIK's original fourteen slots

They remain part of VRIK. Without Body Slots, they continue working normally.

In the fully integrated Body Slots mode, the original slots are disabled and replaced by fourteen Body Slots-managed counterparts with the same accepted item types and positions. Further mod-defined slots use that same mechanism. VRIK still handles their interaction; there must not be two simultaneously active versions of the same holster. Existing settings and saved assignments need a compatibility mapping so the transition preserves the player's setup.

This is the target design. The current prototype has not completed that handover.

## What exists today

A separate Body Slots plugin and SDK already provide definitions, instances, saved placement and activation, with checks outside the game. The private wearable Body Pouches demonstration uses an existing native13 bridge and still needs game validation. It does not yet deliver arbitrary new-slot interaction, custom action handlers or replacement of all fourteen holsters. [Current status](implementation-status.md).

The [complete private code review](https://github.com/Vhodnoylogin/vrik-reconstruction/pull/2) separates VRIK and Body Slots into two directories and preserves their histories. The [first limited MR](https://github.com/Vhodnoylogin/vrik-reconstruction/pull/1) shows an initial read-only bridge, not the complete goal. Both require access; reconstructed VRIK remains 0.8.7-based, not authoritative maintainer source or an already ported 0.8.8 implementation.

This is the slot part of the [existing cooperation concept](concept-en.md): HIGGS owns input/physical handling, VRIK owns body interaction, PLANCK owns NPC physics, and gameplay mods use their services. CommonLib remains the second stage for shared engine operations.
