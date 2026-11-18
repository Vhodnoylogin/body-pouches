Body Pouches 2 - prototype 2

A wearable potion bag using VR Interaction Framework. This is an untested local
prototype, not an update to the released Body Pouches 0.2.0. Do not install both
versions together: both intentionally use BodyPouches.dll and BodyPouches.esp.

Required separately:
- Skyrim VR with SKSEVR, VR Address Library and normal runtime requirements.
- Released VRIK Player Avatar 0.8.8 (unchanged).
- VR Interaction Framework 0.2.0-rc.4 from source a9b3ac9, whose technical
  runtime filename remains BodySlots.dll.
- HIGGS cooperation API build from fd226f6546877fae8b82380ce16aa800000d12ad.
  Released HIGGS alone does not supply the required inventory/input contracts.
PLANCK is optional for this player-body bag; it is not bundled or required by it.
Keep each dependency as its own mod installation.

Enable BodyPouches.esp. The bag is an Apparel item using biped slot60. It is not
distributed into the world automatically. Find it with help "Potion Bag" 4 and
add one returned ARMO using player.additem <id> 1. Local ID is BodyPouches.esp:803.
Equip it to activate its slot on the right hip; removing it disables new actions.
The vanilla coin purse is displayed by Framework; no VRIK holster is reserved.

At the bag, hold a drinkable potion and press the trigger to store it and
assign its effect filter. With an empty hand, grip at the bag to withdraw a
matching potion from ordinary player inventory. Move out while holding grip,
then release outside. HIGGS handles the actual item and drinking at the mouth.
To return a drawn bottle, bring it to the bag and press the trigger while
holding it. Releasing the grip remains the original action's release; it never
also stores the bottle. Only the selected trigger action at this bag is consumed.
Poisons, food and game-equipped weapons are not accepted as potions.

Assignments are per-save and survive removing the bag. Potions remain in normal
player inventory; this prototype is not a separate container. One logical bag is
supported; identical armor copies do not have independent contents. Older 0.2.0
native-slot assignments are not migrated. Keep original saves before use.

No tests, DLL loading, game launch, deployment or publication were performed for
this prototype. Bag rendering, positioning, handling and save behavior await
actual gameplay qualification. Source commit and exact provider requirements are
recorded in BodyPouches2-PROVENANCE.json; full build instructions stay on GitHub.
