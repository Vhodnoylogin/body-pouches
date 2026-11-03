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

The full native Body Slots DLL built successfully: 1,538,048 bytes, SHA256 `d91ce01cb47d1fee207ea335c28d42ace6f62eaba945c6d34cd792b2398a3246`. The build manifest pins 134 source/dependency inputs. All eight CTest targets passed: existing runtime/content/zone/HIGGS/inventory checks, new region and signed-geometry checks, and a separately compiled C discovery caller. The new region suite checks 473 conditions. The C caller independently mirrors the binary table; the public SDK header remains C++ syntax. Actual-DLL table negotiation/disconnected refusal also passed without calling `SKSEPlugin_Load`.

The separate foundation/SDK build passed its older copied-interface, configuration/instance codec, activation and synthetic SKSE serialization checks. It is a different DLL from the full game adapter. Offline checks do not establish game node choice, provider phase coherence, hardware behavior, physical response, rendering quality or actual save/load acceptance in Skyrim VR. No installation or game start occurred.

## Remaining integration work

The new service currently evaluates externally supplied points against finite primitives. Automatic source/provider adapters, capability-bound source registration, anatomical surfaces, rays/sweeps, composite shapes and generic region-to-action routing are not implemented. Existing player-slot activation continues through its existing HIGGS route. Generic regions do not yet replace HIGGS mouth/stash detection or Pickpocketing's detector, and they have no separate content, presentation or persistence subsystem. Those consumers must not be described as ported.

Next: connect real provider snapshots and region action offers to the existing HIGGS recognizer/grant route, qualify source identities/phases and enforce a single detector/execution path for each migrated standard action. Keep PPB anatomy and PLANCK/HIGGS physical bodies under their current owners. Then exercise a concrete consumer using the same production contracts. Original-shield/effective-equipment transitions, full consumed-draw release continuation and other domain executors remain separate unfinished work.

## Other providers and previous delivery

HIGGS source `befa09a9` contains nonbutton hand-use priority rounds; PLANCK source `28eb8a7f` contains the prior cooperation services. This continuation changes neither provider's implementation, only their link to the relocated CommonLib assessment. The prior Body Slots `ddb14db` content-observation service and unresolved-action reservation repairs remain. Body Pouches itself has not been ported. Body Slots is public development source; reconstructed VRIK remains private. No runtime release, new license grant, author outreach or author agreement is implied.

The [CommonLib assessment and inventory](../trinity-research/commonlib-assessment.md) remain stage-two preparation. Shared signed geometry and typed transforms are audit candidates, while region/session/dwell policy remains in Body Slots. No CommonLib implementation or new engine address was added.

The concept folder now contains only this ledger, its README and the aligned language versions. [Research and source evidence](../trinity-research/README.md) and [earlier implementation history](../trinity-research/implementation-history.md) are separate; superseded checkpoint and presentation files remain in Git history.
