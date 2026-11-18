# PPB: proposals for discussion with telord

The November 11 source proposal now includes copied anatomy batches and owned removable completed-scan subscriptions. It is based on public PPB 2.2.0, with production translation-unit compilation and offline lifecycle/queue checks. Body/model generation and the safe publication point for the private worlds in 2.9 remain requests for author review. The broader November 2 questions below retain their original research scope. [PR #3](https://github.com/Telord72612/Precision-Physic-Bodies/pull/3) · [Issue #4](https://github.com/Telord72612/Precision-Physic-Bodies/issues/4)

Research checkpoint: 2 November 2026, public PPB source at [`e26b4eb`](https://github.com/Telord72612/Precision-Physic-Bodies/tree/e26b4ebc8061e8736b344e4f3e9216dea9254747). These are discussion points, not a message sent to the author, an agreed migration or a claim that the supporting HIGGS/PLANCK services are ready in game. The [provider audit](ppb-provider-audit.md) records the evidence and limits. The [maintained concept](../trinity-synergy/concept-en.md) defines the shared responsibilities.

The intended benefit is to let other mods ask **which anatomical surface was touched, by what, and with what evidence**, without recreating PPB's fitting or inspecting Havok objects. PPB would retain anatomical fitting, region classification, touch interpretation and its gameplay features. HIGGS and PLANCK would provide the physical publication and actuation services that PPB could choose to use.

Russian: [предложения на русском](ppb-author-requests-ru.md).

## Division of responsibility

**We ask PPB to own anatomical meaning and fitting data. We must provide physical execution through HIGGS and PLANCK.** The proposal is a division of work, not a request for telord to implement those providers or maintain another copy of their mechanics.

| Owner | Responsibility we expect it to retain/provide | What PPB obtains from it |
|---|---|---|
| **PPB** | Anatomical regions and identifiers, fitting to meshes/skeletons, desired surface/hand-part geometry, interpretation of contact/proximity/dwell, and its own gameplay rules. Publish coherent anatomical observations and submit fitting proposals | PPB is the anatomy service used by other mods. It publishes applied surfaces using physical-owner results, with an explicit distinction from a desired fit |
| **HIGGS** | Controller input, the fixed hand-state/use model, permissions, physical palm/finger bodies, their movement, collision policies, haptics and lifetime. Apply hand-part proposals and report actual results | Register/update fitted hand parts; obtain applied part geometry/pose, canonical physical-hand/part identity and contact evidence; request scoped hand effects and hand use |
| **PLANCK** | NPC body/ragdoll lifetime, physical animation, safe NPC shape publication, scoped actor drive/reaction policies, and its physical-hit decisions/results. Apply NPC fitting proposals | Read current physical bodies/shape children; submit revision-checked capsule edits; request actor-scoped physics policies; receive applied geometry, invalidation and hit evidence |

VRIK remains the provider of the player's body and virtual-hand pose. Body Slots remains the owner of interaction zones, persistence and slot behavior; PPB can supply an optional anatomical anchor where coverage exists. A touch never itself grants hand control or commits damage.

The intended data flows are:

1. PPB computes fitted finger-part data → HIGGS creates/updates the physical parts → HIGGS returns applied state and contacts → PPB adds anatomical interpretation.
2. PPB computes fitted NPC capsules → PLANCK validates and applies them under its graph/world ownership → PLANCK returns applied geometry and revisions → PPB publishes the corresponding anatomical surfaces.
3. A consumer reads a PPB touch → asks HIGGS for a conflicting hand action or PLANCK for an NPC physics operation where needed → the responsible executor reports the actual outcome. PPB's own gameplay features can use exactly this path.

Adoption depends on the physical services being available and qualified. Existing PPB actuation can remain the backend for released dependencies until a supported shared backend is ready; each selected backend must have one physical writer.

## Contents

- [Division of responsibility](#division-of-responsibility)
- [First: a small, independently useful public API extension](#first-a-small-independently-useful-public-api-extension)
- [Then: integration when the physical owners can support it](#then-integration-when-the-physical-owners-can-support-it)
- [Optional extensions to discuss separately](#optional-extensions-to-discuss-separately)
- [What we should prepare on our side](#what-we-should-prepare-on-our-side)

## First: a small, independently useful public API extension

1. **Removable, owned touch subscriptions.** Could a new API revision return a subscription token and provide nonblocking removal with a clear drainage contract? After drainage, PPB must no longer call the callback or access its context. Please define callback phase/thread, permitted reentrancy and teardown/reset behavior. The current `AddTouchCallback` has no matching public removal. This would make existing touch observations safer for consumers even without the wider synergy.

2. **One coherent copied geometry view.** Could consumers copy an actor's supported anatomical regions and all their child shapes from one published revision, with a required-capacity result? Include world/actor-model/body/shape-child identity or generations, geometry revision, sampling phase/time, coordinate space and units. Avoid requiring a consumer to join a live `ChildCount` with separately resolved `ReadCapsule` calls during actor/model replacement. Identifiers may be opaque provider tokens; consumers do not need raw engine pointers. The requested snapshot should describe geometry actually applied to physics, with any proposed but unapplied fit identified separately.

3. **Correlated contact and lifecycle evidence.** Could copied touch records identify the geometry revision, source hand/part generation and target region/child, and expose explicit endings when an actor, model, world or source is replaced? Please preserve the distinction between an engine contact and a geometric proximity fallback, and between raw contacts and dwell/grouped digest events. Where PLANCK can supply a shared hit identifier, optional correlation would help a consumer join anatomy to a physical hit without treating every touch as damage.

4. **Explicit capabilities and coverage.** Could API negotiation identify supported shapes, source kinds and target classes, including NPCs, inactive corpses and the player's own body separately? Document whether a roster omission means unsupported, unavailable, filtered out or simply no current touch. We have not established player-self target coverage from the current NPC API, and would not ask Body Slots to rely on it without an explicit supported path.

These can be additive tables or a new negotiated revision. Existing clients should keep their current ABI and touch/digest semantics. The implementation approach is for discussion; these requests describe the contract rather than prescribe class structure.

## Then: integration when the physical owners can support it

5. **Let PPB describe hand parts while HIGGS applies them.** Would you be open to an optional backend in which PPB submits its fitted, articulated hand-part data to HIGGS? The current geometry includes four independently tracked finger boxes per hand in addition to HIGGS's palm. HIGGS would need to support that actual topology, canonical part attribution, sample timing, collision policy, world changes and applied results. PPB should not have to replace those eight boxes with one whole-hand box or lose its fitting. This is a service we must provide in HIGGS before proposing removal of PPB's current actuator.

6. **Let PPB fit NPC surfaces while PLANCK publishes the changes.** Would you consider submitting bounded geometry proposals for the existing body/list children to PLANCK, with an expected revision and an applied/rejected result? PLANCK would own safe application under its graph/world phase, private shape copies, aggregate bounds, disabled-child state and cleanup. PPB would retain the anatomical map and fitting calculations. Most current PPB capsules are children of compound list shapes: a direct-capsule-only endpoint is insufficient. We must complete and qualify that service on the PLANCK side first.

7. **Replace overlapping hooks and global settings with scoped requests.** Which precise pose, pivot and controller operations does PPB need from PLANCK, and which grab/finger/collision policies does it need from HIGGS? We would like to replace temporary global setting changes and competing drive hooks with actor/hand-scoped requests and copied observations. The current bounded PLANCK gain/weight/pivot API is only part of that requirement; it does not reproduce PPB's full pose-conforming pipeline. PPB's push, walking, scene and equipment rules can remain its own consumers of those services.

Adoption should be capability-negotiated and incremental. A backend must have exactly one physical writer for each registered actor/body/hand part, and must not silently run both the legacy and shared actuator. There is no request to drop support for current released HIGGS or PLANCK before an adequate replacement exists.

## Optional extensions to discuss separately

- **Player-body anatomical anchors.** Would a read-only, generation-aware player-body surface view be useful and feasible? It could let Body Slots anchor a zone to an anatomical region. Body Slots would still own the zone, persistence, customization and interaction events. This is a new capability question, not a claim that current NPC fitting already supplies it.
- **Finger and non-capsule surface topology.** If PPB intends to expose more body parts or shapes, a shape-kind capability and stable part/child identifiers would let consumers opt in without hardcoding the current layout or capsule count.
- **Minimal examples and fixtures.** A small copied-data consumer and documented reset/unsubscribe example would make the ABI easier to adopt. Fixtures can distinguish touch, proximity and damage and demonstrate a model replacement without stale anatomical references.

## What we should prepare on our side

We can offer AI-assisted source analysis, draft additive API changes, example consumers and regression fixtures for the author's review. The next useful concrete exchange is a short contract review: which requests match PPB's intended scope, which are already available through a path we missed, and what should change before implementation.

Our responsibilities are to supply the missing HIGGS articulated-part and scoped-policy services, PLANCK compound-geometry and needed drive contracts, and actual native/game qualification before claiming a PPB migration. We should bring a small working consumer and a reviewable diff, rather than ask telord to implement our providers. No CommonLib migration, PPB asset reuse or private VRIK source is required for this API discussion.
