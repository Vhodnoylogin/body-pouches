# Cooperation between HIGGS VRIK PLANCK PPB and Body Slots

Russian: [Русская версия](concept-ru.md).

This proposal describes a shared architecture for HIGGS, VRIK, PLANCK, PPB, Body Slots and their consumers. HIGGS owns all hand states and ways of using hands, Body Slots owns all logical interaction-region and slot mechanics, VRIK owns the body and virtual hand poses, PLANCK owns its NPC ragdoll bodies, their contacts and physical hits, and PPB supplies anatomical surfaces, fitting and query evidence. Each exposes the observations and requests the others need. This is the target architecture, not a claim of completed implementation or agreement with the authors; the [implementation ledger](implementation-status.md) records delivered code and verification limits. All interface blocks below are illustrative pseudo-ABI: supporting records are abbreviated, and names, layouts, numeric values, calling convention and transport require separate agreement.

The intended benefit is concrete: a mod author can ask for a named interaction region, a physical handover or an NPC attachment without recreating another framework's internals. An interaction region describes a spatial condition. A slot is a registered, named region with offered actions and activation rules; storage, presentation and persistence are optional capabilities. A mouth, an NPC pocket and a wearable pouch can share this contract while keeping different gameplay rules. A bare anatomical or physical surface is not automatically a slot. A single interaction has identifiable participants and a confirmed result.

## Contents

- [Responsibilities](#responsibilities)
- [HIGGS input possession and physical requests](#higgs-input-possession-and-physical-requests)
  - [Input and action selection](#input-and-action-selection)
  - [Hand state and coherent observations](#hand-state-and-coherent-observations)
  - [Shared hand-use operations](#shared-hand-use-operations)
  - [Grabbing release and transfer](#grabbing-release-and-transfer)
  - [Physical constraints](#physical-constraints)
  - [Domain executors](#domain-executors)
- [VRIK body and hand pose](#vrik-body-and-hand-pose)
  - [Body and hand pose](#body-and-hand-pose)
- [PLANCK NPC physics and hits](#planck-npc-physics-and-hits)
  - [Actor bodies and attachment points](#actor-bodies-and-attachment-points)
  - [Physical hits](#physical-hits)
- [PPB anatomical surfaces and touch](#ppb-anatomical-surfaces-and-touch)
  - [Surface geometry and fitting](#surface-geometry-and-fitting)
  - [Touch evidence and action authority](#touch-evidence-and-action-authority)
  - [Existing API and the proposed extension](#existing-api-and-the-proposed-extension)
- [Body Slots all slot mechanics](#body-slots-all-slot-mechanics)
  - [Visual editor before launching the game](#visual-editor-before-launching-the-game)
  - [Slot definitions instances and configuration](#slot-definitions-instances-and-configuration)
  - [Slots supplied by equipment](#slots-supplied-by-equipment)
  - [Zones](#zones)
  - [Runtime slot binding](#runtime-slot-binding)
  - [Slot visuals](#slot-visuals)
  - [Content providers](#content-providers)
  - [Registry pose adapters and executors](#registry-pose-adapters-and-executors)
- [Input and action sequence](#input-and-action-sequence)
- [Shared physical operation](#shared-physical-operation)
- [Mods used to assess ABI expressiveness](#mods-used-to-assess-abi-expressiveness)
- [Body Pouches with the shared services](#body-pouches-with-the-shared-services)
- [Shields and 2H Weapons Unlocked with the shared services](#shields-and-2h-weapons-unlocked-with-the-shared-services)
- [Climbing and harvesting](#climbing-and-harvesting)
- [Compatibility and ABI lifecycle](#compatibility-and-abi-lifecycle)
- [CommonLib as the second level of synergy](#commonlib-as-the-second-level-of-synergy)
- [User priority and mod order](#user-priority-and-mod-order)

## Responsibilities

| Provider | Owns | Provides to other mods |
|---|---|---|
| HIGGS | The sole model of all hand states, every hand-use mechanism, grip/trigger input, reservations, haptic scheduling and physical possession | Closed occupancy/availability states, action grants, shared object/world-support grips, weapon support, release, transfer and consumption operations with confirmed outcomes |
| VRIK | Player body, calibration and virtual hand poses | Coherent virtual-hand/body-anchor observations with coordinates, time, validity and generations |
| PLANCK | NPC physical animation and ragdoll lifecycle; its actual physics contacts, physical-hit interpretation and commitment | Actor/body/bone metadata, validated attachment endpoints, applied geometry revisions, contact and candidate/committed hit data, scoped policies and lifecycle events |
| PPB | Anatomical surface definitions, body-shape fitting and interpretation of anatomical query/touch evidence | Supported-body coverage, effective capsule geometry, anatomical regions and qualified observations; geometry proposals applied by the physical owner |
| Body Slots (working name) | All logical region/slot mechanics: definitions/instances, anchors, geometry evaluation, spatial timing, offers, activation, optional content/presentation bindings and saved customization | Region/slot creation and configuration, definition packages, equipment slot sets, spatial snapshots/events, action offers, addressed behavior requests and observable results |
| External mod | Its gameplay feature and domain/content rules | Potion selection, crime, food behavior, harvesting, unconventional equipment/proxies, gestures, weight response, penetration/extraction or parry rules expressed through shared contracts |
| CommonLib | Engine addresses and types, reusable technical operations and verified wrappers | A common implementation of engine access, geometry, scene and physics operations for the cooperating frameworks and external mods |

HIGGS supplies the common input stream, hand model and control of every hand-use mechanism. Providers may recognize gameplay gestures and propose actions, but use HIGGS's closed set of shared operations. Body Slots recognizes logical spatial conditions and activates selected slot actions; VRIK supplies pose without selecting them. Body Slots neither creates physical colliders nor writes physics bodies, constraints or hand targets. Referencing PPB anatomy or HIGGS/PLANCK surfaces does not duplicate their physical ownership. A recognizer does not gain permission to create a second physical actuator or bypass HIGGS arbitration. Forwarding an action result does not transfer pouch, food, crime, harvest or damage rules to HIGGS; the result identifies the actual executor.

**Minimum contract for one action.** A consumer observing a pouch needs a current target pose and a hand probe in compatible coordinates and time. Body Slots answers whether the hand is in that region. For an action on grip, HIGGS must additionally identify the input, arbitrate all participating offers including the native action, and authorize at most one handler. A delayed handler rechecks the target and hand before changing anything. The inventory owner reports the actual result. A read-only observer can drop an observation when its buffer is full. Registering that observer must never veto a native hit, alter tracking, or start unrelated physics work. A mod requesting a drive or a change of collider geometry negotiates that capability separately. When every optional client is absent, the providers must preserve their original gameplay paths.

A consumer can negotiate an immediate grip edge independently of the extended controller-axis stream. A capability is justified by an interaction that fails without it. For example, opening a pocket on an NPC needs a stable, current anchor even with zero skin contact. Using the player's grip needs HIGGS arbitration even when no NPC is present. Subscribing to the outcome of a hit requires neither input ownership nor the right to modify damage. A hand-use mode outside the shared contract can be reported as unknown only if the provider has evidence of uncertainty. An invisible third-party input hook cannot be detected reliably through another mod's ABI.

## HIGGS input possession and physical requests

HIGGS is the sole authority for the shared hand-state model and coordination of every hand-use mechanism. It observes game equipment through the engine and virtual hand pose through VRIK, and manages physical hands, input, grips, reservations and haptics. Body Slots and other consumers obtain state and grants from HIGGS without a parallel classifier. Locomotion, harvest policy, slot contents and damage remain with their gameplay owners; HIGGS routes authorized operations to the assigned executor and reports the outcome.

### Input and action selection

Recognizers use one input stream and submit candidates before default actions execute. InputBinding declares context, action, recognition deadline and deferred-default behavior. ActionCandidate names the input sequence, required resources and registered executor. HIGGS selects an eligible action using the user's priority and reports the decision. Body Slots supplies slot spatial context; other recognizers use the same broker.

Native HIGGS actions participate in the same choice when they are actually eligible. A candidate has a persistent provider key and action key, with a stable target key where multiple targets compete. Registration order and temporary handles do not break ties. With the order harvest → native grab → pocket, an unavailable harvest leaves native grab ahead of the pocket. Existing possession is checked before this choice. A native release continues its current hold and is never rerun as a new grab.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyInput(ClientToken, PhysicalHand, InputObservation* outInput);

Status RegisterRecognizer(ClientToken, const InputBinding*, RecognizerCallback, void* userContext, Lease* outRegistration);

Status SubmitActionCandidate(ClientToken, const ActionCandidate*, RequestId* outRequest);

Status ProposeHandUse(ClientToken, const HandUseProposal*, RequestId* outRequest);

// Events: InputChanged.

}
```

The broker exposes the effective priority and the reason for its decision. A consumer can inspect these without acquiring the right to consume the same input again. Haptic requests enter one coordinated schedule.

Down, continuation, Up and Cancel identify the original press, selected action and executor. The original owner can receive the terminal notification after its short-lived grant has ended. This notification conveys no new mutation permission. Observers receive correlation identifiers only. Mutation entry points validate a separately issued, client-owned authorization. A release can finish the selected action once and cannot select a competing region action.

Region dwell, release or another observed condition can also initiate an action without a new button edge. Such candidates enter an explicit HIGGS proposal round with named hand/resources, coherent source observations and the same eligibility and priority rules. They do not fabricate input or bypass grants. A release retains its original sequence and decision: the release of an already consumed draw press cannot become a fresh storage action.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyEffectiveInputPriority(ClientToken, const InputContext*, InputPriority*);

Status CopyInputDecision(ClientToken, uint64_t sequenceId, InputDecision*);

Status CopyInputContinuation(ClientToken, uint64_t pressSequenceId, InputContinuation*);

Status RequestHaptic(ClientToken, const HapticRequest*, RequestId* outRequest);

// Events: InputDecisionMade.

}
```

### Hand state and coherent observations

HIGGS exposes two independent closed state fields. Mods cannot register additional state values:

| Occupancy | Meaning |
|---|---|
| Empty | Confirmed absence of both a HIGGS hold and a game item in the hand |
| GameEquipped | An item is being used through game equipment |
| HiggsHeld | HIGGS physically holds an object |
| Both | Both kinds of occupancy are confirmed |

| Availability | Meaning |
|---|---|
| Ready | No known prohibition on a new interaction; the specific action is checked separately |
| Reserved | An agreed operation owns hand use; transitions follow its release/transfer rules |
| Unavailable | A cause of hand unavailability is known |
| Unknown | Required observations are insufficient or contradictory |

An item merely selected in inventory does not prove hand occupancy: HIGGS considers its actual use and weapon state. A record marks which occupancy observations are established; unresolved occupancy must not be reported as Empty, and availability remains Unknown. Known items can remain observable even when permission is unknown. A null pointer alone does not prove a free hand.

Climbing and weapon support do not become new hand states. HIGGS expresses them through shared operations and resource ownership; the owner's identity aids diagnostics without extending the state vocabulary. Unknown cannot discover arbitrary private mod state: synergy participants must use the shared operations, while legacy integrations rely on verified observations/adapters. Missing information cannot establish compatibility.

**Pseudo-ABI**

```cpp
namespace HIGGS {

enum class HandOccupancy : uint32_t { Empty, GameEquipped, HiggsHeld, Both };

enum class HandAvailability : uint32_t { Ready, Reserved, Unavailable, Unknown };

}
```

CopyHandState names physical left/right separately from dominant/off hand. An input or release observation identifies its matching pose sample: Body Slots tests its geometry against a coherent observation rather than combining an earlier drop with an arbitrary current pose. A post-update callback alone cannot guarantee this. Regrabbing, consuming, loading or losing tracking invalidates pending offers before execution.

The original weapon type, actual game equipment location and physical grip are separate observations. A greatsword can remain the original greatsword while a supported equipment executor places it in one game-hand slot; two physical hands supporting one weapon are different from two separately equipped weapons. Consumers must not reconstruct occupancy or the owning physical hand from the base weapon type alone. Cross-hand drawing names the exact item, previous equipment location and destination hand, with an explicit policy for an occupied destination.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyHandState(ClientToken, PhysicalHand, HandState* outState);

Status CopyReleaseObservation(ClientToken, uint64_t releaseId, ReleaseObservation* outRelease);

// Events: InteractionChanged.

}
```

Motion samples distinguish controller movement in VR tracking space from item movement in world space, and identify the tracking-to-world transform version. Locomotion, teleportation and recentering may change that transform without replacing the world or body. Contact snapshots carry source, object, point, normal, motion and body generations; contact with a plant or wall is distinct from an NPC hit.

HIGGS supplies authoritative probes for its resolved palms, fingers and held-item parts, and qualified physical surface queries for support or a secondary grip. VRIK supplies its virtual-hand probes. The source owns each sample's meaning, validity and lifetime; consumers may transform a copied sample with declared provenance, but cannot substitute a controller point for an unavailable palm or relabel a logical overlap as a physics contact.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyMotionSamples(ClientToken, PhysicalHand, PoseSource, uint32_t space, MotionSample* outSamples, uint32_t capacity, uint32_t* outCount);

Status CopyContactSnapshot(ClientToken, const ContactQuery*, ContactSnapshot*);

Status QueryHandSurface(ClientToken, const InteractionProbe*, const SurfaceQuery*, SpatialObservation* outObservation);

// Events: ContactChanged.

}
```

### Shared hand-use operations

The HIGGS ABI version fixes the catalogue of shared operations: object grab, release, item transfer, secondary weapon support, world-support grip, game-equipment use, consumption and coordinated hand-pose goals. Gameplay scenarios compose those operations. A new mod cannot register an arbitrary physical control kind: a new common primitive requires an explicit HIGGS version/capability extension. Drive parameters, constraints and suppression configure supported operations rather than creating a second grip mechanism.

A climbing mod requests HIGGS's standard world-support grip and moves the player after confirmation. HIGGS owns physical support, hand use and release. Multi-hand operations are coordinated together. The engine or an equipment mod may execute equipment changes, but the shared hand model and operation grant belong to HIGGS.

A revision-checked transition can add or relinquish the second hand of an existing use. A failed acquisition leaves the first grip and its physical support intact. Relinquishing one hand drains only its dependent effects before releasing that resource. The request names the complete resulting resource footprint and any equipment or support identities that must remain valid.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RequestHandUse(ClientToken, const HandUseRequest*, Lease* outUse, RequestId* outRequest);

Status RequestEndHandUse(ClientToken, Lease use, RequestId* outRequest);

Status RequestHandUseTransition(ClientToken, Lease use, uint64_t expectedRevision, const HandUseTransition*, RequestId* outRequest);

// HandUseRequest names a supported operation kind; no custom physical driver.
// Events: HandUseChanged.

}
```

### Grabbing release and transfer

Grab, release and transfer requests name the hand and report what actually happened. An inventory-to-hand transfer confirms inventory commitment and physical grip separately, including item reference, quantity and final disposition. It reserves source, destination and possession, preserves item instance data and uses one authorized inventory mutation path exactly once. If gripping fails after removal, the assigned executor applies the declared compensation. Retrying or updating a preview cannot create another item; arbitrary script side effects prevent a promise of full rollback.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RequestGrab(ClientToken, const GrabRequest*, RequestId* outRequest);

Status RequestRelease(ClientToken, const ReleaseRequest*, RequestId* outRequest);

Status RequestTransfer(ClientToken, const TransferRequest*, RequestId* outRequest);

// Events: ActionResult.

}
```

### Physical constraints

Grip, drive and constraint changes apply at the documented safe phase and have receipts. Temporary effects name their owner, hand/item and lifetime. Suppression limits named features without releasing an item; release is a separate request. Ending a climb removes only its owner's effects, leaving another consumer's restrictions intact.

A game-command policy names both the command and its verified origin. Preventing trigger-driven weapon drawing requires a boundary that identifies that command without swallowing other trigger uses. The provider advertises that precise capability only when it can enforce it. An unsupported command or unproved origin returns Unsupported before changing input or game state.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RequestGripChange(ClientToken, const GripChange*, RequestId* outRequest);

Status RequestHandGoal(ClientToken, const HandGoal*, Lease* outGoal, RequestId* outRequest);

Status RequestDrivePolicy(ClientToken, const DrivePolicy*, Lease* outPolicy, RequestId* outRequest);

Status RequestHandGeometry(ClientToken, const HandGeometryRequest*, RequestId* outRequest);

Status RequestConstraint(ClientToken, const ConstraintRequest*, Lease* outConstraint, RequestId* outRequest);

Status RequestSuppression(ClientToken, const SuppressionRequest*, Lease* outLease, RequestId* outRequest);

Status RequestGameCommandPolicy(ClientToken, const GameCommandPolicy*, Lease* outPolicy, RequestId* outRequest);

// Events: ConstraintChanged.

}
```

### Domain executors

Consumers register an execution route; recognition and eligibility callbacks do not mutate inventory. A result is accepted only from the assigned executor, with matching request provider and epoch. Duplicate reports are ignored and a committed stage cannot regress. HIGGS forwards the receipt while preserving that executor's authority over the operation.

An equipment transition declares every affected game location and physical hand, exact item instance when known, original/proxy/display identity, expected before-state and requested after-state. Its receipt reports each location and physical binding separately. A form match alone cannot confirm an exact instance or the pair greatsword-left/shield-right. Executors advertise supported layouts and instance precision, reject unsupported layouts before mutation, and retain partial results if only one change succeeded. A consumer can supply a qualified equipment executor through this same route.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RegisterActionExecutor(ClientToken, const ActionExecutorInfo*, ActionExecutorCallback, void* userContext, ExecutorId* outExecutor, Lease* outRegistration);

Status ReportExecutionResult(ClientToken, const ExecutionResult*);

Status RequestEquipmentTransition(ClientToken, const EquipmentTransitionRequest*, RequestId* outRequest);

Status CopyEquipmentTransitionOutcome(ClientToken, RequestId, EquipmentTransitionOutcome* outOutcome);

Status ReconcileExecution(ClientToken, const ExecutionRecoveryRequest*, RequestId* outRequest);

}
```

## VRIK body and hand pose

### Body and hand pose

VRIK owns the virtual body, calibration and virtual hand poses. **IK means inverse kinematics:** head and hand positions guide calculation of the remaining skeletal pose, including shoulders and elbows. HIGGS manages physical hand use and supplies authorized grip goals; VRIK coordinates the visible pose with them. Zones, slots, their content, activation and presentation belong to Body Slots. Player locomotion remains with its gameplay owner.

Tracked controller, HIGGS resolved palm/grip and VRIK virtual hand are distinct observations with time, validity, space and generation. VRIK supplies virtual hand and body-anchor poses; HIGGS determines hand state and availability, and Body Slots tests the selected probe against its geometry. Hand-use goals go through HIGGS; VRIK remains the sole skeletal pose solver. Final IK must not feed back as the same cycle's physical target.

**Pseudo-ABI**

```cpp
// Optional VRIK pose service, acquired after SKSE PostLoad.
struct IVrikPose001 {
    virtual bool CopyFinalPose(const AnchorKey* keys, uint32_t count,
        PoseStamp* stamp, WorldPose* poses) const = 0;
};
```

A belt pouch needs the belt anchor and the reaching wrist from the same completed avatar update. One main-thread call copies both into caller-owned buffers. Keys identify the physical left or right virtual wrist, or an exact named player skeleton node. Each output has a validity bit and a world transform in Skyrim units. All valid entries share a stamp with the pose sequence, skeleton/calibration generation and monotonic capture time. A missing node invalidates its entry. A load or recalibration invalidates the old stamp until VRIK publishes another final pose. Repeated reads of a completed pose return its original stamp.

Keep the familiar SKSE message to the VRIK recipient and its pointer-returning discovery pattern. Give pose discovery its own message type and require an explicit supported reply before calling the single virtual method. An older provider can ignore the message. Existing clients can continue to request IVrikInterface001. A pouch can poll from its existing post-pose callback. This first interface needs no client registration or new subscriptions.

The VRIK 0.8.8 game download and its separately published 0.8.7 interface archive expose different kinds of information. The native 001 header has 28 methods. Papyrus also exposes hand coordinates. Neither interface identifies an update in which the visible wrist orientation and the belt transform were both finalized. An independent Body Slots adapter can already read named skeleton nodes after the HIGGS post-VRIK update and withdraw the slot when a node is unavailable. Features requiring VRIK-certified pose timing can use a documented sampling point if the maintainer confirms one, or negotiate the optional query above. A simulated pouch needs the transform of its equipped mesh after simulation. Its observation has its own time and validity. [Published VRIK files](https://www.nexusmods.com/skyrimspecialedition/mods/23416?tab=files)

## PLANCK NPC physics and hits

### Actor bodies and attachment points

PLANCK owns NPC physical animation, ragdoll transitions and body lifetime. It supplies actor/bone/body metadata and validated attachment endpoints. PPB supplies anatomical surface geometry and fitting proposals; PLANCK remains the publication owner for bodies in its ragdoll and validates those proposals without creating a competing ragdoll. A penetration mod owns wound and extraction rules; PLANCK validates the target body and HIGGS applies the weapon constraint.

**Pseudo-ABI**

```cpp
namespace PLANCK {

Status CopyActorPhysics(ClientToken, ActorHandle, ActorPhysics* outState);

Status RequestActorGeometry(ClientToken, const ActorGeometryRequest*, RequestId* outRequest);

Status AcquireAttachmentEndpoint(ClientToken, const AttachmentQuery*, AttachmentEndpoint* outEndpoint, Lease* outEndpointLease);

// Events: ActorPhysicsChanged, EndpointInvalidated.

}
```

Endpoints identify world, actor and body generations. Ragdoll replacement, NPC unload or game load invalidates dependent endpoints and constraints safely. An actor policy affects its declared scope instead of changing thresholds for every weapon. Flora and scripted activators need PLANCK only when their feature actually interacts with its actors.

**Pseudo-ABI**

```cpp
namespace PLANCK {

Status RequestActorPolicy(ClientToken, const ActorPolicy*, Lease* outPolicy, RequestId* outRequest);

// Events: ActionResult.

}
```

A pocket anchored to a named NPC bone only needs a read of the current body transform, its observation time and an identity that changes when the body is replaced. A reader can then stop offering the pocket or bind it again. It can do this without acquiring an attachment or subscribing to hits. A pose copied on demand is dated at the read; an earlier physics step needs its own timestamp.

### Physical hits

PLANCK owns interpretation and commitment of its physical hits. A contact record names its actual stage, such as a potential contact before filtering or a confirmed solver observation when available. Hit candidate, policy decision, native hit-handler invocation and independently observed damage are separate results. A handler invocation does not certify health loss. Read-only contact/hit observation cannot veto an event or change physics, including when the observer queue is full. A bounded policy runs only at a declared precommit boundary and modifies a copied decision rather than live engine objects.

**Pseudo-ABI**

```cpp
namespace PLANCK {

Status RegisterHitPolicy(ClientToken, uint64_t scope, HitPolicyCallback, void* userContext, Lease* outRegistration);

Status CopyHitSnapshot(ClientToken, uint64_t hitId, HitSnapshot* outSnapshot);

Status CopyContactSnapshot(ClientToken, const ContactQuery*, ContactSnapshot* outSnapshot);

// Events: ContactObserved, HitCandidateObserved, HitDecisionMade, NativeHitInvoked.
// DamageObserved is a separately negotiated, evidence-backed capability.

}
```

Numeric damage adjustment is separately negotiated for a verified hook and declared fields. Alternative adjustment providers are selected by user priority and stable identity. Vetoes and upper limits compose by an order-independent rule. The receipt names the selected policy and applied fields. A hook that only accepts or vetoes a hit cannot claim numeric control. Passive lifecycle notifications are delivered after leaving external physics locks. Immediate precommit policies declare their thread and restricted calls explicitly.

## PPB anatomical surfaces and touch

Precision Physic Bodies (PPB) supplies the anatomical geometry layer: which surfaces a body has, how its capsules fit the actual model, and which body part a contact concerns. VRIK produces the player's body pose; PPB interprets surface geometry; HIGGS owns hand use; PLANCK owns NPC physical animation and live body lifetime. A named forearm contact is evidence for an action, not permission to grab, equip or damage.

### Surface geometry and fitting

PPB owns anatomical shape definitions, fitting to supported meshes/skeletons and classification of body regions. Its proposed observation contract identifies actor, surface, side and child, effective world geometry, units, sample phase/time, validity and model/body/shape generations. A surface key is qualified by its provider and generations; a bare capsule index is not a permanent saved identity. The contract also declares coverage: an unsupported body is unavailable, rather than a body with no contact.

**Pseudo-ABI**

```cpp
namespace PPB {

Status CopySurfaceSnapshot(ClientToken, const SurfaceQuery*, SurfaceSnapshot* outSurfaces, uint32_t capacity, uint32_t* outCount);

Status CopyCoverage(ClientToken, ActorHandle, SurfaceCoverage* outCoverage);

Status CopyShapeProposal(ClientToken, const ShapeFitQuery*, ShapeProposal* outProposal);

// Events: SurfacesChanged, SurfacesInvalidated.

}
```

The physical owner is identified per world and body. In PPB 2.9 beta, an NPC can have a private physics world stepped by PPB workers. Under this proposal, PPB would publish completed samples for the bodies it owns there and identify when those samples expire. The safe sampling point for 2.9 still needs confirmation from its author. PLANCK validates requests against its own ragdoll bodies. A consumer joining observations across worlds needs both source identities, generations, units and comparable capture times. A pending worker step cannot supply a completed hand contact or a PLANCK hit. [PPB beta release notes](https://github.com/Telord72612/Precision-Physic-Bodies/releases/tag/V2.9.0-beta)

PPB computes proposed shape changes as data. In the target architecture, PLANCK validates and applies geometry to the NPC bodies it manages at its safe mutation phase; HIGGS does so for its hand/finger bodies. Requests carry the required model/body generations and geometry revision; the physical owner reports whether and when the change took effect. Each physical body has one publication/lifetime owner. PPB can fit and read back the applied shapes without installing another hand actuator, replacing the same body independently or changing another provider's global settings. This is a proposed integration boundary: current PPB also creates hand colliders and chains physical updates itself.

### Touch evidence and action authority

PPB reports the target anatomy and contact evidence; HIGGS supplies the authoritative hand state and relevant hand/item identity. Contact source classification describes the observation, not another extensible hand-state machine. A proposed contact record distinguishes proximity, geometric overlap and engine-confirmed contact, names its probe and sampling phase, and explicitly marks unavailable contact point/normal/impulse fields. No consumer may infer physical force from geometric penetration alone.

**Pseudo-ABI**

```cpp
namespace PPB {

Status CopyTouchSnapshot(ClientToken, const TouchQuery*, TouchSnapshot* outContacts, uint32_t capacity, uint32_t* outCount);

Status SubscribeTouches(ClientToken, const TouchSubscription*, TouchCallback, void* userContext, Lease* outSubscription);

// Events: TouchBegan, TouchChanged, TouchEnded, TouchStreamReset.

}
```

A contact may enable a gameplay candidate. HIGGS selects conflicting hand actions; PLANCK interprets and commits its physical hits. PPB adds anatomical detail to the corresponding observation without committing a second hit. PPB geometric proximity or overlap is not evidence of a Havok contact, impulse or damage event; an engine-confirmed contact retains its actual physics source. PPB's existing push and equipment features can remain gameplay providers: they retain reaction thresholds and clothing rules, while requesting hand operations from HIGGS, NPC policies/reactions from PLANCK and equipment changes from the assigned executor. Observing a touch never independently authorizes all listeners to act.

PPB scan IDs, HIGGS contact IDs and PLANCK hit IDs belong to separate namespaces. Joining them requires an explicit witness from the physical owner or a qualified bridge for the specific event, source worlds, body generations, applied geometry revision and child shape where known. Matching actor, weapon and frame is insufficient when two contacts occur in one step. An uncertain join remains unclassified and preserves the original hit result. A PPB private-world overlap cannot certify a contact in the PLANCK world.

### Existing API and the proposed extension

The [public source](https://github.com/Telord72612/Precision-Physic-Bodies) already includes [IPpbTouchInterface1](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/src/PpbTouchAPI.h#L446), acquired through SKSE messaging. It supplies coverage queries, effective capsule endpoints/radius in world game units, anatomical names and digest/raw touch observations. Native calls are main-thread only. This is a useful integration foundation, not an API that needs to be invented from nothing.

PPB's existing `slot` field addresses an anatomical body group, not a Body Slots interaction slot. Current reference-name queries are not a universal actor-specific surface catalogue; consumers must validate the actual body's readable geometry and coverage.

Its [current touch contract](https://github.com/Telord72612/Precision-Physic-Bodies/blob/e26b4ebc8061e8736b344e4f3e9216dea9254747/INTEGRATION.md#L545) covers player-to-NPC contact on supported bodies. It does not promise complete player-body target geometry, NPC-to-NPC touch, custom surface registration or a shared physics-owner handover. Digest and raw reports are grouped and dwell-filtered; neither is a complete instantaneous per-capsule overlap set. Digest depth summarizes the visit, and callbacks follow the configured reporting rate. A new immediate observation/lifecycle contract must not silently replace those existing semantics. Coherent generations, subscription removal/reset, actor-specific surface identities and coordinated physical publication are extension candidates, not delivered services.

PPB has a distinct role in the synergy, but a mod requires it only when it needs its anatomy or contact data. Body Slots keeps its own access volumes and does not require skin contact: a pouch opening may lie in front of the body. An optional anatomical anchor or body-fit adjustment can use PPB only where coverage is verified; player-body support must be added and qualified separately. Losing PPB invalidates dependent observations, not unrelated node-anchored slots. Morphbench can author and compare capsule geometry alongside slot placement; it does not become the in-game geometry or interaction owner.

Two reads illustrate the boundary for PPB. A fitted capsule tells a mod where a surface lies, even when that capsule is disabled for collision at the current level of detail. A numbered touch scan tells it whether another observation has been published since the last poll. A contact flag retained from an earlier scan cannot establish which child shape was hit in a particular physics step. PPB can expose what it knows about anatomy and timing while HIGGS arbitrates a hand action and PLANCK interprets its own physical hits.

## Body Slots all slot mechanics

A slot is a registered, named interaction region with offered actions and activation rules. Its anchor may be the player, a particular NPC, an equipment instance, a world object, a provider surface or a transient interaction. Storage, visuals and saved customization are optional: a mouth offers consumption, an NPC pocket offers inventory access, and a temporary selection wheel offers choices without becoming a persistent player holster. Entering a region changes spatial context; an authorized button, release or dwell-driven proposal may select an action. The region, visible bag and inventory container remain separate entities: one bag may supply several slots, and a slot need not own any stock.

**Body Slots** provides an extensible logical region/slot system without VRIK's architectural fourteen-holster or weapon-type restrictions. It owns creation, anchor binding, logical geometry evaluation, entry/exit, hover and spatial dwell, action offers, activation, optional presentation and saved customization. VRIK supplies the player's body and virtual hand poses; HIGGS supplies hand state, input, grants and physical actuation. Configuration and behavior providers define content types and gameplay responses, executed through the shared operations of the appropriate services. Physical surface observations retain their HIGGS/PLANCK owner and anatomical fitting retains its PPB owner. Practical resource and geometry limits remain explicit capabilities. Offline authoring and the creation API are two ways to supply regions/slots, rather than offline editing being the purpose of the service.

VRIK's original fourteen holsters remain an existing system. New Body Slots slots do not require replacing them. In a full transition mode, native holsters are disabled and replaced by fourteen Body Slots counterparts with matching accepted items and positions, plus mod-defined slots. Settings and assignment mappings require separate validation; only one interaction/presentation path for each equivalent slot is active. Coexistence explicitly coordinates competing holsters through available controls rather than assuming HIGGS automatically governs unchanged VRIK. Full handover remains a target compatibility mode, not a prerequisite for new slots or a delivered prototype capability.

### Visual editor before launching the game

The primary workflow is **offline visual editor → editable project and versioned slot package → Body Slots binding, zone tests, interaction and presentation; body/hand pose from VRIK, hand state and input arbitration from HIGGS**. The editor loads a reference body/skeleton or equipment model. The author selects a named anchor, places and rotates a slot, edits its independent access region and declares scale behavior, physical-hand access, slot kind, allowed contents and optional visuals. For a wearable potion bag, the author can place the access region at its opening before launching Skyrim.

The editable project retains the authoring data; the export is a versioned declarative package that the runtime can validate and load. Neither Skyrim nor VRIK is required to author, reopen or export definitions. A mod may ship such a package without calling a live creation API. The format records stable provider/slot keys, schema and reference-rig versions, anchor names, local transforms, geometry/probe declarations, units, axes, scale conventions, policy bindings and asset references. Engine pointers and save-specific handles never enter the package; live eligibility callbacks cannot run in the editor.

The selected authoring tool is [Morphbench](https://github.com/Vhodnoylogin/morphbench), with a separate Slots mode that reuses its model and skeleton viewer. The mode edits node-relative placement and exports Body Slots definitions; it does not become another runtime slot manager. Skeleton node anchors and physics bodies remain distinct. A missing node cannot be represented as a valid preview at an invented transform. Delivered functionality and remaining authoring requirements are recorded in the [implementation ledger](implementation-status.md).

Offline placement provides a default, not proof of reachability on every character. The package declares reference-to-runtime mapping and explicit conversion/rebinding rules. Body Slots applies them to observed VRIK body proportions/calibration and equipment movement. A missing anchor makes the slot unavailable instead of silently selecting another bone. Player overrides remain separate from the author's package; runtime ergonomics still need game checks.

### Slot definitions instances and configuration

Body Slots owns creation and the definition/instance registry: stable provider-qualified keys, named anchors and local placement, independent access geometry, slot kind, generic allowed-content filters and optional customization persistence. A reusable definition is separate from its instance on a particular actor, equipment item, world reference or transient owner. Definitions/profile defaults, save-scoped instances and player overrides remain separate. Each definition declares profile/save/session/operation scope and lifetime; transient regions are not serialized merely because they use the slot contract. Re-registration preserves durable overrides; runtime IDs and reservations are recreated after loading. Creating a runtime instance is distinct from authoring its reusable definition.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status RegisterSlotDefinition(ClientToken, const SlotDefinition*, SlotDefinitionId* outDefinition);

Status CreateSlotInstance(ClientToken, const SlotInstanceRequest*, SlotId* outSlot, RequestId* outRequest);

Status CopySlotConfiguration(ClientToken, SlotId, SlotConfiguration* outConfiguration);

Status UnregisterSlot(ClientToken, SlotId, RequestId* outRequest);

// Events: SlotConfigurationChanged.

}
```

Type, keyword and form-list filters support weapons, shields, torches, potions, food and tools. Registered bounded eligibility rules refine this generic policy; they do not mutate inventory. Body Slots owns policy registration and its configuration version; Body Slots uses a coherent policy snapshot and invokes registered eligibility checks at the declared phase; hand state and permission come from HIGGS. Callback ownership stays with the registry registration; serialized definitions do not carry executable callbacks. Consumers keep potion selection, stock, original/proxy semantics and assignment persistence. Exclusive/shared bindings declare who may offer an action without making the registry an inventory executor.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status RequestSlotCustomization(ClientToken, SlotId, const SlotCustomization*, RequestId* outRequest);

Status RequestSlotVisualDefinition(ClientToken, SlotId, const SlotVisual*, RequestId* outRequest);

Status BindContentPolicy(ClientToken, SlotId, const SlotContentPolicy*, ContentEligibilityCallback, void* userContext, Lease* outBinding, RequestId* outRequest);

Status EvaluateSlotEligibility(ClientToken, const SlotEligibilityQuery*, SlotEligibilityResult* outResult);

}
```

### Slots supplied by equipment

An equipment mod supplies records, assets and a declared slot set authored on its reference model. Body Slots reconstructs runtime instances from durable keys, preserves their declared customization and follows their anchors on the actual wearable model. The key combines the provider definition, particular equipment instance and slot within it, so identical bags keep separate assignments. The equipment provider validates durable save-scoped instance keys and declares stack split/merge, cloning and ownership-transfer rules. Rebinding restores one set; missing equipment, providers or unresolved identity leave it unavailable without attaching it to another item. Body Slots saves placement/visual overrides; content providers save their own rules under the same key.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status BindEquipmentSlotSet(ClientToken, const EquipmentSlotBinding*, SlotSetId* outSet, RequestId* outRequest);

}
```

### Zones

A bag or scabbard may supply its slots only while equipped. Its access volume can follow a dedicated opening node, including movement produced by FSMP, while saved customization stays local to that anchor. Body Slots observes the resulting pose; the accessory physics owner continues to drive it. Sampling must have explicit post-physics timing/freshness, and deformation of a skinned mesh is not automatically one rigid anchor. Ordinary sway is not anchor replacement and does not rewrite saved placement. Geometric entry alone never authorizes an action; a started transfer remains tied to its slot/item despite small motion, while removal or replacement requires explicit invalidation. Holding the accessory with a HIGGS hand needs an agreed physical bridge rather than two simultaneous drivers.

A zone is the spatial part of an interaction region, evaluated against an explicit probe. A probe may be a virtual hand, resolved palm, finger, held-item part, blade or projectile, with a source-owned sample or interval. Body Slots owns the resulting logical region snapshot and retains the underlying observation's provider, sample ID and validity. Containment, surface proximity, relative pose, a ray/sweep result and an actual physical contact are different spatial facts. Geometry remains observable independently of occupancy, visibility, reservation and available actions. Snapshot queries also serve clients subscribing after entry.

The spatial contract advertises supported forms and queries rather than reducing every interaction to a point in a sphere:

| Capability | Required distinction |
|---|---|
| Sphere, capsule, oriented box and finite cylinder | A finite cylinder has flat end caps; an NPC waist or keyhole tolerance must not silently become a rounded capsule |
| Provider surface reference or coherent surface snapshot | PPB anatomy and HIGGS/PLANCK physical geometry keep their source, coverage and applied revision; a logical access region does not install another collider |
| Point/shape containment and surface distance | The chosen hand/item part and the query tolerance are explicit; geometric proximity does not establish Havok contact |
| Rays, shape sweeps and directed crossings over an interval | A blade crossing or moving projectile names start/end samples, motion and direction; polling isolated points is not equivalent |
| Relative position/orientation and coordinated probes | A key's alignment, a permitted secondary-grip segment or a two-hand harvest condition cannot be represented by one unconditional radius |

Advanced queries are negotiated capabilities of the service/provider that can perform them. HIGGS retains physical hand/surface queries and physical support; PLANCK retains NPC contact queries. Body Slots evaluates or correlates the supported logical spatial condition using their observations. Unsupported forms, missing coverage and stale samples produce an unavailable result, not a fabricated `Outside` or absence of contact. An arbitrary physics body, ray or gesture need not be registered as a slot; climbing need not register the entire landscape.

Anchors explicitly distinguish player head/HMD, pelvis or torso, a particular NPC/bone, an equipped item instance/opening node, a world reference/node, a provider surface and a temporary interaction frame. Local transforms declare axes, units, scale inheritance and rotation policy, including full head tilt versus yaw-only placement. A head-relative native region must not silently become pelvis-relative. A simple NPC bone anchor can work without PPB; anatomical fitting requires verified PPB coverage for the player, living NPC, corpse or inactive body concerned. Target discovery, climbability, food, crime and harvest rules stay with consumers.

Every region snapshot and probe names world/provider epochs, actor/item/model/body/anchor generations, geometry/configuration revisions, coordinate frame and transform version, units and effective scale, time, frame/physics tick and sampling phase. The declared sampling/freshness rules determine whether moving anchors and probes can be compared. A pointer or capsule index is not a saved identity. True exit, anchor removal, model/body replacement, tracking/provider loss and load have distinct invalidation reasons; stale events cannot revive a removed instance, and data loss is neither normal release nor successful completion.

Entry/exit thresholds, spatial dwell, tolerated gaps and rearm policy are separate settings. Body Slots measures region dwell; HIGGS recognizes a button hold/multi-tap; the gameplay owner controls action debounce or repeat-hit delay. Polling cadence is not dwell, and PPB's grouped visit duration retains its own source semantics. Different equip/unequip radii may describe different actions rather than hysteresis for one region.

`Inside` means the probe satisfies the spatial test. `Eligible` means the domain rules allow an offered action. `Granted` means HIGGS has authorized the required hand/input use. A confirmed executor result establishes actual commitment or completion. None implies the next. Overlapping mouth, collar and bag regions publish offers; geometry evaluation never independently consumes an item or takes a hand.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status RegisterInteractionRegion(ClientToken, const RegionDefinition*, RegionId* outRegion);

Status BindRegion(ClientToken, RegionId, const AnchorReference*, RequestId* outRequest);

Status QueryRegion(ClientToken, RegionId, const InteractionProbe*, RegionObservation* outObservation);

Status CopyRegionSnapshot(ClientToken, const RegionQuery*, RegionSnapshot* outRegions, uint32_t capacity, uint32_t* outCount);

Status RemoveRegion(ClientToken, RegionId, RequestId* outRequest);

// Events: RegionEntered, RegionChanged, RegionExited, RegionInvalidated.

}
```

### Runtime slot binding

Body Slots owns both definitions and all runtime region/slot mechanics. It resolves named player, NPC, equipment, world and transient anchors through the declared provider, computes world placement and evaluates supported probes. VRIK observations apply to the player's body; game nodes, PPB anatomy and PLANCK physical endpoints have their own validity and coverage. Body Slots reports registration/geometry readiness independently of hand activation and optional presentation readiness. Definitions and geometric queries can be available while HIGGS is absent or initializing; action activation waits for HIGGS grants. This separation avoids a hard circular initialization dependency. The pose adapter supplies observations without owning regions or slots.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status BindSlotRuntime(ClientToken, const SlotRuntimeBinding*, Lease* outBinding, RequestId* outRequest);

Status CopySlotBinding(ClientToken, SlotId, SlotRuntimeSnapshot* outBinding);

// Events: SlotInstanceAvailable, SlotInstanceUnavailable.

}
```

Body Slots recognizes in-game slot dragging through authorized HIGGS input and applies an expected-revision customization request. It can display a provisional drag, but a rejected edit restores the committed placement. Equipping a bag makes its registered slots available only after model binding; unequipping or model loss stops new offers and cancels uncommitted work. Already delivered items retain their disposition, and the content provider handles unequip policy through the authorized executor.

### Slot visuals

Body Slots stores bag/holder visual configuration and manages its presentation separately from item previews. The binding identifies whether equipment or the Body Slots presentation service renders the bag, preventing duplication. Model-defined access/grip regions are explicit; model bounds do not automatically become activation geometry. Visual changes version their anchors and report readiness/failure; preview readiness identifies the displayed content version. New slots and their visuals do not have to enter VRIK's native holster system.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status RequestItemPreview(ClientToken, SlotId, const ItemPreview*, RequestId* outRequest);

Status CopySlotPresentation(ClientToken, SlotId, SlotPresentationSnapshot* outPresentation);

// Events: PresentationReady, PresentationFailed.

}
```

### Content providers

Body Slots supplies generic content filters and bindings to registered behavior providers. Slot content types and gameplay responses are configurable; hand states and shared hand-use operations come from the closed HIGGS contract. A slot can accept only potions or route food interaction through a standardized HIGGS consumption service. Reusable handlers cover ordinary pouches and holsters; custom handlers express gameplay through the same shared operations. BindSlotContent attaches a provider to a Body Slots slot and references the single registry policy. ImportNativeSlotDefinition is an optional import of a native holster definition through a validated mapping, not creation of a new slot inside VRIK. Releasing callback-bearing bindings drains their calls.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status ImportNativeSlotDefinition(ClientToken, const LegacySlotMapping*, SlotDefinitionId* outDefinition, RequestId* outRequest);

Status BindSlotContent(ClientToken, SlotId, const SlotContentBinding*, Lease* outBinding, RequestId* outRequest);

}
```

A slot can represent stored content or a selection from shared inventory. A preview creates no extra stock. Two hands requesting the last matching item reserve the same instance or quantity through the content executor. Operations name the physical hand and all required resources; original item, equipment proxy and displayed model retain separate identities. The proxy provider preserves enchantment, charge, tempering and ownership.

The provider offers eligible actions from an immutable context without changing state. Body Slots correlates those offers with current geometry and submits candidates to HIGGS; HIGGS grants final permission to use the hand and input. After selection, Body Slots dispatches the request to one assigned executor. Zone entry or a broadcast button event does not authorize observers to perform competing actions. Body Slots publishes committed content from the executor's receipt, validating the expected slot version; previews name that version. An unrelated or stale receipt cannot update the slot. HIGGS coordinates physical possession and transfer results.

Initial assignments, current stock and externally changed content are published as versioned provider observations, with their source and freshness. They are not fabricated transfer receipts; only an actual executed operation can report commitment. A region without content needs neither a content mirror nor a preview.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status CopyActionOffers(ClientToken, const SlotRequest*, ActionOffer* outOffers, uint32_t capacity, uint32_t* outCount);

// Reserves slot content/access only; hand and input grants belong to HIGGS.
Status ReserveSlot(ClientToken, SlotId, const SlotRequest*, Lease* outReservation);

Status RequestSlotAction(ClientToken, const SlotRequest*, RequestId* outRequest);

Status CopySlotContent(ClientToken, SlotId, SlotContentSnapshot* outContent);

Status PublishSlotContentObservation(ClientToken, SlotId, const SlotContentObservation*, RequestId* outRequest);

Status UpdateSlotContentFromCommit(ClientToken, SlotId, const SlotContentCommit*, RequestId* outRequest);

// Events: ActionOffersChanged, SlotContentChanged.

}
```

Ineligibility before selection can allow another candidate. Once accepted, commitment, completion, failure and cancellation remain distinct; a failure after execution starts must not automatically dispatch another action on the same input. For overlapping zones, Body Slots offers candidates with explicit slot priorities; HIGGS arbitrates them alongside other hand actions under the user's rules.

### Registry pose adapters and executors

The definition API and observation adapters are separate. Body Slots receives player hand/body-anchor poses from an available VRIK adapter or another compatible pose provider, and NPC/world/surface observations from their declared source; HIGGS always supplies hand state and permissions. An adapter does not give its source ownership of regions or slots. Body Slots computes logical geometry and manages transitions, activation, bindings and optional presentation. Frame work uses an immutable configuration revision; queued actions revalidate it, source observations, model/body/anchor generations and the HIGGS grant before commitment.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status CopyDefinitionChanges(ClientToken, const DefinitionCursor*, SlotDefinitionSnapshot* outRecords, uint32_t capacity, uint32_t* outCount);

Status CopySlotRuntime(ClientToken, SlotId, SlotRuntimeSnapshot* outRuntime);

// Events: DefinitionAvailabilityChanged.

}
```

Each state has one authoritative writer: HIGGS for hand state/use and physical hand bodies, VRIK for body/virtual-hand pose, PPB for anatomical definitions/fitting and touch interpretation, PLANCK for live NPC physical bodies, Body Slots for definitions, configuration, geometry, binding, interaction and the committed-content mirror/presentation, and the content executor for gameplay assignments/inventory mutation. Configuration, content and presentation versions and body/anchor generations are distinct. Drag, MCM and API edits use expected revisions; callbacks run at declared phases outside registry locks. Provider removal stops new calls and drains in-flight callbacks before releasing code/context. Changes are queued at declared boundaries.

## Input and action sequence

1. HIGGS supplies grip/trigger samples and edges with physical hand, analog/touch distinction and an input sequence ID, plus authoritative hand/item observations. VRIK supplies coherent virtual-hand/player-anchor pose; PPB supplies required anatomical evidence and PLANCK its NPC physical observations. Body Slots resolves the declared anchor, tests the region and reports spatial state. Only HIGGS supplies hand state and availability.
2. Domain providers evaluate eligibility and offer actions from that immutable context. A button sequence or explicit no-button proposal round identifies the trigger; spatial dwell remains separate from HIGGS hold recognition and domain debounce. A hold or multi-tap recognizer reserves the sequence with a decision deadline.
3. HIGGS selects one eligible consuming action under declared contexts, bindings and user-selected priority, then grants the required resources. `Inside` and `Eligible` alone confer no grant. Observers remain able to read the input and region snapshot, but cannot independently act on them.
4. Body Slots activates the selected region/slot action; the assigned executor revalidates observations, generations and grant, then performs the operation at its legal phase. HIGGS executes supported hand/physical operations; the consumer retains its domain rules.
5. Results distinguish accepted, partially committed, committed, completed, rejected, cancelled, unknown outcome, reconciliation and confirmed compensation. They carry request/transaction IDs, the request issuer and its epoch, and the selected executor. Proven effects remain recorded through recovery. An event forwarder remains a separate identity.

Acceptance is not successful completion. Cancellation and deferred default input have declared behavior; synthetic gameplay requests never masquerade as physical button edges. Menus, missing tracking, load and provider shutdown invalidate pending work safely.

## Shared physical operation

Each owned hand/weapon body has one final HIGGS actuator. Consumers submit supported goals and constraints through its common contract without registering a private physical driver. Contact constraints, weight/impedance objectives and an accepted embedded attachment contribute to one physical recipe. They cannot be resolved by allowing each callback to overwrite the pose in turn. Havok still solves world contacts; PLANCK still controls NPC physical animation. PPB contributes fitted surface geometry and anatomical contact evidence; the physical owner applies geometry changes, and contact interpretation does not become a second physical actuator or damage commitment. A penetration provider owns wound and extraction policy while PLANCK validates the NPC attachment endpoint.

A heavy lodged weapon touching a wall is a representative combined case. The contract reports saturation, break or rejection rather than silently teleporting the hand or losing the attachment. Damage and parry policy consume committed physical observations and enter the documented hit decision path once.

A climbing support point belongs to the climbing action and reserves the named hand. HIGGS performs the standard world-support grip, owns the physical hold and suppresses incompatible new actions in scope; VRIK renders the coordinated pose. The climbing consumer retains the player locomotion controller. Two-hand actions validate their footprint together and do not drop another hand's item merely to simplify a new interaction. Climbing starts only after its required hand reservation, physical support and restrictions become effective; acceptance alone is insufficient. Acquisition failure unwinds provisional resources before locomotion starts. Switching actions waits for confirmed physical hand release.

## Mods used to assess ABI expressiveness

These published mods were used as concrete requirements for ABI expressiveness. They remain external consumers; their gameplay does not become framework logic. PPB now has its own provider role above, so its geometry/contact layer is part of the proposed foundation rather than merely another consumer.

The sample is illustrative, not an allowlist: the shared contracts target any Skyrim VR mod within their declared capabilities. The [expanded consumer contract review](../trinity-research/consumer-contract-matrix.md) examines twenty varied mods, including cases where no direct integration is needed, and separates evidence, required capabilities and consumer-owned gameplay.

The [interaction-region review](../trinity-research/interaction-regions-review-ru.md) supplies the spatial distinctions behind this contract. For example, Pickpocketing can register an NPC pocket with spatial dwell and temporary selection-wheel regions: Body Slots evaluates them, optional PPB anatomy fits the anchor, HIGGS grants hand use, and Pickpocketing owns inventory access, theft and crime. A food consumer can reuse a named mouth region while keeping food rules. Neither mechanic requires another hand classifier or another set of physical body colliders.

| Consumer | Requirement on the shared foundation |
|---|---|
| [Body Pouches VR](https://www.nexusmods.com/skyrimspecialedition/mods/194796) | Inventory-backed potion selection, persistent configurable slots and wearable bags, coherent release/zone observations, confirmed handover and independent slot previews |
| [Shields and 2H Weapons Unlocked](https://www.nexusmods.com/skyrimspecialedition/mods/184001) | Body Slots zones/presentation with an explicit target hand and original/proxy item identity; its equipment adapter keeps the proxy policy |
| [Swap Drop and Hold Redux](https://www.nexusmods.com/skyrimspecialedition/mods/185816) | Input recognition reservations, explicit equip/release/hand transfer, exact grip relation, authoritative holster eligibility and final result |
| [Physical Collision VR](https://www.nexusmods.com/skyrimspecialedition/mods/186335) | Coherent rendered/collision/grab/hit transforms, contact snapshots and supported integration with HIGGS's shared physical drive; optional NPC contact/parry collaboration |
| [True Wield VR](https://www.nexusmods.com/skyrimspecialedition/mods/191123) | Grip geometry, mass/inertia, drive budgets, per-hand slide, two-hand state and measured motion; candidate damage policy through PLANCK |
| [Immersive Weapon Penetration VR](https://www.nexusmods.com/skyrimspecialedition/mods/184223) | Rich hit data, actor/bone/body lifecycle, validated attachment constraints, extraction outcomes and per-weapon interaction/visibility requests |
| [Immersive Harvesting VR](https://www.nexusmods.com/skyrimspecialedition/mods/186754) | Grip selection, contact/motion data, coordinated two-hand reservations, confirmed physical delivery of harvest results and haptics |
| [VR Climbing — Aelove Ver](https://www.nexusmods.com/skyrimspecialedition/mods/170321) | Climb-start priority, occupied-hand/support state, scoped HIGGS restrictions, visual pose coordination and world generations |

These are proposed requirements derived from those consumers, not promises that their current binaries already implement this integration.

## Body Pouches with the shared services

[Body Pouches 0.2.0](https://www.nexusmods.com/skyrimspecialedition/mods/194796) already uses VRIK's body slots and HIGGS grabbing/drinking. Its default is one exclusive potion pouch; shared and multiple-slot settings have narrower testing. The pouch saves a potion-selection rule and draws from the player's inventory. Its displayed bottle is a preview, not a separate stored item. [The release source](https://github.com/Vhodnoylogin/body-pouches/tree/0.2.0) shows where a richer shared contract could simplify the integration:

| Current responsibility inside Body Pouches | What the shared services could provide |
|---|---|
| Require an enabled weapon category for detection; suspend normal holstering for exclusive pouches | Body Slots definitions with independent geometry and potion eligibility; Body Slots runtime binding and provider action rights |
| Read raw controller edges for the shared-slot fallback and reproduce withdrawal recognition | HIGGS input sequences and priority, with Body Slots spatial action offers even for empty non-weapon slots |
| Reconstruct slot bones, transforms, hand mapping and camera offsets for shared detection and diagnostics | Body Slots zone/probe snapshots, VRIK body/virtual-hand pose and the unified HIGGS hand state in a declared space and phase |
| Defer a drop to the completed VRIK/HIGGS update, guard it with local serials and cancel it on regrab/load | A correlated release observation, matching zone sample and invalidation contract |
| Drop one inventory item, request a grab and correlate a general grabbed callback within a local timeout | One transfer request with separate inventory/grip results, item identity and declared failure disposition |
| Change an art model through a spell effect, wait for removal and support one exclusive pouch preview | Independent Body Slots visuals and previews per slot, with readiness, version and attachment information |
| Coordinate its own world generations and readiness checks with saved assignments | Provider lifecycle events and stable slot keys, while Body Pouches keeps assignment serialization |

Body Pouches would still decide which potion effects match, reject poisons, order matching potions and save the player's assignments. It would keep the rule that all pouches draw from one inventory. A supported inventory executor could reuse verified CommonLib removal/pickup operations; moving those calls behind a transfer service does not move potion policy into VRIK or HIGGS. PLANCK is not required for this mechanic and should remain optional unless an added feature interacts with NPCs.

Keeping assignment persistence in Body Pouches still requires adapting its saved records: the release identifies assignments by numbered native slots. A versioned migration would map those assignments to native stable keys and store new bag assignments by equipment-slot keys, without attaching an old assignment to unrelated equipment.

This could make several visible additions practical: independently displayed pouches on several slots; explicit user priority when a potion pouch shares a location with a weapon; and an equippable potion bag whose slots follow the bag's model. A provider-declared access region near the bag opening or bottle neck could make reach behavior easier to understand. The framework would expose the tested probe and geometry for both hands; the present difference between visible bottle geometry and activation points still needs calibration and runtime checks. Providing a new ABI alone does not automatically resolve that ergonomic issue.

The simpler mod could ship slot definitions authored offline, register its content rules with Body Slots and executors with the interaction services, publish Body Slots previews, and react to confirmed results. Request correlation and invalidation would still exist, but their shared implementation would replace private reconstructions in each consumer. These are proposed changes; they do not describe additional features already shipped in Body Pouches 0.2.0.

## Shields and 2H Weapons Unlocked with the shared services

Ordinary native VRIK one-handed cross-hand drawing already handles an item that remained equipped to the other hand, subject to its hand permissions and swap settings. The proxy adaptation limitations below concern that experimental path, not a general inability of VRIK to change the drawing hand. Exact-instance identity and confirmed compound equipment outcomes remain separate requirements.

[Dynamic Two-Handed](https://www.nexusmods.com/skyrimspecialedition/mods/170549) supplies another approach: its [public implementation](https://github.com/vinymayan/Dynamic-Grip-NPC/blob/97db41fb02ffb7900be3a5bd95a7df27b55d1acf/src/Hooks.cpp#L334) temporarily changes the original weapon's equip slot around the engine operation and restores the form, instead of creating a one-handed proxy for that operation. A qualified VR equipment executor could use this approach behind the shared request/result contract. The request must specify effective equipment mode separately from original item type and physical grip; HIGGS reserves every affected player hand, Body Slots owns the sheath, and the executor confirms old/new equipment locations and exact item properties. This is a research direction, not delivered VR support or proof that every proxy use can be removed. Shared-form mutation, reentrant calls, duplicate copies, skill/damage/block behavior and restoration need separate qualification.

[Shields and 2H Weapons Unlocked — VR](https://www.nexusmods.com/skyrimspecialedition/mods/184001), by TheMachinaGod, allows shields and two-handed weapons in either hand. Its unconventional equipment uses substitute items, or proxies, and its own shoulder holsters. In the [original implementation](https://github.com/SamuelJohnBrown/Weapons-shields-unlocked-skyrim-VR-framework-/blob/7518a1879a78ca494e77cd4d60c76ca5cba77172/main.cpp), those holsters replace VRIK's two shoulder slots by disabling their item permissions; this disabling is skipped when FalseEdgeVR is present. The conflict concerns the shoulder holsters, not every VRIK slot.

The existing experimental adaptation aims to retain the equipment feature while using VRIK's own zones, placement and display through its public ABI. The tested adaptation displays a sword on the back, draws it into the right hand and preserves it across save/load. However, an empty left hand receives no offer from the occupied shoulder slot. The interface also cannot explicitly address both a particular slot and a target physical hand. Adapting the proxy's type to satisfy equipment, holster and display rules therefore requires coordinating inventory, Papyrus, models and physical state through local workarounds.

| What the original mod or experimental adaptation has to do | What shared services could provide |
|---|---|
| Original: create private shoulder zones and disable VRIK's shoulder holsters to avoid competing actions | Body Slots: equivalent managed shoulder slots, placement, content policy, interaction and presentation with provider action rights |
| Adaptation: rely on weapon-type and hand filtering before receiving an occupied slot's action offer; the empty left hand is filtered out | Body Slots: independent zone observations and provider-defined action offers, while preserving the player's explicit access restrictions |
| Adaptation: fit drawing into VRIK's internal hand choice; the existing call cannot name both slot and destination hand | Body Slots: an explicit slot-to-physical-hand request with a HIGGS grant routed to the mod's equipment executor, with a confirmed result |
| Adaptation: use proxy type for equipment, display and holster eligibility at once; changing it for one purpose affects the others | Body Slots: independent eligibility configuration and separate content/presentation observations, distinguishing original item, proxy and displayed model |
| Maintain original/proxy identity, instance properties, counts, restoration and drop handling while other systems observe inventory changes | A shared operation contract with instance identity, reservations and one authorized inventory executor; the mod retains property-transfer policy |
| Reconcile the mod's records, VRIK's Papyrus content and the displayed model | Body Slots: committed slot content and version, plus presentation readiness tied to that version |
| Coordinate delayed tasks and later Papyrus checks that may observe an already changed proxy type | Executor contract: an immutable request, a legal execution phase and correlated accepted, committed and completed results |
| Guard against a draw immediately following holstering while the same hand remains near the shoulder | HIGGS and Body Slots: correlated input sequences and spatial offers that distinguish continuation of a gesture from a new action |
| Reconstruct hand occupancy and equipment readiness from game equipment, HIGGS grip state and VRIK flags | HIGGS: unified occupancy/availability, equipment/physical-possession/two-hand observations and confirmed outcomes; Body Slots: presentation readiness. Transfers reserve affected hands together and preserve the other hand's item |
| Restore proxy/slot associations after loading and investigate undocumented meanings or call ordering | Providers: stable keys, lifecycle events and documented observations; CommonLib: verified engine operations. The mod retains its own saved state |

The equipment mod still owns unconventional equip rules, proxy selection and preservation of instance properties. Body Slots owns definitions, placement, zones, interaction, committed content and presentation; VRIK owns the body and virtual hand poses; HIGGS owns the common hand model/use, physical possession and input arbitration. A valid equipment record alone does not prove a visible model, physical body or working collision. Shared results make those stages observable; they do not guarantee rollback after arbitrary gameplay side effects. PLANCK participates in physical hits involving NPCs and need not become a required inventory or holster executor. These are proposed services, not a completed port or guarantees of today's ABI.

## Climbing and harvesting

[VR Climbing](https://www.nexusmods.com/skyrimspecialedition/mods/170321) manages its own temporary HIGGS restrictions so a grip on a wall does not also grab an item. It changes shared HIGGS settings while climbing and restores them afterward. In effect, it creates its own way to reserve hand interactions for its gameplay.

[Immersive Harvesting](https://www.nexusmods.com/skyrimspecialedition/mods/186754) encounters the same grip input: beside a plant, the player may want to harvest rather than climb. It adds a separate workaround by intervening in the climb-start check. One consumer builds its own HIGGS interaction control, and another then has to adapt to that control.

In the proposed cooperation, HIGGS provides this shared service. It knows which hand is occupied, who owns the current action and which new actions are offered. VR Climbing requests HIGGS's standard world-support grip; Immersive Harvesting requests coordinated hand use for harvesting. Climbing does not need a private physical grip mechanism. HIGGS selects an eligible action under the user's priority, confirms its start and reports its completion. Each consumer releases only its own reservation, without restoring settings over another consumer's requests.

For example, beside a plant and a wall, the user can prefer harvesting over starting a climb. A hand already supporting a climb must be released first. An item held in the other hand must not be dropped merely to allow harvesting. The shared mechanism provides these rules to all HIGGS consumers.

VR Climbing then focuses on climbing, and Immersive Harvesting on harvest rules and results. They no longer need to recreate shared hand control or patch each other for this conflict. VRIK supplies spatial hand information and the coordinated visible pose; PLANCK participates when NPC physical interaction is involved. Player movement and fall rules remain with the climbing consumer.

Harvesting may use an object region, two coordinated hand probes, pulling direction or a swept blade crossing, depending on the action; its yield and depletion rules remain in the consumer. Climbing uses a qualified HIGGS surface query and confirmed support grip, without turning every wall into a persistent slot. The same observation provenance and lifecycle rules also support a torch's flame region or a temporary keyhole cylinder; heat, ignition and lock mathematics stay with their domain providers.

## Compatibility and ABI lifecycle

PPB observations follow the same epoch and invalidation rules as other providers. Subscriptions declare whether they carry immediate contacts or grouped reports, and reset explicitly on load/model/world replacement. Missing PPB coverage cannot be treated as an empty or free hand. Existing PPB interface1 reporting remains compatible; proposed generation-aware snapshots and removable subscriptions require a separately advertised extension.

Keep existing 001 interfaces and semantics unchanged. Negotiate additive services and capabilities separately. Missing optional providers return a defined result or use an explicitly documented legacy fallback. Old consumers that directly mutate private state cannot automatically gain the new ownership guarantees.

Body Slots owns the concept of a logical region/slot; this does not make it a compulsory startup dependency for HIGGS, VRIK, PLANCK or PPB. Their independent functions continue without it. A consumer whose purpose requires custom slots can explicitly require Body Slots, while an optional integration negotiates its service at runtime and declares what happens when it is absent. Shared SDK headers describe the ABI and allow separate compilation; they do not copy the slot implementation into every mod or create a second slot manager. A documented native fallback is a compatibility path selected instead of the shared detector, never alongside it.

HIGGS's native mouth and shoulder inventory-stash detection can use named Body Slots regions through an optional spatial-provider contract. HIGGS supplies its source observations and native configuration; the Body Slots adapter registers and evaluates the logical regions. HIGGS retains consumption/stash eligibility, hand authority and physical execution; food or backpack consumers can reuse the appropriate supported region contracts. The integration selects one active detector for each evaluation and keeps one execution path for each equivalent action. Without Body Slots, HIGGS retains its standalone behavior. An unavailable shared service follows a declared fallback; a handled outside or unavailable-source result cannot silently become a second local activation. Registration/geometry readiness remains independent of HIGGS activation, so neither service waits for the other to finish initializing. Generic action offers and the extent of consumer rebinding are negotiated capabilities, not consequences of proximity alone; delivered coverage and qualification belong in the implementation ledger.

Body Slots creates and services new slots without requiring VRIK extensions or reconstructed code. VRIK's public 001 can operate native holsters but does not become a new-slot creation API. The pose adapter must validate available game nodes, phases and coordinate spaces against the supported release; insufficient observations produce a defined refusal. Native holsters and their APIs remain available. Compatibility with the original fourteen is validated separately: do not resize old arrays or change Papyrus names or saved assignments; settings are imported or changed only through a supported available route. Full transition disables corresponding native interaction/presentation before enabling Body Slots counterparts. Without verified mappings and control of competing input, the mod does not claim automatic replacement. HIGGS's closed model likewise does not automatically govern legacy DLLs outside the new contract.

Requests, reservations and subscriptions identify their owner. One consumer cannot release another's suppression or reservation. Unloading requires stopping new callbacks and draining in-flight calls. Snapshots use copied, size-tagged records with explicit units, spaces, hand mapping and world/body generations. New boundaries use caller-owned storage and fixed-layout records.

Each feature negotiates required capabilities, usable current data and the provider's supported scope independently. DLL discovery alone cannot establish readiness. At action start it fixes one detector and one executor. Losing a provider invalidates dependent offers and requests. A fallback can serve a later action after revalidation, while the original action retains its known effects and recovery state. Passive callbacks run at a documented safe phase outside registry and external physics locks. Observation queues expose sequence gaps and invalidation explicitly. A gap cannot imply no contact, normal exit or successful completion.

The interface examples are embedded in each provider section. Lifecycle operations apply to services that allocate client-owned requests, registrations or resources. A synchronous copied-only pose reader needs no registration, lease or unsubscribe. Each negotiated service advertises only the lifecycle operations it actually supports.

**Pseudo-ABI**

```cpp
enum class PhysicalHand : uint32_t { Left, Right };
enum class PoseSource : uint32_t { TrackedController, ResolvedPalm, RenderedIK };
enum class RequestStage : uint32_t {
    Accepted, PartiallyCommitted, Committed, Completed,
    Rejected, Cancelled, OutcomeUnknown, Reconciling, Compensated
};

// Slot definitions/configuration have persistent keys and explicit save/profile scopes.
// Equipment instance keys are durable identities, not pointers or runtime handles.
```

RegisterClient establishes ownership of requests and resources; ReleaseClient ends that client lifetime.

A transaction records proven effects separately from its current recovery state. After a partial commit, a timeout, cancellation or disappearing executor cannot assert that nothing happened. The broker retains disputed resource claims and a queryable receipt until an authorized executor proves the final disposition or compensation. Repetition with the same operation key returns the recorded result. A new key cannot bypass an unsettled claim. A recovery executor must be explicitly delegated or reattached under the same durable provider identity, reconcile actual inventory/held state and report evidence before any further mutation. If recovery remains impossible, the outcome stays unknown and the dispute is exposed to the consumer and user. Lease expiry alone cannot erase it. Save/load discards runtime permissions and forces reconciliation from save-scoped records where a feature persists transactions.

**Pseudo-ABI**

```cpp
Status RegisterClient(const ClientIdentity*, ClientToken*);

Status ReleaseClient(ClientToken);

// Every provider negotiates its own version, table size and capabilities.
```

Queued operations return a request ID. Temporary effects also return a provisional lease that becomes effective on commitment and retires on rejection/cancellation. ReleaseClient and Unsubscribe prevent new dispatch and confirm in-flight drainage; callback-bearing registrations follow the same rule. Free code/context only after every relevant provider confirms drainage. Do not block physics or a callback.

**Pseudo-ABI**

```cpp
Status GetRequestResult(ClientToken, RequestId, RequestResult*);

Status CancelRequest(ClientToken, RequestId);

Status ReleaseLease(ClientToken, Lease, RequestId*);

// RequestId is scoped by provider and epoch; TransactionId correlates participants.
```

Copied records use caller buffers and report required capacity when insufficient. Hit policies and eligibility callbacks are bounded and never mutate inventory. Main-frame and physics-tick identities remain separate. Loading changes epochs; runtime lease values are not persistent slot IDs.

**Pseudo-ABI**

```cpp
Status Subscribe(ClientToken, uint64_t eventMask, EventCallback, void* context, Subscription*);

Status Unsubscribe(ClientToken, Subscription);

Status CopyEventPayload(ClientToken, SnapshotId, void* callerBuffer, uint32_t bufferBytes, uint32_t* requiredBytes);

// Event records name source/forwarder, issuer/epoch, executor, phase and thread.
// Pose/contact records name time, coordinate space, units and generations.
```

Body Slots persists definitions and user customization under stable keys where their declared scope requires it. Runtime instances, actor/equipment/world anchors, previews and bindings are rebuilt and validated after loading; readiness is reported before dependent actions resume. Session/operation regions expire with their owner and are not restored as saved slots. Content providers retain their own saved gameplay state under the same durable keys. Provider removal, region unavailability and explicit deletion of saved customization have different meanings and must not silently erase each other's data.

The revised contract is checked with the same acceptance scenarios for every provider and consumer.

| Scenario | Required result |
|---|---|
| Harvest > native grab > pocket | Unavailable harvest leaves native first, regardless of registration order. Up returns to the original selected action once. |
| Item removed, receipt lost | No second removal or invented rollback. Proven effects and disputed resources remain visible until factual reconciliation. |
| Add or release a second hand | Rejected acquisition preserves the first grip. Release drains only dependent effects. |
| Two sword contacts in one physics step | Only an exact event witness permits anatomical attribution. Unknown attribution preserves the original hit. |
| Optional provider absent, old, unavailable or lost during execution | Unrelated features continue. One detector/executor is fixed per action. Provider recovery never replays effects. |

Every feature also tests supported and unsupported targets, stale/replaced sources, duplicate requests, callback removal during dispatch and bounded queue exhaustion. These are contract criteria. The implementation ledger records which have passed for a pinned source version and which still require engine observations. The illustrative consumer list supplies test cases and does not establish compatibility with every future Skyrim VR mod.

## CommonLib as the second level of synergy

The synergy has two levels. The first coordinates HIGGS, VRIK, PLANCK, PPB and the slot service through their APIs/ABIs: each mod provides its own observations and actions. The second establishes a common technical foundation in CommonLib, moving engine address handling and repeated code out of individual mods into the library.

This includes function/data address definitions and resolution, typed object access, references and locks, coordinate transforms and units, shape-distance/intersection math, scene and Havok operations, ray/sweep queries and character controller access. Mods use common CommonLib methods instead of maintaining their own addresses and similar low-level implementations. A fix to an operation is maintained in one place and becomes available to consumers when they adopt the corresponding library version.

Start with the capabilities of the current CommonLib build. When an operation is missing, extend the library with a verified method: establish its signature, data layout, address for the supported game version and call behavior. A common operation should serve both the cooperating frameworks and external mods, with an interface that encapsulates engine access details.

On this foundation, HIGGS manages all hand states and ways of using hands, VRIK manages the body and virtual hand poses, Body Slots manages all slot mechanics, PLANCK manages its NPC ragdoll bodies, and PPB supplies anatomical geometry and touch evidence. CommonLib provides their shared technical tools; decisions about pouch contents, climbing, harvesting and hits belong to the owners of those mechanics. The two levels complement each other: common engine code in the library and coordinated gameplay interaction through the mods' APIs/ABIs.

For PPB, candidate shared technical work includes capsule-distance math, coordinate conversion and verified shape/scene access. Reuse existing CommonLib methods where qualified; missing safe helpers are second-stage library proposals. Region registration, spatial sessions/dwell, user priorities, anatomical fitting, contact meaning and per-provider mutation authority remain with the services described above; crime, food and harvest rules remain with their consumers. This revision implements no CommonLib change.

## User priority and mod order

The user should control which eligible action takes precedence. A grip beside a plant and a wall can mean harvesting, grabbing an item or starting a climb. The broker should expose priority for that context and explain the decision.

Mod order can be one supported way to express that choice. One proposed route is compatibility-rule files sharing a path, so the user chooses the winning file through MO2/Vortex file priority. An explicitly enabled per-action setting can override that order. An optional adapter could import a chosen mod ordering into the priority list. DLL-only mods have no ESP whose position establishes this arbitration; the connection between user ordering and action selection must be implemented rather than assumed.

For eligible new actions, the proposed resolution is an explicit user assignment for the context, then the selected user priority list, then a stable action ID for ties. A visible default profile handles missing custom rules. A consumer or callback registration timestamp cannot silently override the user's choice.

The broker also issues provisional hold/multi-tap reservations under that priority. They have a bounded decision window and declared preemption rules. A recognizer cannot reserve first and make the preferred action ineligible on its own authority. Each input sequence pins a priority-rule version. Configuration changes apply to the next sequence or explicitly cancel the current one under a user-selected policy.

Priority applies to **eligible new actions**. Existing possession and committed hand reservations are checked first: harvesting must not steal a hand already supporting a climb. Release or interruption is an explicit action. Reordering priority cannot duplicate a committed transfer. The user controls gameplay choice while ownership, object validity and exactly-once commitment remain enforced.
