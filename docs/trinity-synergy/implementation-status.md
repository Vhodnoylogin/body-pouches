# Synergy implementation status

Updated: 11 October 2026. This is the implementation ledger next to the [English concept](concept-en.md) and [Russian concept](concept-ru.md). The concept describes the destination; this file records delivered code, checks and remaining gaps. Native fixtures and a successful link do not establish Skyrim runtime compatibility.

## Repositories

| Component | Repository | Working implementation |
|---|---|---|
| HIGGS | [Vhodnoylogin/higgs](https://github.com/Vhodnoylogin/higgs), genuine fork of adamhynek/higgs | `development` at `f3beb76` (tested code `5248a7c`): earlier repairs, consolidated history and owned temporary restrictions |
| PLANCK | [Vhodnoylogin/activeragdoll](https://github.com/Vhodnoylogin/activeragdoll), genuine fork of adamhynek/activeragdoll | `development` at `986a3e7` (tested code `156b3cf`): consolidated history, transient actor-policy reset and nested hit-context repairs |
| VRIK adapter | [vrik-reconstruction](https://github.com/Vhodnoylogin/vrik-reconstruction), private | `feature/extract-body-slots` at `96110cf`: separate authored and configured geometry observations |
| Body Slots | [body-slots](https://github.com/Vhodnoylogin/body-slots), private | `feature/extract-body-slots` at `99417b4`: registry, packages and saved placement customizations |
| Morphbench | [morphbench](https://github.com/Vhodnoylogin/morphbench) | `main` at `5692a04`: separate Slots authoring mode, CLI replay and Body Slots exports |
| CommonLib | [CommonLibSSE-NG](https://github.com/Vhodnoylogin/CommonLibSSE-NG), genuine fork of alandtse/CommonLibSSE-NG | Analysis only; inspected `ng` at `a898f469851c464d05137bb74b069dd234897643` |

HIGGS and PLANCK have exactly two public branches: `main` contains the original current upstream source; `development` contains our work. The former development repositories were removed after verified full-history preservation. PLANCK's unfinished float-precision experiment is retained in development history without enabling its experimental code. Private repository links require access; reconstructed VRIK implementation remains private. This ledger is not an agreement with the original framework authors.

## Implemented and checked

| Area | Implemented | Evidence and practical limit |
|---|---|---|
| Body Slots definitions and packages | Strict JSON validator and deterministic binary compiler; stable provider/key, named anchor, rigid local transform, independent access sphere, physical hand and content masks | 17 Python tests and 716 native checks, including concurrent registry mutation; bounded package parsing and separate consumer DLL. Definitions do not represent equipment instances |
| Saved placement configuration | Separate additive configuration ABI; position, rotation, access sphere and enabled overrides; dedicated customizer authority, revision comparison and coherent effective copies | 417 configuration checks, 75 actual DLL/synthetic SKSE callback checks; independent 589 core and 116 callback checks. Tests cover malformed records, all-or-none loading, dormant missing providers and unchanged authored defaults |
| Serialization boundary | Explicit portable records keyed by provider/key; defaults survive invalid input; load/new game clear prior overrides, absent providers retain dormant records | No engine pointers, process leases or runtime slot IDs are saved. Per-definition overrides are implemented; equipment identity and per-instance state are not |
| VRIK spatial adapter | Named anchor binding, copied placement/access sphere, physical left/right probes, generation and producer-computed age; additive configured geometry view | Final core: 4070 authored checks and 120 configured checks against the new Body Slots DLL; 16 independent geometry/enable-edge checks. Both views share one anchor sample per slot; incoherent configuration is bounded to three attempts and remains unavailable |
| Original VRIK compatibility boundary | Official interface001 and native14 layouts remain unchanged; authored observation ABI retains authored semantics | Full 55-unit SDK build links and both geometry exports are present. The complete VRIK DLL has not been loaded for these checks; original14 gameplay regressions remain unqualified |
| HIGGS temporary restrictions | Separate copied C ABI; owned, expiring physical-hand restrictions, weapon-collision suppression and temporary force-enabled policy; renewal, release and world invalidation; legacy bits compose independently | Nine native/C/C++ suites and an actual dynamic C consumer pass. Independent review found and prompted a pending-pull cancellation repair. Scoped hand suppression gates new admissions; established holds are preserved |
| PLANCK bounded repairs | Reset clears movement/look-at restrictions under their locks; readers no longer inspect the maps outside locks. Synchronous intercepted hit dispatch restores the previous context across nested PLANCK/ordinary events | Five actual-source regression suites and 41 independent dispatch checks pass against engine stubs. Official001 is unchanged. The native plugin remains unbuilt; direct event-send paths and concurrent legacy dispatch are outside this repair |
| Morphbench authoring | Existing mesh/skeleton viewer gains a Slots mode; named anchors, local transform, sphere, policy fields, enabled state and temporary override preview; JSON/package exports and equivalent CLI recipe | 440 checks across 27 suites pass with no skips, including 12 targeted Slots checks. Actual browser checks cover editing, authored/effective separation, exports and missing anchors. Actual page-mirror parity and 46 independent native export checks pass; all 928 downloaded package bytes match the Body Slots compiler. Reference-pose preview does not establish in-game reach or rendering |

The configured module checks pin BodySlots.dll SHA-256 `42041afc64fdab60f4f0ad605c84deca90995ee1fb8f2e5d0d3d309cd4880228` and built VRIK.dll SHA-256 `14e5ded07b1348f682e919d2b1a6846e42fc6f492a020246e9ae25ddec91893f`. Reproducible fixture sources are committed in their repositories; private review reports and compiler outputs remain in the project journal and external evidence directories. The HIGGS standalone service is built and exercised; the full HIGGS/PLANCK plugins remain unbuilt. Original SKSE VR source and the toolchain exist externally, but native project paths need configuration and the required compatible Havok include tree is absent from the audited locations. The fork-specific [HIGGS](https://github.com/Vhodnoylogin/higgs/blob/development/docs/native-build-prerequisites.md) and [PLANCK](https://github.com/Vhodnoylogin/activeragdoll/blob/development/docs/native-build-prerequisites.md) prerequisites distinguish these issues; further linker needs are not established. No mod was installed and no game was launched during this checkpoint.

An additional Morphbench offline check loaded an explicit installed HIMBO body with 14,597 vertices and an XP32 skeleton with 543 finite NODE frames. Both slot markers use the actual pelvis NODE transform. Browser editing preserved authored placement while showing a separate temporary override; downloaded authored JSON and all 928 package bytes match the current Body Slots compiler. Reference assets were unchanged. This checks authoring against real reference geometry; it does not determine MO2 winners or qualify animated poses, calibration or Skyrim behavior.

HIGGS's force-enabled flag only bypasses its temporary post-hit weapon-collision disable window. It does not override sheathing or an unavailable physical body, and any active suppression wins. It is not a general contact query or a new collision actuator.

## Still required

- Equipment instances, per-instance activation, content/visual descriptors and preview lifecycle; the current saved override applies to a provider/key definition.
- A translating facade for original native14 configuration, with saved-array and Papyrus compatibility checks.
- VRIK interaction and presentation for new slots, plus actual tracking validity and model/handedness qualification. Geometry alone cannot draw, deposit or display an item.
- HIGGS input arbitration, exclusive physical resource ownership, coherent possession and correlated transfer outcomes. Temporary restrictions can coexist; they do not arbitrate an exclusive action.
- PLANCK's `DropObject` currently uses legacy `DisableHand` to end an established hold. New admission restrictions preserve that hold; this consumer needs explicit release with a correlated result before changing that call.
- Transfer cancellation across both hands: cancelling a receiver's pending pull cannot undo a donor release queued before a new restriction. A correlated transaction must cover that earlier side effect.
- PLANCK copied actor endpoints, generation-aware attachment requests and bounded hit-policy participation.
- Consumer migrations and Skyrim testing, including real anchors, loading, inventory-to-grip commitment and original14 regression coverage.

## Consumer ABI checks

| Consumer from the concept | Current usable foundation | Missing contract before migration |
|---|---|---|
| Body Pouches | Non-weapon content masks, independent spheres, physical proximity, saved placement customization and configured frames | Assignment/instance persistence, richer content policy, preview lifecycle, reservation and confirmed inventory-to-grip transfer |
| Shields and 2H Weapons Unlocked | Body anchors, authored/configured placement and physical hand observations | Explicit destination hand, original/proxy identities, committed equip/transfer and independent presentation |
| Swap Drop and Hold Redux | Spatial observations as action candidates; temporary HIGGS restrictions | Input ownership, coherent grip relation, eligibility and confirmed swap/drop/transfer |
| Physical Collision VR | Spatial frames with age/generation; owned weapon-collision suppression and temporary force-enabled policy | Coherent physical/collision transforms, contacts and single physical actuator participation |
| True Wield VR | Physical hand distinction and temporary collision policy | Grip geometry, mass/inertia, drive budgets, two-hand state and hit policy |
| Immersive Weapon Penetration VR | No actor endpoint service yet | Hit data, actor/body/bone generations, scoped attachment and extraction outcomes |
| Immersive Harvesting VR | Shared temporary HIGGS hand/collision restrictions | Exclusive grip arbitration, atomic action reservations, contact/motion and confirmed harvested-item delivery |
| VR Climbing — Aelove Ver | Centralized owned HIGGS restrictions with expiry, renewal and world invalidation | Occupied/support state, climb-start arbitration, pose coordination and release outcomes |

These are checks against the concept's consumer requirements. None of the listed consumers has yet been migrated to these new interfaces or qualified in Skyrim. In particular, the centralized restriction foundation addresses part of climbing/harvesting's duplicated coordination; it does not complete their input and physical-action contract.

## Current internal audit follow-up

Current PLANCK already restores pending ragdoll constraints on removal; the earlier premise that restoration was missing is obsolete. Remaining source-backed cases need a larger lifetime contract: tracking can remain active after the animation graph/world disappears, delayed furniture cleanup does not revalidate active status or world generation, and legacy per-actor overrides retain raw actor keys. Counterexamples establish the conditional control-flow gaps, not their frequency or harm in Skyrim. New actor endpoints and queued actions must validate generation and ownership instead of dereferencing saved driver pointers. Constraint-snapshot restoration also performs repeated membership searches; its performance importance needs measurement before optimization.

Additional HIGGS source review found that `FillControllerVelocities` preserves the previous velocity history when a required HMD/controller pose is missing, disconnected or invalid. The tracking-data record carries no validity or sample-age field, and the inspected release/pull readers use its cached values without such a gate. This establishes conditional stale-data retention; reachability and gameplay harm during tracking loss remain unqualified. A future HIGGS pose/motion service must report per-hand tracking validity, source sample age and a valid tracking-to-world conversion. VRIK's current node-presence/finite-position probe is also only a proxy, so it cannot substitute for that tracking contract. No tracking-loss repair or pose API is implemented by this checkpoint.

## CommonLib follow-up

Current reuse and proposed extensions are tracked in [CommonLib assessment](commonlib-assessment.md). No library implementation is part of this stage. Registry ownership, slot policy, HIGGS possession, VRIK body solving and PLANCK hit decisions remain their providers' responsibilities.
