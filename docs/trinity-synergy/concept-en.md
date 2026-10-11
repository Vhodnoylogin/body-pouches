# Cooperation between VRIK HIGGS and PLANCK

Russian: [Русская версия](concept-ru.md).

This proposal describes how the three frameworks could provide a coherent base for Skyrim VR mods. Each framework owns its existing domain and exposes the observations and requests that other mods need. It is a direction for discussion, not an agreed design or a promise of an available API. A separate slot service extends this foundation with definitions authored outside the game and interpreted by VRIK at runtime.

The intended benefit is concrete: a mod author can ask for a body zone, a physical handover or an NPC attachment without recreating another framework's internals. A single interaction has identifiable participants and a confirmed result.

## Contents

- [Responsibilities](#responsibilities)
- [HIGGS input possession and physical requests](#higgs-input-possession-and-physical-requests)
  - [Input and action selection](#input-and-action-selection)
  - [Hand state and coherent observations](#hand-state-and-coherent-observations)
  - [Grabbing release and transfer](#grabbing-release-and-transfer)
  - [Physical constraints](#physical-constraints)
  - [Domain executors](#domain-executors)
- [VRIK body hands zones and slots](#vrik-body-hands-zones-and-slots)
  - [Body and hand pose](#body-and-hand-pose)
  - [Zones](#zones)
  - [Runtime slot binding](#runtime-slot-binding)
  - [Slot visuals](#slot-visuals)
  - [Content providers](#content-providers)
- [PLANCK NPC physics and hits](#planck-npc-physics-and-hits)
  - [Actor bodies and attachment points](#actor-bodies-and-attachment-points)
  - [Physical hits](#physical-hits)
- [Separate slot mod and offline authoring](#separate-slot-mod-and-offline-authoring)
  - [Visual editor before launching the game](#visual-editor-before-launching-the-game)
  - [Slot definitions instances and configuration](#slot-definitions-instances-and-configuration)
  - [Slots supplied by equipment](#slots-supplied-by-equipment)
  - [Registry and VRIK adapter](#registry-and-vrik-adapter)
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
| HIGGS | Grip/trigger input brokerage, haptic scheduling, physical hands/weapons, grabbing and physical possession | Input observations and reservations, physical grip state, explicit grab/release/transfer requests, supported grip/drive/constraint integration and action receipts |
| VRIK | Player body and final skeletal pose; runtime anchor mapping, spatial hand interpretation, zone tests, slot interaction and presentation | Coherent pose/zone snapshots, runtime slot binding, zone transitions, eligible actions, content previews, named-hand requests and scoped reservations |
| PLANCK | NPC physical animation and ragdoll lifecycle; its physical-hit interpretation and commitment | Actor/body/bone metadata, validated attachment endpoints, candidate/committed hit data, scoped interaction policies and lifecycle events |
| Body Slots (working name) | Offline slot authoring and definition/instance registry; stable keys, local placement/geometry, generic content rules and saved customization | Versioned declarative packages, slot creation/configuration API, equipment slot sets and copied definitions for the VRIK runtime adapter |
| External mod | Its gameplay feature and content policy | Pouch selection, unconventional equipment/proxies, handling gestures, weight response, penetration/extraction or parry rules, expressed through provider contracts |
| CommonLib | Engine addresses and types, reusable technical operations and verified wrappers | A common implementation of engine access, geometry, scene and physics operations for the three frameworks and external mods |

Input brokerage does not move every gesture into HIGGS. VRIK can recognize spatial gestures and a handling mod can recognize taps/holds. They use a common input stream and submit candidates; the broker resolves ownership before defaults act. Equally, forwarding an action result does not make HIGGS the authority for damage, pouch contents or every inventory operation. The result identifies its executor.

## HIGGS input possession and physical requests

HIGGS supplies grip/trigger input and haptic scheduling, and owns physical hands, weapons and possession. It reports both the input and the confirmed outcome of the selected action. Locomotion, harvest rules, slot contents and damage policy remain with their domain owners; forwarded results identify the actual executor.

### Input and action selection

Recognizers use one input stream and submit candidates before default actions execute. InputBinding declares context, action, recognition deadline and deferred-default behavior. ActionCandidate names the input sequence, required resources and registered executor. HIGGS selects an eligible action using the user's priority and reports the decision; spatial gesture recognition can remain in VRIK.

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

CopyHandState names physical left/right separately from dominant/off hand. An input or release observation identifies the sample used for its spatial decision: VRIK tests the corresponding probe and zone rather than combining an earlier drop with an arbitrary current pose. A post-update callback alone cannot guarantee this. Regrabbing, consuming, loading or losing tracking invalidates pending offers before execution.

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

## VRIK body hands zones and slots

### Body and hand pose

VRIK owns the virtual body and spatial interpretation of the hands. **IK means inverse kinematics:** head and hand positions guide calculation of the remaining skeletal pose, including shoulders and elbows. HIGGS supplies the physically resolved palm/item state; VRIK coordinates the visible hand with it. Player locomotion remains with its gameplay owner.

Tracked controller, resolved palm/grip and rendered IK hand are distinct observations. Samples name time, validity, space and generation, including any previous-frame input. Final IK must not feed back as the same cycle's physical target. CopyHandPose selects the observation; RequestHandGoal expresses a pose objective, without permission to write bones directly.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status CopyHandPose(ClientToken, PhysicalHand, PoseSource, HandPose* outPose);

Status RequestHandGoal(ClientToken, const HandGoal*, Lease* outGoal, RequestId* outRequest);

// Events: PoseAvailable.

}
```

### Zones

A zone is an anchored spatial region with an explicit probe: normally the resolved palm, optionally the controller, an item anchor or weapon tip. Geometry remains observable independently of slot occupancy, visibility or reservation; zone transitions and available actions are separate events. QueryZones also serves clients subscribing after entry.

Body anchors are primary. An optional world-reference anchor could reuse the contract around a plant or wall; target discovery, harvesting and climbability rules remain with consumers using CommonLib queries. Losing an anchor invalidates the zone.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status CreateZone(ClientToken, const ZoneDefinition*, ZoneId* outZone);

Status UpdateZone(ClientToken, ZoneId, const ZoneDefinition*, RequestId* outRequest);

Status DestroyZone(ClientToken, ZoneId, RequestId* outRequest);

Status QueryZones(ClientToken, const ZoneProbe*, ZoneSnapshot* outZones, uint32_t capacity, uint32_t* outCount);

// Events: ZoneEntered, ZoneExited, ZoneInvalidated.

}
```

### Runtime slot binding

The separate slot mod owns definitions and saved customization. VRIK resolves their named anchors against the actual calibrated body or equipment model, computes world placement and evaluates hand access. Its adapter reports availability, interaction and presentation; it does not become another writer of the saved definition. Creating temporary VRIK zones remains available for runtime interactions; persistent slot geometry comes from the slot registry.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status BindSlotRuntime(ClientToken, const SlotRuntimeBinding*, Lease* outBinding, RequestId* outRequest);

Status CopySlotBinding(ClientToken, SlotId, SlotRuntimeSnapshot* outBinding);

// Events: SlotInstanceAvailable, SlotInstanceUnavailable.

}
```

VRIK recognizes in-game drag gestures using HIGGS input and submits an expected-revision customization request to the slot mod. It can display a provisional drag, but a rejected edit restores the committed placement. Equipping a bag makes its registered slots available only after model binding; unequipping or model loss stops new offers and cancels uncommitted work. Already delivered items retain their disposition, and the content provider handles unequip policy through the authorized executor.

### Slot visuals

Body Slots stores editable bag/holder visual configuration; VRIK renders it separately from item previews. The binding identifies whether equipment or VRIK renders the bag, preventing duplication. Model-defined access/grip regions are explicit; model bounds do not automatically become activation geometry. Visual changes version their anchors and report readiness/failure; preview readiness identifies the displayed content version.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status RequestItemPreview(ClientToken, SlotId, const ItemPreview*, RequestId* outRequest);

Status CopySlotPresentation(ClientToken, SlotId, SlotPresentationSnapshot* outPresentation);

// Events: PresentationReady, PresentationFailed.

}
```

### Content providers

Body Slots supplies generic content filters and policy bindings. BindSlotContent attaches the content executor and its action rights to a VRIK runtime slot; it references the registry policy rather than creating a second filter definition. BindNativeSlot is an optional compatibility bridge for native geometry and player placement without a fictitious weapon category. Releasing callback-bearing bindings drains their calls; new services preserve the existing holster interface.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status BindNativeSlot(ClientToken, uint32_t nativeSlot, const SlotContentBinding*, SlotId* outSlot, Lease* outBinding, RequestId* outRequest);

Status BindSlotContent(ClientToken, SlotId, const SlotContentBinding*, Lease* outBinding, RequestId* outRequest);

}
```

A slot can represent stored content or a selection from shared inventory. A preview creates no extra stock. Two hands requesting the last matching item reserve the same instance or quantity through the content executor. Operations name the physical hand and all required resources; original item, equipment proxy and displayed model retain separate identities. The proxy provider preserves enchantment, charge, tempering and ownership.

Eligible actions name a registered executor. VRIK publishes committed occupancy/content from that executor's receipt, validating expected slot version; previews name that version. An unrelated or stale receipt cannot update the slot. HIGGS coordinates physical possession and transfer results.

**Pseudo-ABI**

```cpp
namespace VRIK {

Status CopyActionOffers(ClientToken, const SlotRequest*, ActionOffer* outOffers, uint32_t capacity, uint32_t* outCount);

Status ReserveSlot(ClientToken, SlotId, const SlotRequest*, Lease* outReservation);

Status RequestSlotAction(ClientToken, const SlotRequest*, RequestId* outRequest);

Status CopySlotContent(ClientToken, SlotId, SlotContentSnapshot* outContent);

Status UpdateSlotContentFromCommit(ClientToken, SlotId, const SlotContentCommit*, RequestId* outRequest);

// Events: ActionOffersChanged, SlotContentChanged.

}
```

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

## Separate slot mod and offline authoring

Slot creation becomes a separate service, with **Body Slots** as a working name. Its main purpose is to let a mod author create new slots outside the game. Keeping VRIK's native 14 holsters is an optional compatibility route. The three frameworks keep their domains; this service supplies the declarative slot data that VRIK uses at runtime.

### Visual editor before launching the game

The primary workflow is **offline visual editor → editable project and versioned slot package → separate slot runtime/API → VRIK runtime binding, hand detection, interactions and presentation**. The editor loads a reference body/skeleton or equipment model. The author selects a named anchor, places and rotates a slot, edits its independent access region and declares scale behavior, physical-hand access, slot kind, allowed contents and optional visuals. For a wearable potion bag, the author can place the access region at its opening before launching Skyrim.

The editable project retains the authoring data; the export is a versioned declarative package that the runtime can validate and load. Neither Skyrim nor VRIK is required to author, reopen or export definitions. A mod may ship such a package without calling a live creation API. The format records stable provider/slot keys, schema and reference-rig versions, anchor names, local transforms, geometry/probe declarations, units, axes, scale conventions, policy bindings and asset references. Engine pointers and save-specific handles never enter the package; live eligibility callbacks cannot run in the editor.

Offline placement provides a default, not proof of reachability on every character. The package declares reference-to-runtime mapping and explicit conversion/rebinding rules. VRIK applies them to actual proportions, calibration and equipment movement. A missing anchor makes the slot unavailable instead of silently selecting another bone. Player overrides remain separate from the author's package; runtime ergonomics still need game checks. The editor is a proposed requirement, not an implemented feature or a selected tool.

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

Type, keyword and form-list filters support weapons, shields, torches, potions, food and tools. Registered bounded eligibility rules refine this generic policy; they do not mutate inventory. Body Slots owns policy registration and its configuration version; VRIK uses a coherent policy snapshot and requests any live eligibility decision through the registry at its declared phase. Callback ownership stays with the registry registration; serialized definitions do not carry executable callbacks. Consumers keep potion selection, stock, original/proxy semantics and assignment persistence. Exclusive/shared bindings declare who may offer an action without making the registry an inventory executor.

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

An equipment mod supplies records, assets and a declared slot set authored on its reference model. Body Slots creates and persists the instances; VRIK follows their anchors on the actual wearable model. The key combines the provider definition, particular equipment instance and slot within it, so identical bags keep separate assignments. The equipment provider validates durable save-scoped instance keys and declares stack split/merge, cloning and ownership-transfer rules. Rebinding restores one set; missing equipment, providers or unresolved identity leave it unavailable without attaching it to another item. Body Slots saves placement/visual overrides; content providers save their own rules under the same key.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status BindEquipmentSlotSet(ClientToken, const EquipmentSlotBinding*, SlotSetId* outSet, RequestId* outRequest);

}
```

### Registry and VRIK adapter

The public definition API and the VRIK runtime adapter are separate contracts. The registry can start without VRIK; capability negotiation attaches VRIK as the spatial/interaction/presentation runtime. It copies versioned definition changes outside the per-hand hot path and publishes anchor/presentation availability through its assigned adapter identity. Frame work uses an immutable configuration revision; queued actions revalidate it and the body/anchor generation before commitment.

**Pseudo-ABI**

```cpp
namespace BodySlots {

Status CopyDefinitionChanges(ClientToken, const DefinitionCursor*, SlotDefinitionSnapshot* outRecords, uint32_t capacity, uint32_t* outCount);

Status ReportRuntimeBinding(ClientToken, const SlotRuntimeReport*);

// Events: DefinitionAvailabilityChanged.

}
```

Each state has one authoritative writer: Body Slots for definitions/configuration, the content executor for gameplay assignments, VRIK for runtime binding and its committed-content mirror/presentation, HIGGS for physical possession. Configuration, content and presentation versions and body/anchor generations are distinct. Drag, MCM and API edits use expected revisions; callbacks run at declared phases outside registry locks. Provider removal stops new calls and drains in-flight callbacks before releasing code/context. Bind/unbind and customization are queued at declared boundaries rather than recursively changing the registry during a frame callback.

## Input and action sequence

1. HIGGS supplies grip/trigger samples and edges with physical hand, analog/touch distinction and an input sequence ID. VRIK supplies current zone/action offers.
2. Frameworks and consumers propose actions using those observations. A hold or multi-tap recognizer reserves the sequence with a decision deadline.
3. The broker selects one consuming action under declared contexts, bindings and user-selected priority. Observers remain able to read the input, but cannot independently act on it.
4. The domain executor revalidates and performs the operation at its legal phase.
5. Results distinguish accepted, committed, completed, rejected and cancelled. They carry request/transaction IDs, the request issuer and its epoch, and the selected executor; an event forwarder remains a separate identity.

Acceptance is not successful completion. Cancellation and deferred default input have declared behavior; synthetic gameplay requests never masquerade as physical button edges. Menus, missing tracking, load and provider shutdown invalidate pending work safely.

## Shared physical operation

Each owned hand/weapon body has one final actuator, managed or explicitly delegated by HIGGS. Contact constraints, weight/impedance objectives and an accepted embedded attachment contribute to one physical recipe. They cannot be resolved by allowing each callback to overwrite the pose in turn. Havok still solves world contacts; PLANCK still controls NPC physical animation. A penetration provider owns wound and extraction policy while PLANCK validates the NPC attachment endpoint.

A heavy lodged weapon touching a wall is a representative combined case. The contract reports saturation, break or rejection rather than silently teleporting the hand or losing the attachment. Damage and parry policy consume committed physical observations and enter the documented hit decision path once.

A climbing support point belongs to the climbing action and reserves the named hand. HIGGS accepts its palm constraint and scoped suppression of incompatible new grabs; VRIK renders the coordinated pose. The climbing consumer retains the player locomotion controller. Two-hand actions validate their footprint together and do not drop another hand's item merely to simplify a new interaction. Climbing starts only after its required hand reservation, physical support and restrictions become effective; acceptance alone is insufficient. Acquisition failure unwinds provisional resources before locomotion starts. Switching actions waits for confirmed physical hand release.

## Mods used to assess ABI expressiveness

These published mods were used as concrete requirements for ABI expressiveness. They remain external consumers; their gameplay does not become framework logic.

| Consumer | Requirement on the shared foundation |
|---|---|
| [Body Pouches VR](https://www.nexusmods.com/skyrimspecialedition/mods/194796) | Inventory-backed potion selection, persistent configurable slots and wearable bags, coherent release/zone observations, confirmed handover and independent slot previews |
| [Shields and 2H Weapons Unlocked](https://www.nexusmods.com/skyrimspecialedition/mods/184001) | VRIK zones/presentation with an explicit target hand and original/proxy item identity; its equipment adapter keeps the proxy policy |
| [Swap Drop and Hold Redux](https://www.nexusmods.com/skyrimspecialedition/mods/185816) | Input recognition reservations, explicit equip/release/hand transfer, exact grip relation, authoritative holster eligibility and final result |
| [Physical Collision VR](https://www.nexusmods.com/skyrimspecialedition/mods/186335) | Coherent rendered/collision/grab/hit transforms, contact snapshots and supported physical-drive integration; optional NPC contact/parry collaboration |
| [True Wield VR](https://www.nexusmods.com/skyrimspecialedition/mods/191123) | Grip geometry, mass/inertia, drive budgets, per-hand slide, two-hand state and measured motion; candidate damage policy through PLANCK |
| [Immersive Weapon Penetration VR](https://www.nexusmods.com/skyrimspecialedition/mods/184223) | Rich hit data, actor/bone/body lifecycle, validated attachment constraints, extraction outcomes and per-weapon interaction/visibility requests |
| [Immersive Harvesting VR](https://www.nexusmods.com/skyrimspecialedition/mods/186754) | Grip selection, contact/motion data, coordinated two-hand reservations, confirmed physical delivery of harvest results and haptics |
| [VR Climbing — Aelove Ver](https://www.nexusmods.com/skyrimspecialedition/mods/170321) | Climb-start priority, occupied-hand/support state, scoped HIGGS restrictions, visual pose coordination and world generations |

These are proposed requirements derived from those consumers, not promises that their current binaries already implement this integration.

## Body Pouches with the shared services

[Body Pouches 0.2.0](https://www.nexusmods.com/skyrimspecialedition/mods/194796) already uses VRIK's body slots and HIGGS grabbing/drinking. Its default is one exclusive potion pouch; shared and multiple-slot settings have narrower testing. The pouch saves a potion-selection rule and draws from the player's inventory. Its displayed bottle is a preview, not a separate stored item. [The release source](https://github.com/Vhodnoylogin/body-pouches/tree/0.2.0) shows where a richer shared contract could simplify the integration:

| Current responsibility inside Body Pouches | What the shared services could provide |
|---|---|
| Require an enabled weapon category for detection; suspend normal holstering for exclusive pouches | Body Slots definitions with independent geometry and potion eligibility; VRIK runtime binding and owner-scoped action rights |
| Read raw controller edges for the shared-slot fallback and reproduce withdrawal recognition | HIGGS input sequences and priority, with VRIK spatial action offers even for empty non-weapon slots |
| Reconstruct slot bones, transforms, hand mapping and camera offsets for shared detection and diagnostics | VRIK zone/probe snapshots in a declared space and phase; physical hand data from HIGGS |
| Defer a drop to the completed VRIK/HIGGS update, guard it with local serials and cancel it on regrab/load | A correlated release observation, matching zone sample and invalidation contract |
| Drop one inventory item, request a grab and correlate a general grabbed callback within a local timeout | One transfer request with separate inventory/grip results, item identity and declared failure disposition |
| Change an art model through a spell effect, wait for removal and support one exclusive pouch preview | Independent VRIK visuals and previews per slot, with readiness, version and attachment information |
| Coordinate its own world generations and readiness checks with saved assignments | Provider lifecycle events and stable slot keys, while Body Pouches keeps assignment serialization |

Body Pouches would still decide which potion effects match, reject poisons, order matching potions and save the player's assignments. It would keep the rule that all pouches draw from one inventory. A supported inventory executor could reuse verified CommonLib removal/pickup operations; moving those calls behind a transfer service does not move potion policy into VRIK or HIGGS. PLANCK is not required for this mechanic and should remain optional unless an added feature interacts with NPCs.

Keeping assignment persistence in Body Pouches still requires adapting its saved records: the release identifies assignments by numbered native slots. A versioned migration would map those assignments to native stable keys and store new bag assignments by equipment-slot keys, without attaching an old assignment to unrelated equipment.

This could make several visible additions practical: independently displayed pouches on several slots; explicit user priority when a potion pouch shares a location with a weapon; and an equippable potion bag whose slots follow the bag's model. A provider-declared access region near the bag opening or bottle neck could make reach behavior easier to understand. The framework would expose the tested probe and geometry for both hands; the present difference between visible bottle geometry and activation points still needs calibration and runtime checks. Providing a new ABI alone does not automatically resolve that ergonomic issue.

The simpler mod could ship slot definitions authored offline, register its content rules with Body Slots and executors with the interaction services, publish VRIK previews, and react to confirmed results. Request correlation and invalidation would still exist, but their shared implementation would replace private reconstructions in each consumer. These are proposed changes; they do not describe additional features already shipped in Body Pouches 0.2.0.

## Shields and 2H Weapons Unlocked with the shared services

[Shields and 2H Weapons Unlocked — VR](https://www.nexusmods.com/skyrimspecialedition/mods/184001), by TheMachinaGod, allows shields and two-handed weapons in either hand. Its unconventional equipment uses substitute items, or proxies, and its own shoulder holsters. In the [original implementation](https://github.com/SamuelJohnBrown/Weapons-shields-unlocked-skyrim-VR-framework-/blob/7518a1879a78ca494e77cd4d60c76ca5cba77172/main.cpp), those holsters replace VRIK's two shoulder slots by disabling their item permissions; this disabling is skipped when FalseEdgeVR is present. The conflict concerns the shoulder holsters, not every VRIK slot.

My experimental adaptation aims to retain the equipment feature while using VRIK's own zones, placement and display through its public ABI. The tested adaptation displays a sword on the back, draws it into the right hand and preserves it across save/load. However, an empty left hand receives no offer from the occupied shoulder slot. The interface also cannot explicitly address both a particular slot and a target physical hand. Adapting the proxy's type to satisfy equipment, holster and display rules therefore requires coordinating inventory, Papyrus, models and physical state through local workarounds.

| What the original mod or experimental adaptation has to do | What shared services could provide |
|---|---|
| Original: create private shoulder zones and disable VRIK's shoulder holsters to avoid competing actions | Body Slots: shared placement and content policy; VRIK: optional native bindings and presentation, with provider action rights instead of disabling the player's holsters |
| Adaptation: rely on weapon-type and hand filtering before receiving an occupied slot's action offer; the empty left hand is filtered out | VRIK: independent zone observations and provider-defined action offers, while preserving the player's explicit access restrictions |
| Adaptation: fit drawing into VRIK's internal hand choice; the existing call cannot name both slot and destination hand | VRIK: an explicit slot-to-physical-hand request routed to the mod's equipment executor, with a confirmed result |
| Adaptation: use proxy type for equipment, display and holster eligibility at once; changing it for one purpose affects the others | Body Slots: independent eligibility configuration; VRIK: separate content/presentation observations, distinguishing original item, proxy and displayed model |
| Maintain original/proxy identity, instance properties, counts, restoration and drop handling while other systems observe inventory changes | A shared operation contract with instance identity, reservations and one authorized inventory executor; the mod retains property-transfer policy |
| Reconcile the mod's records, VRIK's Papyrus content and the displayed model | VRIK: committed slot content and version, plus presentation readiness tied to that version |
| Coordinate delayed tasks and later Papyrus checks that may observe an already changed proxy type | Executor contract: an immutable request, a legal execution phase and correlated accepted, committed and completed results |
| Guard against a draw immediately following holstering while the same hand remains near the shoulder | HIGGS and VRIK: correlated input sequences and spatial offers that distinguish continuation of a gesture from a new action |
| Reconstruct hand occupancy and equipment readiness from game equipment, HIGGS grip state and VRIK flags | HIGGS: distinct equipment/physical-possession/two-hand observations and confirmed grip or transfer results; VRIK: presentation readiness. Transfers reserve affected hands together and preserve the other hand's item |
| Restore proxy/slot associations after loading and investigate undocumented meanings or call ordering | Providers: stable keys, lifecycle events and documented observations; CommonLib: verified engine operations. The mod retains its own saved state |

The equipment mod still owns unconventional equip rules, proxy selection and preservation of instance properties. Body Slots owns saved slot definitions and placement; VRIK owns runtime body zones, placement mapping, the committed-content mirror and display; HIGGS owns physical possession and coordinates input and hand resources. A valid equipment record alone does not prove a visible model, physical body or working collision. Shared results make those stages observable; they do not guarantee rollback after arbitrary gameplay side effects. PLANCK participates in physical hits involving NPCs and need not become a required inventory or holster executor. These are proposed services, not a completed port or guarantees of today's ABI.

## Climbing and harvesting

[VR Climbing](https://www.nexusmods.com/skyrimspecialedition/mods/170321) manages its own temporary HIGGS restrictions so a grip on a wall does not also grab an item. It changes shared HIGGS settings while climbing and restores them afterward. In effect, it creates its own way to reserve hand interactions for its gameplay.

[Immersive Harvesting](https://www.nexusmods.com/skyrimspecialedition/mods/186754) encounters the same grip input: beside a plant, the player may want to harvest rather than climb. It adds a separate workaround by intervening in the climb-start check. One consumer builds its own HIGGS interaction control, and another then has to adapt to that control.

In the proposed cooperation, HIGGS provides this shared service. It knows which hand is occupied, who owns the current action and which new actions are offered. VR Climbing requests a hand for support; Immersive Harvesting requests it for harvesting. HIGGS selects an eligible action under the user's priority, confirms its start and reports its completion. Each consumer releases only its own reservation, without restoring settings over another consumer's requests.

For example, beside a plant and a wall, the user can prefer harvesting over starting a climb. A hand already supporting a climb must be released first. An item held in the other hand must not be dropped merely to allow harvesting. The shared mechanism provides these rules to all HIGGS consumers.

VR Climbing then focuses on climbing, and Immersive Harvesting on harvest rules and results. They no longer need to recreate shared hand control or patch each other for this conflict. VRIK supplies spatial hand information and the coordinated visible pose; PLANCK participates when NPC physical interaction is involved. Player movement and fall rules remain with the climbing consumer.

## Compatibility and ABI lifecycle

Keep existing 001 interfaces and semantics unchanged. Negotiate additive services and capabilities separately. Missing optional providers return a defined result or use an explicitly documented legacy fallback. Old consumers that directly mutate private state cannot automatically gain the new ownership guarantees.

Extracting slots requires an additive VRIK adapter: original 001 alone can configure existing holsters but cannot create arbitrary new slots. Preserve the old vtable, Papyrus names, saved arrays and event-acceptance semantics; an old boolean does not prove a committed transfer. Keep the legacy arrays at 14, because resizing can reset saved assignments. New slots use separate stable-keyed records. When the registry is active, compatible legacy settings edits translate to it, rather than competing as a second configuration writer. Fixed numbered anchors/art resources and weapon-dependent hover, access geometry and presentation need runtime adapters; native weapon assignments and the existing Papyrus equip executor remain in their compatibility path. These are reconstruction-informed boundaries to validate against the original behavior, not author-confirmed implementation details.

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

On this foundation, HIGGS manages input and physical possession, VRIK manages the body and runtime slot interactions, Body Slots manages slot definitions and customization, and PLANCK manages NPC physics. CommonLib provides their shared technical tools; decisions about pouch contents, climbing, harvesting and hits belong to the owners of those mechanics. The two levels complement each other: common engine code in the library and coordinated gameplay interaction through the mods' APIs/ABIs.

## User priority and mod order

The user should control which eligible action takes precedence. A grip beside a plant and a wall can mean harvesting, grabbing an item or starting a climb. The broker should expose priority for that context and explain the decision.

Mod order can be one supported way to express that choice. One proposed route is compatibility-rule files sharing a path, so the user chooses the winning file through MO2/Vortex file priority. An explicitly enabled per-action setting can override that order. An optional adapter could import a chosen mod ordering into the priority list. DLL-only mods have no ESP whose position establishes this arbitration; the connection between user ordering and action selection must be implemented rather than assumed.

For eligible new actions, the proposed resolution is an explicit user assignment for the context, then the selected user priority list, then a stable action ID for ties. A visible default profile handles missing custom rules. A consumer or callback registration timestamp cannot silently override the user's choice.

The broker also issues provisional hold/multi-tap reservations under that priority. They have a bounded decision window and declared preemption rules. A recognizer cannot reserve first and make the preferred action ineligible on its own authority. Each input sequence pins a priority-rule version. Configuration changes apply to the next sequence or explicitly cancel the current one under a user-selected policy.

Priority applies to **eligible new actions**. Existing possession and committed hand reservations are checked first: harvesting must not steal a hand already supporting a climb. Release or interruption is an explicit action. Reordering priority cannot duplicate a committed transfer. The user controls gameplay choice while ownership, object validity and exactly-once commitment remain enforced.
