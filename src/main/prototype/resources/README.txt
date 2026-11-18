Body Pouches 2 - prototype 9

A wearable potion bag using VR Interaction Framework. This is an untested local
prototype, not an update to the released Body Pouches 0.2.0. Do not install both
versions together: both intentionally use BodyPouches.dll and BodyPouches.esp.

Required separately:
- Skyrim VR with SKSEVR, VR Address Library and normal runtime requirements.
- Released VRIK Player Avatar 0.8.8 (unchanged).
- VR Interaction Framework with BehaviorApi2/GameApi3 and semantic held-release
  integration and native-source validation; its technical runtime filename remains
  BodySlots.dll.
- HIGGS cooperation API build with HeldReleaseInterface1 (including explicit
  FollowInventoryGrip), AuthorizedDispatch2 and InventoryEffectsInterface1. Exact source pins accompany the
  archive. Released HIGGS or earlier prototype builds lack these contracts.
PLANCK is optional for this player-body bag; it is not bundled or required by it.
Keep each dependency as its own mod installation.

Enable BodyPouches.esp. The bag is an Apparel item using biped slot60. It is not
distributed into the world automatically. Find it with help "Potion Bag" 4 and
add one returned ARMO using player.additem <id> 1. Local ID is BodyPouches.esp:803.
Equip it to activate its slot on the right hip; removing it disables new actions.
The vanilla coin purse is displayed by Framework; no VRIK holster is reserved.

Hold a drinkable potion, bring it inside the bag and release grip to store it
and assign its effect filter. With an empty hand, press grip inside the bag and
pull the same hand out while keeping grip held to draw a matching inventory
potion. Releasing inside before exit cancels the draw. Bring a drawn bottle back
and release the original grip to return it; no second press or trigger is needed.
Outside eligible regions, release follows HIGGS's normal drop path. HIGGS handles
actual item transfers and drinking; overlapping native actions share its choice.
Armed draw waits at most60seconds and cancels if its source or item becomes stale.
These controls are implemented in source and compiled, not gameplay-qualified.
Poisons, food and game-equipped weapons are not accepted as potions.

Assignments are per-save and survive removing the bag. Potions remain in normal
player inventory; this prototype is not a separate container. One logical bag is
supported; identical armor copies do not have independent contents.

The first load of a save without BP2A imports only the configured 0.2.0 ASGN
assignment (native slot13 by default). Other legacy slots are not merged. BP2A
including an empty or invalid record prevents legacy fallback. A missing form
rejects the whole filter. A different potion never replaces an assigned filter.
Keep original saves: released0.2.0 cannot read the new BP2A record.

Settings are created at SKSE/Plugins/bodypouches/bodypouches2.json. Edit with the
game closed: enabled, entryHaptics, logLevel, importLegacySlot (0 disables import),
anchor, position, rotationDegrees and radius. Framework's saved placement takes
precedence over these defaults. The released mod's settings/INI are not changed.
Entry vibration is requested through HIGGS and is optional, not proof of hardware
delivery. Invalid new settings disable the bag while retaining the file.

No tests, DLL loading, game launch, deployment or publication were performed for
this prototype. Bag rendering, positioning, handling and save behavior await
actual gameplay qualification. Source commit and exact provider requirements are
recorded in BodyPouches2-PROVENANCE.json; full build instructions stay on GitHub.

Prototype9 retains the lifecycle fixes: it drains cancelled unstarted work after save/world replacement and
retains terminal receipts while final reporting is retried. A cell-world change
does not erase an assignment from factual storage in the same save; receipts
cannot assign another loaded save. Both storage and withdrawal recheck the
actual age of their source observations immediately before admission.
No tests or game runs were performed; this is a compile-only candidate.

Inventory-only cancellation requires positive HIGGS evidence of no recorded
mutation. Unknown or incomplete outcomes stay blocked; the framework does not
infer nonmutation merely because an action was cancelled.
