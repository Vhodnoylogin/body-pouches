# Synergy implementation status

Updated 11 November 2026. This ledger accompanies the [English concept](concept-en.md) and [Russian concept](concept-ru.md). It records source implementation, tests and remaining dependencies.

## Current proposals

| Component | Source and review | Verification |
|---|---|---|
| HIGGS | `a062cb6`, [branch diff](https://github.com/Vhodnoylogin/higgs/compare/main...development), [author PR #9](https://github.com/adamhynek/higgs/pull/9) | Full native DLL, old-SDK discovery of 27 tables, 11 offline suites and native/external haptic regressions. C11/C++20 declarations and released 001 preserved. Author base `93bf67b`. |
| PLANCK | `e5f4dd3`, [branch diff](https://github.com/Vhodnoylogin/activeragdoll/compare/main...development), [author PR #9](https://github.com/adamhynek/activeragdoll/pull/9) | Full native DLL linked, six source/native/C-consumer groups and 19 producer checks. Released 001 matches author `aed524a`. Standalone initialization fails equally before/after; Skyrim/SKSE load unqualified. |
| PPB | `4496302`, [branch diff](https://github.com/Vhodnoylogin/Precision-Physic-Bodies/compare/main...development), [author PR #3](https://github.com/Telord72612/Precision-Physic-Bodies/pull/3) | Production translation unit and exact API/queue/subscription checks pass. Full PPB linking remains unverified. Author source is 2.2.0, `e26b4eb`. |
| Body Slots | `54ba47a`, [development source](https://github.com/Vhodnoylogin/body-slots/tree/feature/extract-body-slots), [RC3 documentation](https://github.com/Vhodnoylogin/body-slots/blob/54ba47af615f1fd6f6c99205bf802f2ad58f9882/docs/release-candidate.md) | Native DLL, 12 release-configuration tests, four extracted SDK examples, discovery checks and reproducible archives. |

HIGGS and PLANCK PR #8 were partial examples and are closed without merging. PR #9 contains the current provider proposals; their remaining contract gaps are listed below. [HIGGS issue #7](https://github.com/adamhynek/higgs/issues/7) introduces the concept and roles. [PPB issue #4](https://github.com/Telord72612/Precision-Physic-Bodies/issues/4) asks about anatomy and physical ownership. Gameplay acceptance and author agreement remain pending. Current work is limited to concept review and HIGGS/PLANCK compaction; Body Slots, PPB, VRIK, Morphbench and CommonLib implementation are deferred.

Each HIGGS/PLANCK fork contains the inspected author baseline on `main` and one working `development` branch. Previous development and partial-proposal heads are retained in archive tags. PLANCK's earlier main contained an author commit subsequently rewritten upstream, and its previous head is also archived.

## Contract and change-scope review

Both concepts define native actions in user priority, stable action identities, originating Up/Cancel, separate public labels and mutation authority, second-hand transitions, equipment outcomes for every affected place, semantic command coverage, partial-effect recovery, safe passive notifications and exact contact attribution. Synchronous copied-only readers need no compulsory registration. Equipment instances are rebuilt under durable keys.

An earlier design review covered five normative cases: native/harvest/pocket competition, a lost receipt after inventory removal, a second-hand transition, two contacts in one physics step and optional provider loss. All 28 pseudo-ABI blocks, containing 80 function declarations, match between languages. Subsequent source review found implementation gaps in common admission and passive surface queries. Both concepts now state the common decision-window requirement explicitly. The [full contract/minimality audit](../trinity-research/abi-boundary-audit-2026-11-11-ru.md) distinguishes target guarantees from delivered code. The [twenty-consumer matrix](../trinity-research/consumer-contract-matrix.md) supplies examples, not a closed consumer list.

Earlier repairs addressed native trigger priority, late-grab release ownership, unrelated grants masking native Up, delayed recognition after delegation changed, equipment resources and asynchronous settlement. The compaction pass restores native haptic first-poll/newest-event semantics, handles same-context replacement at capacity and isolates nonfinite native samples. PLANCK passive observation/history loss no longer vetoes a native hit; explicit policy vetoes survive. No-policy gradual pivot behavior matches the author. An unrelated CustomMenu behavior fix and warning-only cast were removed from the proposals.

HIGGS has 76 changed files and +13927/−119 lines against the author, including 11 existing runtime/header files (+404/−117). The prior proposal had 94 files and +14335/−159. The shared ABI boundary, hand-use helpers and 26→7 public-header grouping remove 368 net source lines while preserving all 27 exports. C11/C++20 checks match 117 record layouts/types, 54 signatures/typedefs, 386 enum values and 75 macros. Experimental source consumers update include paths; compiled old-SDK consumers retain the ABI.

PLANCK has 26 changed files and +2755/−28 lines, previously 30 and +3279/−26. The 526 net lines removed comprise 79 implementation/private-header lines, 434 dependency declarations and 13 provenance lines. Its public headers are unchanged. Direct/compound geometry share lifecycle handling, hit snapshots/evidence share storage, and two contact consumers share one lazy point-velocity sample. These are source-size measurements, not performance claims.

## Implemented roles

HIGGS supplies its closed hand model, input/gesture selection, original press continuation, owned hand uses, physical operations, transaction/equipment routes, motion/contact evidence, hand parts and coordinated haptics. Assigned executors obtain private credentials. Observations carry separate public labels. Experimental tables with changed authority semantics negotiate revision 2.

PLANCK supplies copied poses and generations, owned attachment endpoints, direct/compound capsule proposals applied at its native boundary, drive policies, pre-filter contacts and hit-decision evidence. A causal ID reaches a hit through a connected native path. Passive notifications use bounded main-task delivery and gaps. Numeric policies declare supported fields and hit classes.

PPB supplies actor-specific capsule reads, bounded batches dated at their live read interval, completed digest/raw scans and removable digest subscriptions. Load, overflow, removal and scheduler allocation failure have explicit handling. A normal main frame retries an undelivered gap even with scanning disabled. Existing touch reporting retains its original meaning.

Body Slots supplies logical definitions/instances, saved customization, primitive region evaluation, source validity, eligibility and selected dispatch. It reads actual HIGGS palms and PLANCK body anchors and invalidates replaced sources. Owned source bindings remain distinct from caller-published legacy anchors. PPB digest evidence remains distinct from a physical contact or authoritative surface anchor. Consumers retain gameplay and inventory/content rules.

The native mouth/stash adapter uses actual HIGGS source data and one selected detector. HIGGS keeps standalone detection and the sole consumption/stash executor. Arbitrary button-driven regions use the common recognizer and dispatch. Matching release/cancellation reaches its originating action after a short grant ends, without granting another mutation. An NPC pocket SDK example composes these services.

## Body Slots candidate and structure

RC3 is an unpublished GitHub draft with separate runtime and SDK archives. Its tested pins are Body Slots `54ba47a`, HIGGS `f3a8320`, PLANCK `22f2131` and PPB `4496302`. Those producer commits are ancestors of the current proposals. The compact proposals preserve those binary layouts; their SDK source paths have changed. Existing archives retain their original pins and notices and were not rebuilt or replaced in this pass.

Geometry and transform validation use shared pure helpers. Provider connections read copied observations. Owned binding state applies validated transitions. Dispatch rechecks target, hand, generations and authority before one consumer invocation. Bounded reused scratch storage serves provider reads. Registration removal drains callbacks before releasing code/context. These properties are separate from measured game performance.

The name recommendation is [Interaction Regions VR](https://github.com/Vhodnoylogin/body-slots/blob/54ba47af615f1fd6f6c99205bf802f2ad58f9882/docs/naming.md). It covers player, NPC, equipment, world and temporary regions. Product naming remains for owner review.

## Remaining integration

- A precise trigger-originated draw policy needs a qualified semantic command provider. Current native coverage is zero. Registration, coordination and outcome contracts are implemented.
- Unconventional exact equipment layouts need a capable executor proving every affected place and instance. The new multi-place service advertises zero native layout coverage. Ordinary inventory transfer remains separately available.
- PPB model/body generations, exact PLANCK contact joins and safe publication from 2.9 private worlds need author integration. A scan ID supplies none of those guarantees.
- Stock VRIK remains usable through the independent scene adapter. A certified completed-pose stamp is an optional extension. Simulated accessories have their own sample phase.
- HIGGS button, queued-nonbutton and immediate hand-use admission still have separate closure points and priority keys. A low-priority immediate request can precede a high-priority proposal. Native selection followed by nonbutton admission before held-state publication can also admit competing actions. These are unresolved contract gaps, not a shared global-priority capability. Body Slots dwell and compaction prototypes remain preserved and deferred.
- A generally callable passive HIGGS surface query is absent. The raycast inside a mutating support request is not an equivalent observation capability.
- Advanced spatial queries and consumer ports remain capability-specific work. Released Body Pouches 0.2.0 does not automatically acquire the new services. Its wearable demonstration is a separate experimental branch.

These builds have no in-game acceptance. Hardware input, rendering, physics and actual save/load need their own qualification. Testing-tool custody belongs to CLAW.

## CommonLib and history

The [CommonLib assessment](../trinity-research/commonlib-assessment.md) records available wrappers and stage-two candidates. The provider changes retain the author baselines' literal relocation sets, 247 unique addresses in HIGGS and 440 in PLANCK. No CommonLib implementation or Address Library mapping was changed.

This folder contains its README, both language versions and this ledger. [Research](../trinity-research/README.md) and [history](../trinity-research/implementation-history.md) are separate. The [previous ledger](https://github.com/Vhodnoylogin/body-pouches/blob/b4425e2998a1c55970da081bbb6da19fda61b4e3/docs/trinity-synergy/implementation-status.md) retains earlier detailed build records and partial proposals in Git history.
