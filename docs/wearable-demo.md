# Private wearable potion-bag demonstration

This branch is a demonstration for discussion with VRIK's maintainer. It is based on Body Pouches 0.2.0 and the existing [synergy concept](trinity-synergy/concept-en.md). The published mod and its archive remain unchanged. Do not upload this package as a public Body Pouches update.

## What it demonstrates

One new inventory item, **Potion Bag (Body Slots Demo)**, occupies equipment slot 60. Body Pouches observes the player's actual equipped armor and creates one logical Body Slots instance. Equipping requests activation; removing the bag requests deactivation. Missing interfaces, templates, equipment observations or current revisions prevent new pouch actions.

Released VRIK still supplies native slot **13**, its calibrated placement, hand reach, holster callback and Art presentation. HIGGS still handles the physical potion. The existing pouch selection, drawing, drinking, returning and assignment persistence are reused. The bag remains visible when empty; it replaces the old potion preview in this private build.

The visual reference supplied by the owner was Immersive Equipment Displays (Nexus 62001). Its Coin Pouch profile refers to Skyrim's own `CoinPurseLarge`, whose model is `Plants\CoinBagLarge.nif`. The demo references that installed base-game model. It contains no IED binary, copied game mesh or reconstructed VRIK DLL. The inventory/world model and VRIK Art use the same model; no separate armor-addon renderer is introduced.

## Deliberate limits

- One unique bag form and one logical equipment key. Several identical physical copies are not independent containers or independent persistent identities.
- Native13 remains dedicated for the session, including while the bag is removed. The demo prevents its pouch actions and removes its Art; it does not attempt to clear another mod's unowned legacy VRIK suspension flag.
- Move native13 in VRIK to position the bag. The authored Body Slots template supplies identity and eligibility; its independent placement geometry does not reposition released VRIK in this bridge.
- Potions remain in the player's normal inventory. Removing the bag preserves its potion assignment, and a potion already delivered to the hand remains an ordinary held item. No potion is refunded, deleted or force-dropped by bag removal.
- Deferred draw/return work keeps the original equipment epoch and Body Slots scope/view/instance revisions. Removing and reequipping the bag does not authorize an old queued action.
- Configuration uses a single exclusive pouch and permits enabling native13 in VRIK memory for this demo. It is not an installation intended to share native13 with a weapon or another pouch mod.

## Build and stage

Configure an external build directory with `BODYPOUCHES_WEARABLE_DEMO=ON`, `BODYPOUCHES_BUILD_TESTS=ON` and, for observation, `BODYPOUCHES_WITH_DEVBENCH=ON`. Build Release normally. `tools/make-wearable-demo.py` writes a **new** demo ESP and refuses to replace an existing file; do not use the ordinary release staging script for this build.

The private Data-based package contains the newly built `SKSE/Plugins/BodyPouches.dll`, demo `BodyPouches.esp`, the tested Body Slots DLL, and `SKSE/Plugins/BodySlots/body-pouches-demo.bslots` compiled from the demo template using the Body Slots SDK's `author.py`. Include the existing OpenVR and DevBench license notices, this description and source/binary provenance. Raw JSON is an authoring input, not a runtime package. Ordinary VRIK/HIGGS, SKSEVR, VR Address Library and their usual requirements must already be installed.

Use an isolated test save and mutually exclude the normal Body Pouches installation when a qualified game-test reservation is available. A useful starting item command is `help "Potion Bag" 4`, followed by `player.additem <the returned ARMO ID> 1`; its local ID is `BodyPouches.esp:000803`. Equip it from Apparel. Assign a drinkable potion by releasing it in native13, then use the existing grip/withdraw interaction. Unequip and re-equip while a draw or return is pending, and repeat across save/load. Slot60 may conflict with other equipment; choose a test outfit without that slot.

## Evidence and acceptance

Offline checks compile the full demo DLL, exercise its actual Body Slots consumer against the actual service DLL and explicit failure boundaries, inspect ESP records/master links, and check stale queued work through production method fixtures. They do **not** establish that the armor equips, the Art is visible, the controls are comfortable or potion transactions succeed in Skyrim.

The optional existing `body_pouches_state` observation adds `wearableDemo`: actual worn status, negotiated-interface availability, activation/eligibility, exact revision strings and a separate bounded Art-node observation. A logical active instance or an Art node is not rendered-pixel proof. The display, both hands, unequip/re-equip cancellation, inventory totals and save/load require an actual qualified game run before calling this a working demonstration.
