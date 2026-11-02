# PPB provider integration audit

Research checkpoint: 2 November 2026. A read-only fetch of the author's [public repository](https://github.com/Telord72612/Precision-Physic-Bodies) left `main` at `e26b4ebc8061e8736b344e4f3e9216dea9254747`. Source is MIT; native dependencies and third-party assets remain separate. No PPB source change, closed-code decompilation, installation, author outreach or game test was performed.

This audit supports the [maintained concept](concept-en.md), the [consumer requirements](consumer-contract-matrix.md) and the [proposals for telord](ppb-author-requests.md). It is not an implementation-status claim. Exact source hashes and published native bindings are recorded in [the provenance dataset](ppb-source-and-bindings.json).

## Contents

- [Actual geometry and observations](#actual-geometry-and-observations)
- [Required HIGGS contract](#required-higgs-contract)
- [Required PLANCK contract](#required-planck-contract)
- [Factory evidence and limits](#factory-evidence-and-limits)
- [Consumer data and lifetime](#consumer-data-and-lifetime)

## Actual geometry and observations

- [CapFix.cpp](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/CapFix.cpp#L45) maps 12 NPC body slots with left/right twins and capsule children inside existing `hkpListShape` objects. The source's plausibility bound is 44 children, rather than one direct capsule per body. Capsule edits also update the list's union AABB; child state and cached bounds matter. Approximately 148 capsules is the API header's descriptive count, not a universal per-actor constant.
- [HandBox.cpp](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/HandBox.cpp#L358) supplies four tracked player boxes per hand: index proximal, index distal/tip, a grouped three-finger proximal slab and distal tip plate. It does not add a thumb collider; HIGGS supplies the palm. Its pre-physics path samples bone pose once per frame, adjusts the slab, creates/retires parts, updates collision policy, applies player-space warp and keyframes the eight boxes. This requires articulated-part ownership and attribution, not only whole-hand extents.
- [PpbTouchAPI.h](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/PpbTouchAPI.h#L409) supplies 160-byte touch records with actor form ID, anatomical slot/child/side, source hand/kind, distance/duration, region/subregion and weapon classification. It distinguishes engine contact from geometric fallback. Capsule geometry uses world coordinates in game units; count and label queries are separate. Digest grouping has different semantics from raw per-capsule contacts.
- [PpbApi.cpp](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/PpbApi.cpp#L2469) provides copied contact snapshots but resolves actors live for capsule geometry. Native `AddTouchCallback` has no matching public removal/drain. Records do not carry world/actor/body/shape generations or a shared geometry revision. Its API comments constrain the main-thread roster/frame pipeline. Player-self target coverage cannot be inferred from the NPC geometry service.

## Required HIGGS contract

HIGGS should own the palm and registered articulated hand parts. A bounded part-set proposal needs a physical hand, part ID, supported shape, bone/local frame, geometry revision and tracked sample. Application belongs to a declared HIGGS physics phase, with actual applied geometry/pose and canonical physical-hand attribution available to contact, haptic and PLANCK consumers. Shape support and topology support are separate capabilities.

One actuator must apply warp, keyframes, collision lifecycle and hand targets. PPB supplies fitting/pose data. Holding, secondary-grip and equipped-weapon states need to compose collision policy, so PPB does not have to clear native `userData` to suppress false hits. A bounded whole-hand geometry call does not implement the eight-part service.

## Required PLANCK contract

PLANCK needs bounded proposals for existing NPC capsule/list children, stable body/shape-child identities and expected geometry revisions. Safe adoption should clone the existing list wrapper and every mutated child, verify that modified shapes are private, recompute aggregate bounds and preserve disabled masks, filters and materials. Only actual owner-applied geometry should be reported as applied. Direct capsule cloning alone misses most PPB geometry. Non-ragdoll followers require a separate owned capability; adding bodies to a ragdoll is not a substitute.

[Hooks.cpp](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/Hooks.cpp#L309) chains the outer drive-to-pose call; its inner controller path modifies pose and palette inputs. It temporarily changes PLANCK's global pivot-loosening setting. [PPBHook.cpp](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/PPBHook.cpp#L2031) brackets per-bone weights around the controller drive.

Actor-generation-scoped pivot/drive policies and copied pre-drive/body/pose observations can remove those particular hook/global-setting dependencies. PLANCK remains the writer. The bounded gain/weight/pivot extension now being developed does not provide PPB's complete pose-conforming pipeline. PPB's push, walking, scene and equipment gameplay need not be absorbed into PLANCK.

## Factory evidence and limits

The provenance dataset records the exact published box/body/world/keyframe signatures and RVAs. PLANCK already declares a list-wrapper constructor and configuration destructor, but this audit found no exercised list-creation call site in the inspected paths. A declaration is evidence for a binding, not qualification of construction, destruction or ownership.

There is **no bound native capsule constructor** in the inspected PPB source. `NpcFingerTest.cpp` constructs capsules by manually stamping vtables and using the never-free `memSizeAndFlags` convention. The author's hand-collider notes explicitly say that a capsule constructor was not bound. This recipe is not a qualified reusable allocator/destructor route and should not be promoted to CommonLib as one.

Prefer engine-cloned existing shape wrappers for same-topology adoption. New finger boxes can use the published existing box/body constructors, subject to separate lifetime qualification. An arbitrary new capsule/list factory needs verified construction and ownership. The author's pitfall ledger records shared-shape contamination and historical allocation/`setShape` failures; those observations guide investigation but do not qualify our new paths.

## Consumer data and lifetime

Consumers need copied anatomical geometry/contact data tied to world, actor/model, body, shape-child and source-hand lifetimes, sample phase/time and applied geometry revision. Preserve contact-versus-proximity, raw-versus-digest semantics and side/source attribution. Optional hit correlation can connect observations to PLANCK outcomes, but touch itself grants no action or damage permission.

Subscriptions need removal/drain and explicit reset/removal endings. HIGGS grants hand use, PLANCK commits its hits, PPB interprets anatomy, and Body Slots owns player slot mechanics. The current PPB interface is useful evidence for these contracts; it is not automatically the proposed service or proof of player-body support.
