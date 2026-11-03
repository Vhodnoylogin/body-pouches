# Synergy implementation status

Updated: 3 November 2026. This ledger accompanies the [English concept](concept-en.md) and [Russian concept](concept-ru.md). The concept describes the target architecture; source implementation, native linking, consumer adoption and running-game acceptance are separate evidence levels.

## Interaction regions and slots

The owner accepted the region/slot standard after the [twenty-consumer investigation](../trinity-research/interaction-regions-review-ru.md). Both concepts now define a slot as a named interaction region with offered actions and activation rules. Storage, presentation and persistence are optional. Body Slots owns logical spatial evaluation and slot mechanics; HIGGS owns hand state/use, input, grants and physical operations; VRIK supplies body/virtual-hand poses; PPB supplies anatomical geometry/fitting/query evidence; PLANCK owns NPC physics, physical contacts and hits. Consumer gameplay remains external.

[Body Slots development source](https://github.com/Vhodnoylogin/body-slots/tree/feature/extract-body-slots) implements the first additive [interaction-regions table](https://github.com/Vhodnoylogin/body-slots/blob/feature/extract-body-slots/docs/interaction-regions.md), with unchanged layouts for older negotiated tables:

- Live-writer-owned named regions with exact revisions, copied snapshots and bounded atomic publication.
- Player/NPC/world/transient anchor kinds; externally supplied anchor and point-probe observations with declared provider/source/target identity, frame, time, units and generations.
- Shared sphere/capsule/box/finite-cylinder geometry; flat endcaps, proper rigid transforms and positive uniform scale. Existing saved slot shapes also support cylinders through an advertised capability.
- Separate entry/exit hysteresis and spatial dwell, with explicit monotonic time and continuity limits.
- Missing, stale, incoherent or repeated samples are unavailable. Generation changes, omissions, rollback, runtime invalidation and save changes break dwell.
- Copied last-evaluated observations, including spatial readiness. They do not authenticate external providers, establish present freshness, create physical contacts or grant hand authority.

Independent review reproduced and verified repairs for two defects: a direct save reset preserving old readiness, and pruning a live writer during a failed release. The registry owns actual save generation and lease existence; the region service observes them through narrow private getters. Runtime scope acceptance and invalidation use the same runtime lock. The simultaneous scheduling case was reviewed in source; the two concrete reset/release counterexamples were independently replayed.

## Verification

The next bounded integration connects HIGGS's actual native mouth and shoulder inventory-stash source frames to Body Slots. HIGGS exposes an additive optional spatial-detector table; Body Slots negotiates it at runtime and owns the registered region checks. No Body Slots startup/link dependency is added to HIGGS, and the spatial connection does not require the VRIK-backed player-slot adapter. Missing or incompatible services select HIGGS's native fallback. A valid handled result, including outside/unavailable zero, does not invoke that fallback. HIGGS keeps eligibility, velocity gates, hand authority and the sole consume/stash/drop executor. Transactional physical release still suppresses auto-consumption/stash.

Published source: [HIGGS `3deb5bf`](https://github.com/Vhodnoylogin/higgs/commit/3deb5bfbcfe1c40d8b2318bc331f5121eefaf399) and [Body Slots `c1b38fc`](https://github.com/Vhodnoylogin/body-slots/commit/c1b38fcf6b161f5b8ba2e68e8c98ab47131f4bd1). Their [HIGGS contract](https://github.com/Vhodnoylogin/higgs/blob/development/docs/native-regions.md) and [Body Slots adapter](https://github.com/Vhodnoylogin/body-slots/blob/feature/extract-body-slots/docs/higgs-native-regions.md) describe the implemented boundary.

The copied frame uses current native HMD/hand/palm/configuration inputs, not an older post-VRIK observation. It declares epoch, monotonic sample/time, hand generation and a private source generation that changes for HMD replacement, geometry configuration and scale reinterpretation. Body Slots publishes three transient regions per physical-hand stream, preserves other-hand observations, disables only individual nonpositive-radius regions, rejects replay/regression and resets across saves. Spatial readiness is not a generic action offer or a confirmed inventory receipt. Its strict double-precision boundary is not bit-identical to the old float calculation near rounding boundaries.

Native action admission refreshes through the selected detector and checks current HIGGS authority. Requested-event callbacks are followed by another refresh/fence before the game write. Rejection becomes ordinary drop without retrying another slot. Physical/operation grant creation shares the native-phase gate; callback removal/unregister uses HIGGS's existing ownership and drainage rules. These changes implement a narrow native consumer integration, not the whole generic region action architecture.

Both new full native DLLs build. Body Slots: 1,545,216 bytes, SHA256 `86e91a85dcaa18fce49496905cd8ae850e88abecdf1b44c08f8b481a3e700ae6`, 141 pinned inputs, all nine CTest targets and actual-DLL probe pass. The combined production HIGGS/Body Slots region test checks 186 conditions. HIGGS: 1,661,952 bytes, SHA256 `3a3cddcd35d5f0e2d4b8050aa3886bd598d1418799b4486046d99147e8a48897`, 126 pinned native inputs; production hand/C ABI/DLL tests, 89 detector checks, 14 extracted release-branch checks, actual-source capture fixtures and motion tests pass. A separate C caller also negotiates the new table against the complete HIGGS DLL without `SKSEPlugin_Load`. These checks remain offline; no installation or game start occurred.

Independent review additionally replayed 15 checks against the production cores and the exact extracted native admission lambda. Disabled shoulders preserve mouth use; a Body Slots reset during the requested-event callback prevents admission; detector removal/drainage and transactional suppression preserve a single executor. The engine mutation in that replay is an admission counter, not a running Skyrim call. No remaining confirmed blocker was found within this bounded review.

The following earlier region-service verification remains historical evidence for its prior source version:

The full native Body Slots DLL built successfully: 1,538,048 bytes, SHA256 `d91ce01cb47d1fee207ea335c28d42ace6f62eaba945c6d34cd792b2398a3246`. The build manifest pins 134 source/dependency inputs. All eight CTest targets passed: existing runtime/content/zone/HIGGS/inventory checks, new region and signed-geometry checks, and a separately compiled C discovery caller. The new region suite checks 473 conditions. The C caller independently mirrors the binary table; the public SDK header remains C++ syntax. Actual-DLL table negotiation/disconnected refusal also passed without calling `SKSEPlugin_Load`.

The separate foundation/SDK build passed its older copied-interface, configuration/instance codec, activation and synthetic SKSE serialization checks. It is a different DLL from the full game adapter. Offline checks do not establish game node choice, provider phase coherence, hardware behavior, physical response, rendering quality or actual save/load acceptance in Skyrim VR. No installation or game start occurred.

## Remaining integration work

The new service evaluates supplied points against finite primitives. The optional native HIGGS mouth/stash integration above is implemented in the cooperation sources, with game qualification pending. General capability-bound source registration, PPB/PLANCK source adapters, anatomical surfaces, rays/sweeps, composite shapes and generic region action offers remain unimplemented. Existing player-slot activation continues through its existing HIGGS route. Pickpocketing and the other consumers have not been ported; generic regions have no separate content, presentation or persistence subsystem.

Next: qualify the native HIGGS/Body Slots integration in the game through the testing workflow, then extend generic region offers to the existing HIGGS recognizer/grant route for another concrete consumer. The native spatial callback does not itself provide that general action ABI. Keep PPB anatomy and PLANCK/HIGGS physical bodies under their current owners. Original-shield/effective-equipment transitions, full consumed-draw release continuation and other domain executors remain separate unfinished work.

## Other providers and previous delivery

HIGGS's earlier `befa09a9` nonbutton hand-use priority rounds are retained alongside the new optional native-region integration; PLANCK source `28eb8a7f` retains the prior cooperation services and is unchanged by this integration. The prior Body Slots `ddb14db` content-observation service and unresolved-action reservation repairs remain. Body Pouches itself has not been ported. Body Slots is public development source; reconstructed VRIK remains private. No runtime release, new license grant, author outreach or author agreement is implied.

The [CommonLib assessment and inventory](../trinity-research/commonlib-assessment.md) remain stage-two preparation. Shared signed geometry and typed transforms are audit candidates, while region/session/dwell policy remains in Body Slots. No CommonLib implementation or new engine address was added.

The concept folder now contains only this ledger, its README and the aligned language versions. [Research and source evidence](../trinity-research/README.md) and [earlier implementation history](../trinity-research/implementation-history.md) are separate; superseded checkpoint and presentation files remain in Git history.
