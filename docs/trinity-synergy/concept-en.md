# Cooperation between VRIK HIGGS and PLANCK

Russian: [Русская версия](concept-ru.md).

This proposal describes a shared architecture for the three frameworks and their consumers. HIGGS owns all hand states and ways of using hands, Body Slots owns all slot mechanics, VRIK owns the body and virtual hand poses, and PLANCK owns NPC physics. Each exposes the observations and requests the others need. The complete API is not implemented or agreed with the authors; the [implementation ledger](implementation-status.md) separates delivered code from illustrative interfaces. This concept revision of 28 October 2026 does not resume implementation.

The intended benefit is concrete: a mod author can ask for a body zone, a physical handover or an NPC attachment without recreating another framework's internals. A single interaction has identifiable participants and a confirmed result.

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
| PLANCK | NPC physical animation and ragdoll lifecycle; its physical-hit interpretation and commitment | Actor/body/bone metadata, validated attachment endpoints, candidate/committed hit data, scoped interaction policies and lifecycle events |
| Body Slots (working name) | All slot mechanics: definitions/instances, binding, geometry, detection, activation, presentation, content policies and saved customization | Creation/configuration API, definition packages, equipment slot sets, zone events, action offers, addressed behavior requests and observable results |
| External mod | Its gameplay feature and content policy | Pouch selection, unconventional equipment/proxies, handling gestures, weight response, penetration/extraction or parry rules expressed through shared contracts |
| CommonLib | Engine addresses and types, reusable technical operations and verified wrappers | A common implementation of engine access, geometry, scene and physics operations for the three frameworks and external mods |

HIGGS supplies the common input stream, hand model and control of every hand-use mechanism. Providers may recognize gameplay gestures and propose actions, but use HIGGS's closed set of shared operations. Body Slots recognizes interaction with its slots; VRIK supplies pose without selecting slot actions. A recognizer does not gain permission to create a second physical actuator or bypass HIGGS arbitration. Forwarding an action result does not transfer pouch policy or damage rules to HIGGS; the result identifies the actual executor.

## HIGGS input possession and physical requests

HIGGS is the sole authority for the shared hand-state model and coordination of every hand-use mechanism. It observes game equipment through the engine and virtual hand pose through VRIK, and manages physical hands, input, grips, reservations and haptics. Body Slots and other consumers obtain state and grants from HIGGS without a parallel classifier. Locomotion, harvest policy, slot contents and damage remain with their gameplay owners; HIGGS routes authorized operations to the assigned executor and reports the outcome.

### Input and action selection

Recognizers use one input stream and submit candidates before default actions execute. InputBinding declares context, action, recognition deadline and deferred-default behavior. ActionCandidate names the input sequence, required resources and registered executor. HIGGS selects an eligible action using the user's priority and reports the decision. Body Slots supplies slot spatial context; other recognizers use the same broker.

**Pseudo-ABI — illustrative example, not a final interface.**

```cpp
namespace HIGGS {

Status CopyInput(ClientToken, PhysicalHand, InputObservation* outInput);

Status RegisterRecognizer(ClientToken, const InputBinding*, RecognizerCallback, void* userContext, Lease* outRegistration);

Status SubmitActionCandidate(ClientToken, const ActionCandidate*, RequestId* outRequest);

// Events: InputChanged.

}
```

The broker exposes the effective priority and the reason for its decision. A consumer can inspect these without acquiring the right to consume the same input again. Haptic requests enter one coordinated schedule.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyEffectiveInputPriority(ClientToken, const InputContext*, InputPriority*);

Status CopyInputDecision(ClientToken, uint64_t sequenceId, InputDecision*);

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

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyHandState(ClientToken, PhysicalHand, HandState* outState);

Status CopyReleaseObservation(ClientToken, uint64_t releaseId, ReleaseObservation* outRelease);

// Events: InteractionChanged.

}
```

Motion samples distinguish controller movement in VR tracking space from item movement in world space, and identify the tracking-to-world transform version. Locomotion, teleportation and recentering may change that transform without replacing the world or body. Contact snapshots carry source, object, point, normal, motion and body generations; contact with a plant or wall is distinct from an NPC hit.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status CopyMotionSamples(ClientToken, PhysicalHand, PoseSource, uint32_t space, MotionSample* outSamples, uint32_t capacity, uint32_t* outCount);

Status CopyContactSnapshot(ClientToken, const ContactQuery*, ContactSnapshot*);

// Events: ContactChanged.

}
```

### Shared hand-use operations

The HIGGS ABI version fixes the catalogue of shared operations: object grab, release, item transfer, secondary weapon support, world-support grip, game-equipment use, consumption and coordinated hand-pose goals. Gameplay scenarios compose those operations. A new mod cannot register an arbitrary physical control kind: a new common primitive requires an explicit HIGGS version/capability extension. Drive parameters, constraints and suppression configure supported operations rather than creating a second grip mechanism.

A climbing mod requests HIGGS's standard world-support grip and moves the player after confirmation. HIGGS owns physical support, hand use and release. Multi-hand operations are coordinated together. The engine or an equipment mod may execute equipment changes, but the shared hand model and operation grant belong to HIGGS.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RequestHandUse(ClientToken, const HandUseRequest*, Lease* outUse, RequestId* outRequest);

Status RequestEndHandUse(ClientToken, Lease use, RequestId* outRequest);

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

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RequestGripChange(ClientToken, const GripChange*, RequestId* outRequest);

Status RequestHandGoal(ClientToken, const HandGoal*, Lease* outGoal, RequestId* outRequest);

Status RequestDrivePolicy(ClientToken, const DrivePolicy*, Lease* outPolicy, RequestId* outRequest);

Status RequestConstraint(ClientToken, const ConstraintRequest*, Lease* outConstraint, RequestId* outRequest);

Status RequestSuppression(ClientToken, const SuppressionRequest*, Lease* outLease, RequestId* outRequest);

// Events: ConstraintChanged.

}
```

### Domain executors

Consumers register an execution route; recognition and eligibility callbacks do not mutate inventory. A result is accepted only from the assigned executor, with matching request provider and epoch. Duplicate reports are ignored and a committed stage cannot regress. HIGGS forwards the receipt while preserving that executor's authority over the operation.

**Pseudo-ABI**

```cpp
namespace HIGGS {

Status RegisterActionExecutor(ClientToken, const ActionExecutorInfo*, ActionExecutorCallback, void* userContext, ExecutorId* outExecutor, Lease* outRegistration);

Status ReportExecutionResult(ClientToken, const ExecutionResult*);

}
```

## VRIK body and hand pose

### Body and hand pose

VRIK owns the virtual body, calibration and virtual hand poses. **IK means inverse kinematics:** head and hand positions guide calculation of the remaining skeletal pose, including shoulders and elbows. HIGGS manages physical hand use and supplies authorized grip goals; VRIK coordinates the visible pose with them. Zones, slots, their content, activation and presentation belong to Body Slots. Player locomotion remains with its gameplay owner.

Tracked controller, HIGGS resolved palm/grip and VRIK virtual hand are distinct observations with time, validity, space and generation. VRIK supplies virtual hand and body-anchor poses; HIGGS determines hand state and availability, and Body Slots tests the selected probe against its geometry. Hand-use goals go through HIGGS; VRIK remains the sole skeletal pose solver. Final IK must not feed back as the same cycle's physical target.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status CopyHandPose(ClientToken, PhysicalHand, HandPose* outPose);

Status CopyBodyAnchor(ClientToken, const BodyAnchorQuery*, BodyAnchorSnapshot* outAnchor);

// Events: PoseAvailable, BodyAnchorInvalidated.

}
```

This is a proposed observation contract, not a claim that released VRIK provides these methods. The inspected public 001 interface has no general GetHandPose method. An independent Body Slots adapter can observe available game nodes updated by VRIK, validating phase, coordinates, calibration and freshness; insufficient data makes the observation unavailable. New slots are designed without requiring VRIK changes or reconstructed code. Qualification of that adapter against a particular VRIK release remains separate work.

## PLANCK NPC physics and hits

### Actor bodies and attachment points

PLANCK owns NPC physical animation, ragdoll transitions and body lifetime. It supplies actor/bone/body metadata and validated attachment endpoints. A penetration mod owns wound and extraction rules; PLANCK validates the target body and HIGGS applies the weapon constraint.

**Pseudo-ABI**

```cpp
namespace PLANCK {

Status CopyActorPhysics(ClientToken, ActorHandle, ActorPhysics* outState);

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

### Physical hits

PLANCK owns interpretation and commitment of its physical hits. A bounded hit-policy callback runs before the relevant commitment without writing live engine objects; HitCommitted reports the outcome. Observing HitCandidateObserved alone is not a writable damage hook. Policy conflicts follow declared rules and user-visible priorities where alternatives are selectable.

**Pseudo-ABI**

```cpp
namespace PLANCK {

Status RegisterHitPolicy(ClientToken, uint64_t scope, HitPolicyCallback, void* userContext, Lease* outRegistration);

Status CopyHitSnapshot(ClientToken, uint64_t hitId, HitSnapshot* outSnapshot);

// Events: HitCandidateObserved, HitCommitted.

}
```

## Body Slots all slot mechanics

A slot is an identified, configurable region on or near the player's body with a bound interaction behavior. A hand entering it changes spatial context; an authorized input can invoke an offered action. The region, visible bag and inventory container are separate entities: one bag may supply several slots, and a slot need not own any stock.

**Body Slots** provides an extensible slot system without VRIK's architectural fourteen-holster or weapon-type restrictions. It owns all slot mechanics: creation, body/equipment binding, geometry, hand entry/exit, hover, action offers, activation, presentation and saved customization. VRIK supplies the body and virtual hand poses; HIGGS supplies hand state and coordinates every way of using a hand. Configuration and behavior providers define slot content types and gameplay responses, executed through the shared operations of the appropriate services. Practical resource limits remain explicit implementation capabilities. Offline authoring and the creation API are two ways to supply slots, rather than offline editing being the purpose of the service.

VRIK's original fourteen holsters remain an existing system. New Body Slots slots do not require replacing them. In a full transition mode, native holsters are disabled and replaced by fourteen Body Slots counterparts with matching accepted items and positions, plus mod-defined slots. Settings and assignment mappings require separate validation; only one interaction/presentation path for each equivalent slot is active. Coexistence explicitly coordinates competing holsters through available controls rather than assuming HIGGS automatically governs unchanged VRIK. Full handover remains a target compatibility mode, not a prerequisite for new slots or a delivered prototype capability.

### Visual editor before launching the game

The primary workflow is **offline visual editor → editable project and versioned slot package → Body Slots binding, zone tests, interaction and presentation; body/hand pose from VRIK, hand state and input arbitration from HIGGS**. The editor loads a reference body/skeleton or equipment model. The author selects a named anchor, places and rotates a slot, edits its independent access region and declares scale behavior, physical-hand access, slot kind, allowed contents and optional visuals. For a wearable potion bag, the author can place the access region at its opening before launching Skyrim.

The editable project retains the authoring data; the export is a versioned declarative package that the runtime can validate and load. Neither Skyrim nor VRIK is required to author, reopen or export definitions. A mod may ship such a package without calling a live creation API. The format records stable provider/slot keys, schema and reference-rig versions, anchor names, local transforms, geometry/probe declarations, units, axes, scale conventions, policy bindings and asset references. Engine pointers and save-specific handles never enter the package; live eligibility callbacks cannot run in the editor.

The selected authoring tool is [Morphbench](https://github.com/Vhodnoylogin/morphbench), with a separate Slots mode that reuses its model and skeleton viewer. The mode edits node-relative placement and exports Body Slots definitions; it does not become another runtime slot manager. Skeleton node anchors and physics bodies remain distinct. A missing node cannot be represented as a valid preview at an invented transform. Delivered functionality and remaining authoring requirements are recorded in the [implementation ledger](implementation-status.md).

Offline placement provides a default, not proof of reachability on every character. The package declares reference-to-runtime mapping and explicit conversion/rebinding rules. Body Slots applies them to observed VRIK body proportions/calibration and equipment movement. A missing anchor makes the slot unavailable instead of silently selecting another bone. Player overrides remain separate from the author's package; runtime ergonomics still need game checks.

### Slot definitions instances and configuration

Body Slots owns creation and the definition/instance registry: stable keys, named anchors and local placement, independent access geometry, slot kind, generic allowed-content filters and customization persistence. Definitions/profile defaults, save-scoped instances and player overrides remain separate. Re-registration preserves overrides; runtime IDs and reservations are recreated after loading. Creating a runtime instance is distinct from authoring its reusable definition.

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

An equipment mod supplies records, assets and a declared slot set authored on its reference model. Body Slots creates and persists the instances and follows their anchors on the actual wearable model. The key combines the provider definition, particular equipment instance and slot within it, so identical bags keep separate assignments. The equipment provider validates durable save-scoped instance keys and declares stack split/merge, cloning and ownership-transfer rules. Rebinding restores one set; missing equipment, providers or unresolved identity leave it unavailable without attaching it to another item. Body Slots saves placement/visual overrides; content providers save their own rules under the same key.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status BindEquipmentSlotSet(ClientToken, const EquipmentSlotBinding*, SlotSetId* outSet, RequestId* outRequest);

}
```

### Zones

A zone is an anchored spatial region with an explicit probe. Slot access normally uses the VRIK virtual hand; the HIGGS resolved palm, controller, item anchor and weapon tip are separate sources. Body Slots computes world placement, tests containment and reports entry, exit and hover. Geometry remains observable independently of slot occupancy, visibility or reservation; zone transitions and available actions are separate events. QueryZones also serves clients subscribing after entry.

Body anchors are primary. An optional world-reference anchor could reuse the contract around a plant or wall; target discovery, harvesting and climbability rules remain with consumers using CommonLib queries. Losing an anchor invalidates the zone.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status CreateZone(ClientToken, const ZoneDefinition*, ZoneId* outZone);

Status UpdateZone(ClientToken, ZoneId, const ZoneDefinition*, RequestId* outRequest);

Status DestroyZone(ClientToken, ZoneId, RequestId* outRequest);

Status QueryZones(ClientToken, const ZoneProbe*, ZoneSnapshot* outZones, uint32_t capacity, uint32_t* outCount);

// Events: ZoneEntered, ZoneExited, ZoneInvalidated.

}
```

### Runtime slot binding

Body Slots owns both definitions and all runtime slot mechanics. It maps named anchors onto the actual body or equipment model using VRIK observations and game skeleton data, computes world placement and evaluates hand access. Body Slots reports binding availability, interaction and presentation readiness; persistent geometry comes from its registry. The pose adapter supplies observations without owning zones or slots.

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

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status CopyActionOffers(ClientToken, const SlotRequest*, ActionOffer* outOffers, uint32_t capacity, uint32_t* outCount);

// Reserves slot content/access only; hand and input grants belong to HIGGS.
Status ReserveSlot(ClientToken, SlotId, const SlotRequest*, Lease* outReservation);

Status RequestSlotAction(ClientToken, const SlotRequest*, RequestId* outRequest);

Status CopySlotContent(ClientToken, SlotId, SlotContentSnapshot* outContent);

Status UpdateSlotContentFromCommit(ClientToken, SlotId, const SlotContentCommit*, RequestId* outRequest);

// Events: ActionOffersChanged, SlotContentChanged.

}
```

Ineligibility before selection can allow another candidate. Once accepted, commitment, completion, failure and cancellation remain distinct; a failure after execution starts must not automatically dispatch another action on the same input. For overlapping zones, Body Slots offers candidates with explicit slot priorities; HIGGS arbitrates them alongside other hand actions under the user's rules.

### Registry pose adapters and executors

The definition API and body-observation adapters are separate. Body Slots receives hand/body-anchor poses from an available VRIK adapter or another compatible pose provider; HIGGS always supplies hand state and permissions. An adapter does not give VRIK ownership of slots. Body Slots computes geometry and manages transitions, activation, bindings and presentation. Frame work uses an immutable configuration revision; queued actions revalidate it, body/anchor generations and the HIGGS grant before commitment.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status CopyDefinitionChanges(ClientToken, const DefinitionCursor*, SlotDefinitionSnapshot* outRecords, uint32_t capacity, uint32_t* outCount);

Status CopySlotRuntime(ClientToken, SlotId, SlotRuntimeSnapshot* outRuntime);

// Events: DefinitionAvailabilityChanged.

}
```

Each state has one authoritative writer: HIGGS for hand state/use, VRIK for body/virtual-hand pose, Body Slots for definitions, configuration, geometry, binding, interaction and the committed-content mirror/presentation, and the content executor for gameplay assignments/inventory mutation. Configuration, content and presentation versions and body/anchor generations are distinct. Drag, MCM and API edits use expected revisions; callbacks run at declared phases outside registry locks. Provider removal stops new calls and drains in-flight callbacks before releasing code/context. Changes are queued at declared boundaries.

## Input and action sequence

1. HIGGS supplies grip/trigger samples and edges with physical hand, analog/touch distinction and an input sequence ID. VRIK supplies coherent virtual-hand pose; Body Slots tests zones and offers slot actions. Only HIGGS supplies hand state and availability.
2. Frameworks and consumers propose actions using those observations. A hold or multi-tap recognizer reserves the sequence with a decision deadline.
3. The broker selects one consuming action under declared contexts, bindings and user-selected priority. Observers remain able to read the input, but cannot independently act on it.
4. The domain executor revalidates and performs the operation at its legal phase.
5. Results distinguish accepted, committed, completed, rejected and cancelled. They carry request/transaction IDs, the request issuer and its epoch, and the selected executor; an event forwarder remains a separate identity.

Acceptance is not successful completion. Cancellation and deferred default input have declared behavior; synthetic gameplay requests never masquerade as physical button edges. Menus, missing tracking, load and provider shutdown invalidate pending work safely.

## Shared physical operation

Each owned hand/weapon body has one final HIGGS actuator. Consumers submit supported goals and constraints through its common contract without registering a private physical driver. Contact constraints, weight/impedance objectives and an accepted embedded attachment contribute to one physical recipe. They cannot be resolved by allowing each callback to overwrite the pose in turn. Havok still solves world contacts; PLANCK still controls NPC physical animation. A penetration provider owns wound and extraction policy while PLANCK validates the NPC attachment endpoint.

A heavy lodged weapon touching a wall is a representative combined case. The contract reports saturation, break or rejection rather than silently teleporting the hand or losing the attachment. Damage and parry policy consume committed physical observations and enter the documented hit decision path once.

A climbing support point belongs to the climbing action and reserves the named hand. HIGGS performs the standard world-support grip, owns the physical hold and suppresses incompatible new actions in scope; VRIK renders the coordinated pose. The climbing consumer retains the player locomotion controller. Two-hand actions validate their footprint together and do not drop another hand's item merely to simplify a new interaction. Climbing starts only after its required hand reservation, physical support and restrictions become effective; acceptance alone is insufficient. Acquisition failure unwinds provisional resources before locomotion starts. Switching actions waits for confirmed physical hand release.

## Mods used to assess ABI expressiveness

These published mods were used as concrete requirements for ABI expressiveness. They remain external consumers; their gameplay does not become framework logic.

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

## Compatibility and ABI lifecycle

Keep existing 001 interfaces and semantics unchanged. Negotiate additive services and capabilities separately. Missing optional providers return a defined result or use an explicitly documented legacy fallback. Old consumers that directly mutate private state cannot automatically gain the new ownership guarantees.

Body Slots creates and services new slots without requiring VRIK extensions or reconstructed code. VRIK's public 001 can operate native holsters but does not become a new-slot creation API. The pose adapter must validate available game nodes, phases and coordinate spaces against the supported release; insufficient observations produce a defined refusal. Native holsters and their APIs remain available. Compatibility with the original fourteen is validated separately: do not resize old arrays or change Papyrus names or saved assignments; settings are imported or changed only through a supported available route. Full transition disables corresponding native interaction/presentation before enabling Body Slots counterparts. Without verified mappings and control of competing input, the mod does not claim automatic replacement. HIGGS's closed model likewise does not automatically govern legacy DLLs outside the new contract.

Requests, reservations and subscriptions identify their owner. One consumer cannot release another's suppression or reservation. Unloading requires stopping new callbacks and draining in-flight calls. Snapshots use copied, size-tagged records with explicit units, spaces, hand mapping and world/body generations; new boundaries do not transfer STL objects or ownership of allocated memory.

The illustrative ABI is embedded in each provider section. Supporting records are abbreviated; numeric values, layouts, calling convention and transport are not final. Each provider exposes the same lifecycle operations:

**Pseudo-ABI**

```cpp
enum class PhysicalHand : uint32_t { Left, Right };
enum class PoseSource : uint32_t { TrackedController, ResolvedPalm, RenderedIK };
enum class RequestStage : uint32_t {
    Accepted, Committed, Completed, Rejected, Cancelled
};

// Slot definitions/configuration have persistent keys and explicit save/profile scopes.
// Equipment instance keys are durable identities, not pointers or runtime handles.
```

RegisterClient establishes ownership of requests and resources; ReleaseClient ends that client lifetime.

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

Body Slots persists slot definitions and user customization under stable keys. Runtime instances, equipment anchors, previews and bindings are rebuilt and validated after loading; readiness is reported before dependent actions resume. Content providers retain their own saved gameplay state under the same keys. Provider removal, slot unavailability and explicit deletion of saved customization have different meanings and must not silently erase each other's data.

## CommonLib as the second level of synergy

The synergy has two levels. The first coordinates HIGGS, VRIK, PLANCK and the slot service through their APIs/ABIs: each mod provides its own observations and actions. The second establishes a common technical foundation in CommonLib, moving engine address handling and repeated code out of individual mods into the library.

This includes function/data address definitions and resolution, typed object access, references and locks, coordinate transforms, scene and Havok operations, ray queries and character controller access. Mods use common CommonLib methods instead of maintaining their own addresses and similar low-level implementations. A fix to an operation is maintained in one place and becomes available to consumers when they adopt the corresponding library version.

Start with the capabilities of the current CommonLib build. When an operation is missing, extend the library with a verified method: establish its signature, data layout, address for the supported game version and call behavior. A common operation should serve both the three frameworks and external mods, with an interface that encapsulates engine access details.

On this foundation, HIGGS manages all hand states and ways of using hands, VRIK manages the body and virtual hand poses, Body Slots manages all slot mechanics, and PLANCK manages NPC physics. CommonLib provides their shared technical tools; decisions about pouch contents, climbing, harvesting and hits belong to the owners of those mechanics. The two levels complement each other: common engine code in the library and coordinated gameplay interaction through the mods' APIs/ABIs.

## User priority and mod order

The user should control which eligible action takes precedence. A grip beside a plant and a wall can mean harvesting, grabbing an item or starting a climb. The broker should expose priority for that context and explain the decision.

Mod order can be one supported way to express that choice. One proposed route is compatibility-rule files sharing a path, so the user chooses the winning file through MO2/Vortex file priority. An explicitly enabled per-action setting can override that order. An optional adapter could import a chosen mod ordering into the priority list. DLL-only mods have no ESP whose position establishes this arbitration; the connection between user ordering and action selection must be implemented rather than assumed.

For eligible new actions, the proposed resolution is an explicit user assignment for the context, then the selected user priority list, then a stable action ID for ties. A visible default profile handles missing custom rules. A consumer or callback registration timestamp cannot silently override the user's choice.

The broker also issues provisional hold/multi-tap reservations under that priority. They have a bounded decision window and declared preemption rules. A recognizer cannot reserve first and make the preferred action ineligible on its own authority. Each input sequence pins a priority-rule version. Configuration changes apply to the next sequence or explicitly cancel the current one under a user-selected policy.

Priority applies to **eligible new actions**. Existing possession and committed hand reservations are checked first: harvesting must not steal a hand already supporting a climb. Release or interruption is an explicit action. Reordering priority cannot duplicate a committed transfer. The user controls gameplay choice while ownership, object validity and exactly-once commitment remain enforced.
