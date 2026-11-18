# Body Pouches 2 prototype

The prototype replaces Body Pouches' native VRIK slot integration with a consumer of VR Interaction Framework. One equippable potion bag activates one framework-owned slot on the right hip. Body Pouches retains potion acceptance, effect-based assignment, inventory selection and assignment persistence. Framework owns the slot, geometry, saved placement and presentation; HIGGS owns input arbitration, transfer into the hand, storage and drinking. Unchanged public VRIK supplies the body observation. This player bag needs no PLANCK or PPB capability.

## Scope

The bag is `BodyPouches.esp:000803`, one unique ARMO on biped slot60. The inventory/world model references Skyrim's `Plants\CoinBagLarge.nif`, the vanilla coin-purse visual identified through the owner's IED reference. Framework displays that same model while the equipment gate is satisfied. No game/IED asset, reconstructed VRIK implementation, VRIK Art ability or native holster allocation is included. A body-anchor binding starts at pelvis-local position `(18, 0, -4)`, with a radius10 access sphere. Visual positioning and ergonomics remain unqualified.

Equipping the bag enables new offers. Removing it stops new offers without deleting assignment or claiming that an already delivered potion was returned. A successfully stored drinkable potion assigns an unassigned bag's effect filter; withdrawal selects a matching potion from ordinary player inventory. A different potion never replaces an existing filter. Food, poison and a game-equipped weapon are rejected. Saved placement belongs to Framework; the bag's logical assignment belongs to Body Pouches. Several copies of this armor form share the declared singleton key and do not become independent containers.

The prototype4 gesture follows the released default pouch: release a drinkable bottle inside the bag to assign its effect filter or return a matching potion. With an empty hand, press grip inside the bag and pull the same hand out while keeping grip held to receive the selected potion. Releasing before exit cancels the unstarted draw. A withdrawn bottle can be carried, brought back and returned with its original grip release; a release outside eligible regions resumes ordinary HIGGS dropping. HIGGS mouth/stash actions and the bag compete through one admission window. No trigger storage, local raw-button hook or post-drop regrab is used. These are source-implemented controls, not a gameplay-qualified parity claim. The old native13 demo and released0.2.0 paths remain separate build choices.

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
| Release a held bottle in the bag to assign/return it | Implemented in prototype4 through an optional HIGGS semantic pre-drop intent. The original hardware Up remains owned by its original sequence. |
| Draw, carry away, bring back and release without another grab | Implemented in prototype4 with explicit per-transfer `FollowInventoryGrip`, after factual grip confirmation. This does not alter old inventory consumers. |
| Grip in the slot, withdraw while holding, then receive the bottle | Implemented in prototype4 by Framework deferred draw: retain the selected stack, validate the original held press and source, then transfer on exit. Early release cancels. |
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

### Release and deferred-draw contracts

A hardware Up is still an observation of the original press, not a second permission. The new optional HIGGS `HeldReleaseInterface` offers a distinct semantic intent before a real held item is dropped. It uses the same ordered arbitration/routing and native mouth/stash window. HIGGS retains the original release through one factual hand publication so Framework's region and the consumer's newly held potion metadata can catch up. No offer restores native release; pending or committed/uncertain transfers cannot fall through into another drop. Native transaction-driven releases bypass the offers to avoid recursion.

`FollowInventoryGrip` opts one factually completed withdrawal into original-release handling. It validates the owned receipt and original press, preserves a terminal received during delivery, is idempotent per transfer and leaves legacy consumers unchanged. Hand possession is tracked separately from changes to game-equipped weapons.

Framework's additive BehaviorApi2 explicitly opts a behavior into semantic release and deferred exit, supplies its grant-duration bound and copies the still-valid original source outside the access region. GameApi2 reports actual optional-service readiness. Old tables and behaviors retain their semantics. Drawing pins a read-only native selection while armed; no inventory mutation or extra world bottle occurs until exit. The original still-held sequence, source and native transfer request are rechecked together through HIGGS's existing authorized-dispatch fence. The wait is bounded to60seconds. Removing equipment, changing configuration/content/body/anchor, losing observations or releasing early cancels unstarted work.

The migration exposed and addressed additional contract gaps: quick native pickup/release before the first published grip, entering a bag and releasing within one update, exiting between Down and admission, stale10Hz held metadata and an Up racing with transfer admission. Stock observations remain throttled; a changed held identity triggers immediate metadata refresh. Actual native stack delivery, rendering and ergonomic parity remain unqualified because no tests or game runs were requested. Multiple bags, additional item domains and advanced legacy configurations remain later consumer work.

## Pre-test hardening in prototype5

The bag now refreshes current Framework eligibility on every safe update, independently of inventory scanning. A recovered anchor or newly available slot can accept a quick release without waiting for the previous100ms stock timer. A dedicated `PotionObserver` owns engine observations: immutable inventory snapshots are refreshed by player-container events, own-transfer/lifecycle invalidation and a one-second fallback. Held metadata follows copied HIGGS identity immediately and retains a100ms refresh for in-place form edits. Event callbacks only invalidate the cache; they never read or mutate inventory. No performance measurements or gameplay claims are made.

Before proposing first storage, the main phase runs the same pure assignment/codec preflight used after the receipt. An unrepresentable initial filter is rejected before storing the potion. The immutable policy callback only reads the precomputed result and does not allocate. Existing effect matching, ranking and the BP2A save format remain unchanged.

An armed draw now survives another slot's content, binding or anchor changes. Framework invalidates accepted work only for its own source, body, anchor, hand observation or behavior loss; restoring that source does not revive the old action. Visual-only updates retain geometry and no longer synthesize an exit/entry pair.

HIGGS ends authority at the original deadline but retains exact admitted child receipts for factual settlement. A completed transfer can therefore be accounted after natural expiry, without another transfer or an extended grant. Original-grip continuation additionally requires the same original context and possession episode. If that continuation is no longer available, the helper reports `releaseStatus`, settles the already completed delivery and the consumer logs the limitation. It does not invent a press, drop, refund or second item. A child cancelled before completion remains subject to the existing compensation/recovery protocol. Tests, DLL loading and game runs remain disabled by the owner's instruction.

## Scoped completion and freshness in prototype6

The consumer captures its assignment generation together with the filter before offering storage. Revert/load retires that generation under the same assignment-store mutex used for commit and save. Loading another save cannot apply a previous receipt, even if Framework scope observations have not caught up. A cell's physics-world change can retire HIGGS authority while remaining in the same logical save; it does not erase a valid stored-potion assignment. Retirement and temporarily unavailable-player paths consume successful progress before detaching, and a successfully drained lifecycle clears its temporary stop.

Framework cancels and releases unstarted old-scope work after a positively observed save/epoch transition, without demanding a content mirror for a vanished source. Uncertain admitted transfers still require recovery. Exact observed terminal receipts are retained locally for cleanup retries; no new transfer or automatic refund is issued. Removing a slot in the same save permits only authenticated settlement of that exact pending history, never a replacement slot. Terminal completion must match the earlier committed projection.

GameApi3 supplies the actual spatial observation-start time. Both storage and withdrawal revalidate source freshness immediately before admission through HIGGS's existing authorized-dispatch fence. Storage preserves the accepted release gesture: the hand need not stay inside the bag afterward, but a stale/invalidated source cannot start new work. The old first-seen-frame timer is removed. HIGGS changes remain limited to one private receipt-acknowledgement flag and retention/cleanup at existing lifecycle points; all its public table layouts and exports remain unchanged. Its parent, child and transaction history survive terminal reporting until the consumer's explicit `ReleaseGrant` acknowledgement.

These are source changes under the owner's no-testing instruction. Compilation/linking is recorded separately and does not qualify actual loading, inventory, save behavior or ergonomics.

The SDK now waits for Body Pouches to acknowledge its semantic outcome before rearming that bag. Completed protocol reporting releases physical authority first; a separate `Settling` status retains only the owned receipt while save/assignment metadata is temporarily unavailable. Retrying it never transfers another item or depends on already released provider history. A narrow immutable context pins the already preflighted assignment to the exact accepted plan. Completion does not reread a potion form or repeat codec preflight; loading can postpone application until its generation is either current or positively retired. Policy is suppressed until the settled filter is published, closing the interval where another offer could observe the old unassigned bag. Confirmed provider reconciliation is distinguished from unresolved mutation by one shared pure receipt helper; no automatic reconciliation is performed.

HIGGS's optional accepted-receipt capability lets this queued consumer explicitly retain even an unstarted action's cancellation proof. An exact current `ACCEPTED` echo records responsibility for processing it, without creating a transfer or extending hand authority. Other clients keep their old finite-history behavior. Tracking loss or removal of only the spatial source also ends physical authority while preserving an admitted child's truthful receipt channel; the owning consumer's explicit teardown remains separate.

The ordinary provider now declares an explicit inventory-only content contract through 24-byte Framework behavior options. Confirmed release into the world remains a physical fact, but no longer falsely implies that a potion entered inventory. If storage is cancelled before mutation, unchanged content can be acknowledged without creating a refund or duplicate bottle. Positive inventory evidence is sticky and required for typed content commitment; generic behaviors keep their conservative rules. Body Pouches checks these capabilities at startup so an older mixed dependency set does not enter silent attachment retries.

Draw/store identities presented to HIGGS are now the stable ordinary transfer kinds, while each offer retains a separate cache token for exact execution. User action priorities no longer follow an incrementing offer number. The identity mode is explicit in the extended options; legacy providers keep their existing 64-bit discriminator.

Saving may project an already observed, factual successful storage receipt and its captured filter into BP2A when the live filter is still unset and the assignment generation matches. This is a copy-only serialization path: it does not alter live policy, processed IDs or helper work, and does not call inventory/provider dispatch. A progress copy without terminal factual evidence cannot authorize the projection. Contexts are prepared on the safe main phase; the controller policy callback only copies their shared pointers.

Explicit reconciliation after an uncertain native mutation is still a bounded recovery limitation: if HIGGS has already terminally cancelled the original broker action, a later transaction reconciliation cannot reopen that action for content mirroring. The helper retains the recovery stop rather than replaying the operation or fabricating success. This prototype supplies no automatic reconciliation or refund.

## Positive inventory outcomes in prototype7

The common validator no longer treats a cancelled action's missing inventory flag as proof of nonmutation. HIGGS's additive read-only `InventoryEffectsInterface1` reports closed, correlated recorded inventory effects without changing old receipt flags or layouts. Frame accepts typed unchanged cancellation only with positive evidence (or confirmed compensation); active, missing, uncertain or incomplete child history remains reserved. A callback that has explicitly failed can release a known nonmutating reservation, but a still-running callback and accepted consumer work cannot. Body Pouches requires the observation service; no extra inventory or button mechanism is introduced.

## Assignment projection in prototype8

The framework content is the pouch's logical assignment, rather than an inventory count. Unassigned is `0/0`; an assigned filter has a deterministic provider-domain identity and cardinality `1`. Exact filters and canonicalized effect sets retain their identity across draws, matching returns and stock refreshes. The identity is an opaque noncryptographic fingerprint, not a Skyrim form/reference or mutation credential. First successful storage projects the already preflighted target assignment; later actions keep it. Zero stock never clears that assignment.

Inventory stock still determines which potion can actually be drawn and which preview model is shown. Unknown stock suppresses withdrawal and its preview, but does not suppress known assignment or the bag holder. Publishing `0/0` merely to hide an unobserved post-transfer stock count has been removed: unknown count was not proof of empty inventory. Projections are prepared on the safe main phase; controller policy only copies them. Immutable per-hand settlement contexts are reused while their full filter, save/generation and held-item descriptors match. Assignment mutation now has one captured-generation entry point, avoiding an unfenced setter and repeated filter copies. This consumer correction needs no new HIGGS or Framework API and does not change BP2A, matching, ranking or grip gestures.

## Building

Production sources are in `src/main/prototype/`. The separate CMake entry consumes only the public framework tables and its pinned SDK helper. It reuses existing pure Body Pouches potion rules and its form-description boundary, together with the completed RC4 build's CommonLib/fmt/spdlog libraries. It does not compile legacy input, VRIK holster or inventory-mutation code. Generated ESP/DLL/ZIP outputs stay outside Git.

Run `python src/main/prototype/build.py --output <external-build> --rc4-native <completed-RC4-native> --framework-root <framework-checkout> --higgs-root <HIGGS-checkout> [--json-root <nlohmann-json-source>]`. Exact new Framework and HIGGS source revisions are pinned by `FRAMEWORK_COMMIT` and `HIGGS_COMMIT` in the build script and copied into both build and package provenance. Older RC4/prototype3 binaries do not provide the new optional services. The RC4 cache must identify compatible x64 Release CommonLib/fmt/spdlog libraries and their existing source directories. Configuration uses the existing nlohmann single-header dependency from the sibling cache or explicit source root; the exact header/license hashes are recorded in the build receipt. The script configures, compiles and links only; no test, LoadLibrary probe, deployment or game start is performed.

After committing the source, build that clean revision and run `python src/main/prototype/package.py --build-manifest <external-build>/prototype-build-manifest.json --output <new-external-package-directory>`. The runtime archive contains this prototype's DLL, generated ESP, brief instructions, provenance and required notices. Framework, HIGGS and VRIK remain separate dependencies. The ESP generator reuses the repository's maintained record encoder and creates only the bag's record; no generated game file is committed.

## Delivery state

This is a prototype source/build delivery. No tests were run at the owner's request. Native linking is not proof of SKSE initialization, worn rendering, successful transfer, comfortable input or save/load behavior. No MO2 installation, automatic-test order, Nexus upload, publication is part of this change. HIGGS source changes belong to its existing cooperation proposal; PLANCK, PPB and VRIK are unchanged. Keep the released Body Pouches installation mutually exclusive with this prototype when later authorized for a game run.
