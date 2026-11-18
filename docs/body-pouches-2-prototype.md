# Body Pouches 2 prototype

The prototype replaces Body Pouches' native VRIK slot integration with a consumer of VR Interaction Framework. One equippable potion bag activates one framework-owned slot on the right hip. Body Pouches retains potion acceptance, effect-based assignment, inventory selection and assignment persistence. Framework owns the slot, geometry, saved placement and presentation; HIGGS owns input arbitration, transfer into the hand, storage and drinking. Unchanged public VRIK supplies the body observation. This player bag needs no PLANCK or PPB capability.

## Scope

The bag is `BodyPouches.esp:000803`, one unique ARMO on biped slot60. The inventory/world model references Skyrim's `Plants\CoinBagLarge.nif`, the vanilla coin-purse visual identified through the owner's IED reference. Framework displays that same model while the equipment gate is satisfied. No game/IED asset, reconstructed VRIK implementation, VRIK Art ability or native holster allocation is included. A body-anchor binding starts at pelvis-local position `(18, 0, -4)`, with a radius10 access sphere. Visual positioning and ergonomics remain unqualified.

Equipping the bag enables new offers. Removing it stops new offers without deleting assignment or claiming that an already delivered potion was returned. A successfully stored drinkable potion assigns an unassigned bag's effect filter; withdrawal selects a matching potion from ordinary player inventory. A different potion never replaces an existing filter. Food, poison and a game-equipped weapon are rejected. Saved placement belongs to Framework; the bag's logical assignment belongs to Body Pouches. Several copies of this armor form share the declared singleton key and do not become independent containers.

An empty-hand grip press currently withdraws a bottle. A trigger press with a drinkable potion at the bag stores it, assigning an unassigned bag or returning a matching bottle. These are temporary prototype controls, not the familiar release-to-assign/return gesture of 0.2.0. In particular, the original release of a selected withdrawal is still consumed by HIGGS and must not be advertised as an ordinary drop outside the bag. This prototype deliberately reuses HIGGS's shared executor through Framework's ordinary-inventory SDK helper instead of adding a competing drop detector or controller hook. The old native13 demo and released0.2.0 paths remain separate build choices.

## Comparison with released 0.2.0

The baseline is release tag `0.2.0`, source `699196120d3c72e5eb463bf98a9c025e8d98c1f3`. The comparison below is based on source and the release's maintained quick start, not a new gameplay test.

| Mechanic | Current Framework consumer |
|---|---|
| Normal player inventory; no second container | Retained. The preview is not another item. |
| All seed effects with OR matching; exact form for an effectless potion | Retained through the existing pure filter rules. |
| Fewest effects, then weakest maximum magnitude, then stable form identity | Retained. |
| First assignment persists after empty stock or removal | Retained; a different effect does not silently reassign. Released0.2.0 has no reassignment gesture either. |
| Both hands, exact held stack quantity, HIGGS drinking | Routed through public HIGGS services; gameplay remains unqualified. |
| Saved0.2.0 assignment | Added in prototype3: import the explicitly selected native slot, default13, only when no BP2A record is present. |
| Position/rotation/radius settings and disable switch | Added in prototype3 as Framework-authored defaults; existing saved Framework overrides take precedence. No VRIK INI or native holster is changed. |
| Entry vibration | Added in prototype3 using Framework Enter observations and HIGGS's coordinated haptic service. Missing feedback does not block the potion operation. |
| Release a held bottle in the bag to assign/return it | **Not yet transferred.** Current trigger storage is a temporary fallback. A native grab's matching Up cannot be consumed by another action. |
| Draw, carry away, bring back and release without another grab | **Not yet transferred.** The withdrawal's original Up gives no new mutation authority and currently does not drive a native drop. |
| Grip in the slot, withdraw while holding, then receive the bottle | Still an immediate press withdrawal. Matching the old pull-out timing needs a shared continuation, not a new local controller detector. |
| Advanced multiple/shared native slots and translated log settings | Not ported. One wearable Framework bag replaces the accepted default pouch, without claiming all advanced configurations. |

### Assignment migration and settings

`SKSE/Plugins/bodypouches/bodypouches2.json` is created on first initialization. It is separate from released `bodypouches.json`, which is never rewritten. Settings are read between game sessions. The defaults are:

```json
{
  "enabled": true,
  "entryHaptics": true,
  "importLegacySlot": 13,
  "logLevel": "info",
  "anchor": "NPC Pelvis [Pelv]",
  "position": [18, 0, -4],
  "rotationDegrees": [0, 0, 0],
  "radius": 10
}
```

Position and radius use Skyrim units in the selected bone's local space; rotation uses degrees, applied as row-major `Rz * Ry * Rx`. They are authored defaults supplied to Framework. Per-save customization already saved by Framework still wins. `enabled:false` disables this consumer; an explicitly empty old `pouches:[]` is respected when creating the new settings for the first time. Invalid new settings disable the bag and leave the file intact. Native shared/exclusive mode and native placement do not migrate as Framework geometry.

`importLegacySlot` selects exactly one legacy assignment; `0` disables import. Default13 maps the released stomach pouch's filter into the new logical bag, without allocating that VRIK slot. No other native assignments are merged or selected by guesswork. A valid empty BP2A record overrides an old ASGN record; corrupt, duplicate or unsupported BP2A does not fall back to ASGN. Missing forms reject the complete filter. On the next save the imported filter uses BP2A; no handles, inventory stock or activation are imported. The original save pair should be kept if a later return to released0.2.0 is needed, because that release does not understand BP2A.

### Remaining release contract

Switching the helper's `storeEdge` to Up cannot restore the released behavior. HIGGS closes a selected native/external Down sequence before recognizing its Up, and the terminal event/sequence copy conveys observation rather than authority. This affects ordinary native-held potions as well as bottles withdrawn by this consumer.

The missing provider capability is an optional semantic release offer before HIGGS physically drops the item. It should use the existing pre-drop native consume/stash admission boundary and the same shared hand-use decision window, preserving standalone default release when no external offer is selected. HIGGS must also associate a factually completed inventory-to-hand withdrawal with its original input sequence, so its matching release can enter that same path. Framework supplies the current slot and acceptance context; Body Pouches supplies the potion rule; HIGGS retains release, inventory execution, real receipts and uncertain-mutation handling. One intent must not become repeated offers or a second storage operation. Cancellation may resume an uncommitted original release once; uncertain inventory mutation must not blindly fall through to dropping. This provider extension is **not implemented** in prototype3, and the author PRs have not been changed for it.

## Building

Production sources are in `src/main/prototype/`. The separate CMake entry consumes only the public framework tables and its pinned SDK helper. It reuses existing pure Body Pouches potion rules and its form-description boundary, together with the completed RC4 build's CommonLib/fmt/spdlog libraries. It does not compile legacy input, VRIK holster or inventory-mutation code. Generated ESP/DLL/ZIP outputs stay outside Git.

Run `python src/main/prototype/build.py --output <external-build> --rc4-native <completed-RC4-native> --framework-root <framework-checkout> --higgs-root <HIGGS-checkout> [--json-root <nlohmann-json-source>]`. The exact framework source is `88e29961747f3e28e33e1027ab93c1da9b5a278a`; the runtime RC4 implementation remains `a9b3ac9c7d1cfc3bebc05f5bb8bfbbbfe06bfd61`. HIGGS contracts are pinned to `fd226f6546877fae8b82380ce16aa800000d12ad`. The RC4 cache must identify compatible x64 Release CommonLib/fmt/spdlog libraries and their existing source directories. Configuration uses the existing nlohmann single-header dependency from the sibling cache or explicit source root; the exact header/license hashes are recorded in the build receipt. The script configures, compiles and links only; no test, LoadLibrary probe, deployment or game start is performed.

After committing the source, build that clean revision and run `python src/main/prototype/package.py --build-manifest <external-build>/prototype-build-manifest.json --output <new-external-package-directory>`. The runtime archive contains this prototype's DLL, generated ESP, brief instructions, provenance and required notices. Framework, HIGGS and VRIK remain separate dependencies. The ESP generator reuses the repository's maintained record encoder and creates only the bag's record; no generated game file is committed.

## Delivery state

This is a prototype source/build delivery. No tests were run at the owner's request. Native linking is not proof of SKSE initialization, worn rendering, successful transfer, comfortable input or save/load behavior. No MO2 installation, automatic-test order, Nexus upload, publication or resumed provider PR work is part of this change. Keep the released Body Pouches installation mutually exclusive with this prototype when later authorized for a game run.
