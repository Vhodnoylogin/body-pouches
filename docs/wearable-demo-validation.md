# Wearable demo: game acceptance plan

This is a subject test plan for the private wearable build, not a completed test report. The shared game-testing platform must first be qualified and installation/restoration ownership established. Do not change the live MO2 profile or start Skyrim merely because this file exists.

Use the exact reviewed demo archive and provenance, ordinary released VRIK/HIGGS, a disposable save and an outfit leaving equipment slot60 free. Native13 belongs exclusively to this demonstration. Preserve the source profile/save, original Body Pouches files and relevant VRIK settings for restoration. The platform's own tools and launch details are selected separately by the testing system.

Start factual subject coverage only after the fixture is loaded and the initial **unworn bag** checkpoint can be observed. A successful launch is not acceptance of the mod. Close menus before reading `body_pouches_state`; loading, playroom, paused/menu and missing-world observations remain unavailable. Collect actual inventory/equipment observations, Body Pouches state/causal history, Body Slots status and separate images or headset observations of the Art.

| Check | Action | Required evidence |
|---|---|---|
| Unworn baseline | Have exactly one demo bag in inventory without equipping it; approach native13 with an empty hand and a held drinkable potion | Actual worn=false; no new pouch draw/pickup or bag Art. Logical instance is inactive when the provider is available. Unavailable provider data is reported explicitly |
| Equipping | Equip the unique bag, close the inventory and allow ordinary callbacks | Actual worn=true, current nonzero save/view/instance revisions and requestedActive/effectiveEnabled=true; visible coin bag at native13. Record Art-node evidence separately from the image |
| Assignment and both hands | Release a known healing potion in native13, then draw/return with each physical hand and drink one through HIGGS | Actual assigned filter, one removed bottle per draw, actual HIGGS possession, matching return quantity and expected consumption. No duplicated/deleted stock; maintain inventory + held/world disposition accounting |
| Unequipping | Remove the bag, close menus and repeat the approach/release | Actual worn=false and inactive instance; bag visual disappears and new pouch actions stop. Existing assignment remains |
| Reequipping | Put the same unique item back on | Current instance becomes active again; the saved filter still selects the expected potion. No delayed action from the previous equipment epoch executes |
| Deferred cancellation | Use a bounded, supported test boundary to remove/reequip between the original draw/release event and its queued task; if that boundary cannot be controlled, mark this coverage unavailable | Equipment epoch changes; the original token is rejected; no RemoveItem/PickUpObject side effect from that old event and no filter assignment from a rejected first return |
| Already delivered potion | Draw a potion, confirm it actually reached the hand, then remove the bag | The delivered item remains in its ordinary HIGGS/world disposition. Bag removal neither deletes/refunds it nor forces a drop |
| Save/load | Save with bag worn and assigned, reload; repeat saved unworn and after a complete game restart | Current actual equipment controls reconstructed activation; fresh scope/handles, retained expected filter, no old queued work, correct independent Art state |

Use a separate bounded missing-template/provider fixture only when installation ownership permits changing dependencies between runs. It must refuse new actions instead of silently granting access; restore the normal fixture afterward. An inactive or invisible bag must not be mistaken for restored native weapon use: this compatibility build retains the dedicated native13 suspension for the session.

End with game termination, restoration and verified evidence collection. Record each check as passed, failed, unavailable or not run, with the source/binary pins and actual game-start count. Never infer actual equipment from requested activation, visible output from a node, a successful inventory operation from a request trace, or generic new-slot compatibility from this native13 bridge.
