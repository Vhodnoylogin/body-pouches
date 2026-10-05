# Cooperation between VRIK HIGGS and PLANCK

Russian: [Русская версия](concept-ru.md).

This proposal describes how the three frameworks could provide a coherent base
for Skyrim VR mods. Each framework owns its existing domain and exposes the
observations and requests that other mods need. It is a direction for discussion,
not an agreed design or a promise of an available API.

The intended benefit is concrete: a mod author can ask for a body zone, a physical
handover or an NPC attachment without recreating another framework's internals.
A single interaction has identifiable participants and a confirmed result.

## Responsibilities

| Provider | Owns | Provides to other mods |
|---|---|---|
| VRIK | Player body and final skeletal pose; body anchors, spatial hand interpretation, zones, slots and their presentation | Coherent pose and zone snapshots, zone transitions, eligible slot actions, content previews, named-hand slot requests and scoped reservations |
| HIGGS | Grip/trigger input brokerage, haptic scheduling, physical hands/weapons, grabbing and physical possession | Input observations and reservations, physical grip state, explicit grab/release/transfer requests, supported grip/drive/constraint integration and action receipts |
| PLANCK | NPC physical animation and ragdoll lifecycle; its physical-hit interpretation and commitment | Actor/body/bone metadata, validated attachment endpoints, candidate/committed hit data, scoped interaction policies and lifecycle events |
| External mod | Its gameplay feature and content policy | Pouch selection, unconventional equipment/proxies, handling gestures, weight response, penetration/extraction or parry rules, expressed through provider contracts |
| CommonLib | Generic engine types, references, locks, math and verified scene/Havok wrappers | Technical building blocks for all providers; gameplay arbitration remains in runtime services owned by the mods |

Input brokerage does not move every gesture into HIGGS. VRIK can recognize spatial
gestures and a handling mod can recognize taps/holds. They use a common input
stream and submit candidates; the broker resolves ownership before defaults act.
Equally, forwarding an action result does not make HIGGS the authority for damage,
pouch contents or every inventory operation. The result identifies its executor.

## VRIK body hands zones and slots

VRIK owns the virtual player body and spatial interpretation of the hands.
**IK means inverse kinematics:** given head and hand positions, VRIK calculates
the remaining skeletal pose, including shoulders and elbows. A known hand
position alone does not determine where the elbow should be.

VRIK supplies body anchors and zones. HIGGS supplies the physical palm/item state
that VRIK uses to align the visible hand. Moving the character during climbing
remains the climbing mod's responsibility; skeletal pose calculation and player
locomotion are separate services.

Three observations must remain distinct: the tracked controller, the physically
resolved palm/grip, and the final hand rendered by VRIK's IK. HIGGS supplies
physical constraints; VRIK presents their spatial interpretation together with
body anchors and the resulting visual pose. A wall or embedded blade must not
be ignored merely because the controller moved farther.

A zone states which probe it tests: normally the resolved palm, optionally a
controller, held-item anchor or weapon tip. Samples carry time, validity, space
and generation. Previous-frame inputs are explicit. Final rendered IK must not
feed back as the same cycle's physical target. This defines dependencies without
assuming every engine update has the same order or one physics step per frame.

A zone is a spatial region anchored to the body. A slot adds occupancy, content
policy and presentation. An action describes what can be done there. Their states
are separate: an empty, invisible or reserved slot can still have observable
geometry. Zone entry/exit and changes to available actions are separate events.

Slots support declarative type/keyword/form-list filters and bounded provider
eligibility rules. Weapons, shields and torches remain supported; a provider can
also use potions, food, tools or other items without inventing a weapon identity.
Display models and content are separate, with explicit presentation readiness.

VRIK supplies the geometry and presentation; the content provider decides what
belongs there. A transfer reserves its source, destination and possession and
uses one inventory mutation path. It preserves item instance data and commits
exactly once. Cancellation before commitment differs from compensation afterward;
full rollback cannot be promised after arbitrary game-script side effects.

Slot operations name the requested physical hand and the other hands/resources
they reserve, then report actual committed occupancy. Equipment providers may
adapt an original item to an execution proxy and a displayed model: all three
identities remain distinct, preserving enchantment, charge, tempering and
ownership. VRIK need not reproduce each proxy pipeline.

Custom actions name a registered domain executor. Eligibility callbacks only
decide whether an action is possible; the selected executor commits inventory
through the agreed path and reports its result. VRIK publishes committed slot
content/version from that receipt; previews identify the content version they show.

Body anchors are the main zone use case. An optional extension could anchor a
zone to a world reference and its generation, allowing the same spatial contract
around a plant or surface. The consumer still selects harvest targets, decides
whether a surface is climbable and discovers relevant world objects. CommonLib
provides geometry and engine queries; VRIK does not scan the world for every
consumer. Losing an anchor invalidates the zone and produces an event.

The following illustrative declarations belong to VRIK's responsibility. Their
names and record layouts are discussion examples, not an available VRIK SDK.

**Pseudo-ABI — illustrative example, not a final interface.**

```cpp
namespace VRIK {

Status CopyHandPose(ClientToken, PhysicalHand, PoseSource, HandPose* outPose);

Status RequestHandGoal(ClientToken, const HandGoal*, Lease* outGoal, RequestId* outRequest);

Status CreateZone(ClientToken, const ZoneDefinition*, ZoneId* outZone);

Status UpdateZone(ClientToken, ZoneId, const ZoneDefinition*, RequestId* outRequest);

Status DestroyZone(ClientToken, ZoneId, RequestId* outRequest);

Status QueryZones(ClientToken, const ZoneProbe*, ZoneSnapshot* outZones, uint32_t capacity,
    uint32_t* outCount);

Status RegisterSlot(ClientToken, ZoneId, const SlotContentPolicy*, ContentEligibilityCallback,
    void* userContext, SlotId* outSlot);

Status UnregisterSlot(ClientToken, SlotId, RequestId* outRequest);

Status CopyActionOffers(ClientToken, const SlotRequest*, ActionOffer* outOffers, uint32_t capacity,
    uint32_t* outCount);

Status ReserveSlot(ClientToken, SlotId, const SlotRequest*, Lease* outReservation);

Status RequestSlotAction(ClientToken, const SlotRequest*, RequestId* outRequest);

Status CopySlotContent(ClientToken, SlotId, SlotContentSnapshot* outContent);

Status UpdateSlotContentFromCommit(ClientToken, SlotId, const SlotContentCommit*,
    RequestId* outRequest);

Status RequestItemPreview(ClientToken, SlotId, const ItemPreview*, RequestId* outRequest);

// Events: PoseAvailable, ZoneEntered, ZoneExited, ZoneInvalidated,
//         ActionOffersChanged, SlotContentChanged, PresentationReady.
}
```

CopyHandPose explicitly selects the controller, resolved palm or rendered hand.
RequestHandGoal expresses an objective or constraint, never permission to write
bones directly. QueryZones serves clients that subscribe after entry.
RequestSlotAction routes to a registered domain executor.
UpdateSlotContentFromCommit publishes an authoritative receipt without creating
another inventory item; PresentationReady names the displayed content version.
Removing a slot with an eligibility callback must drain its in-flight calls.
Content publication validates the authorized executor and expected slot version;
an unrelated or stale commit cannot update the slot.

## HIGGS input possession and physical requests

HIGGS supplies the shared grip/trigger stream, input consumption and haptic
scheduling, and owns physical hands, weapons and possession. It reports both
the input and the confirmed outcome of the selected action. Forwarded results
preserve the identity of the actual domain executor.

VR controller ownership here means device input and haptics. It does not move
player locomotion, climbing rules, harvesting or slot contents into HIGGS.
Recognizers keep their policies and submit actions through the shared contract.

Physical grip, constraint and transfer requests have receipts. Temporary effects
identify an owner, the affected hand/item and a lifetime. RequestSuppression
limits named features without implicitly releasing an item; RequestRelease
explicitly asks to release it. Ending a climb removes only the climbing mod's
effects and leaves another consumer's restrictions intact.

**Pseudo-ABI — illustrative example, not a final interface.**

```cpp
namespace HIGGS {

Status CopyInput(ClientToken, PhysicalHand, InputObservation* outInput);

Status RegisterRecognizer(ClientToken, const InputBinding*, RecognizerCallback, void* userContext,
    Lease* outRegistration);

Status SubmitActionCandidate(ClientToken, const ActionCandidate*, RequestId* outRequest);

Status RegisterActionExecutor(ClientToken, const ActionExecutorInfo*, ActionExecutorCallback,
    void* userContext, ExecutorId* outExecutor, Lease* outRegistration);

Status ReportExecutionResult(ClientToken, const ExecutionResult*);

Status CopyHandState(ClientToken, PhysicalHand, HandState* outState);

Status CopyMotionSamples(ClientToken, PhysicalHand, PoseSource, uint32_t space,
                         MotionSample* outSamples, uint32_t capacity, uint32_t* outCount);

Status RequestGrab(ClientToken, const GrabRequest*, RequestId* outRequest);

Status RequestRelease(ClientToken, const ReleaseRequest*, RequestId* outRequest);

Status RequestTransfer(ClientToken, const TransferRequest*, RequestId* outRequest);

Status RequestGripChange(ClientToken, const GripChange*, RequestId* outRequest);

Status RequestDrivePolicy(ClientToken, const DrivePolicy*, Lease* outPolicy, RequestId* outRequest);

Status RequestConstraint(ClientToken, const ConstraintRequest*, Lease* outConstraint,
    RequestId* outRequest);

Status RequestSuppression(ClientToken, const SuppressionRequest*, Lease* outLease,
    RequestId* outRequest);

Status RequestHaptic(ClientToken, const HapticRequest*, RequestId* outRequest);

Status CopyEffectiveInputPriority(ClientToken, const InputContext*, InputPriority*);

Status CopyInputDecision(ClientToken, uint64_t sequenceId, InputDecision*);

Status CopyContactSnapshot(ClientToken, const ContactQuery*, ContactSnapshot*);

// Events: InputChanged, InputDecisionMade, InteractionChanged, ContactChanged,
//         ConstraintChanged, ActionResult.
}
```

InputBinding declares context, action, recognition deadline and deferred-default
behavior. ActionCandidate identifies the input sequence, resource footprint and
registered executor. InputDecision reports the winner and the applicable user
priority rule. CopyHandState distinguishes physical left/right from dominant/off
hand. ContactSnapshot identifies the contact source, object, point, normal,
motion and body generations; a plant or static-wall contact is not an NPC hit.

Physical changes apply at the documented safe phase. CopyMotionSamples separates
controller movement in VR tracking space from item movement in world space;
climbing needs the former for intent and the latter for contacts.
RegisterActionExecutor gives consumers a supported execution route. Eligibility
and recognition callbacks do not mutate inventory. ReportExecutionResult accepts
only the assigned executor, validates the request provider/epoch, ignores duplicates
and never regresses a committed stage. Motion samples identify the versioned
tracking-to-world transform: locomotion, teleportation or recentering can change
it without replacing the world or body.

## PLANCK NPC physics and hits

PLANCK owns NPC physical animation, ragdoll transitions, body lifetime and its
physical-hit processing. It supplies NPC metadata and validated attachment
endpoints. A penetration mod owns wound and extraction rules; PLANCK validates
the target body and HIGGS applies the weapon constraint.

A bounded hit-policy callback runs before the relevant hit commitment and does
not write live engine objects. HitCommitted reports the outcome. Merely observing
HitCandidateObserved is not a writable damage hook. Policy conflicts have declared
rules and user-visible priorities wherever alternatives are selectable.

**Pseudo-ABI — illustrative example, not a final interface.**

```cpp
namespace PLANCK {

Status CopyActorPhysics(ClientToken, ActorHandle, ActorPhysics* outState);

Status AcquireAttachmentEndpoint(ClientToken, const AttachmentQuery*,
    AttachmentEndpoint* outEndpoint, Lease* outEndpointLease);

Status RequestActorPolicy(ClientToken, const ActorPolicy*, Lease* outPolicy, RequestId* outRequest);

Status RegisterHitPolicy(ClientToken, uint64_t scope, HitPolicyCallback, void* userContext,
    Lease* outRegistration);

Status CopyHitSnapshot(ClientToken, uint64_t hitId, HitSnapshot* outSnapshot);

// Events: ActorPhysicsChanged, EndpointInvalidated, HitCandidateObserved,
//         HitCommitted, ActionResult.
}
```

AttachmentEndpoint identifies world, actor and body generations. Ragdoll
replacement, NPC unload or game load invalidates dependent endpoints and
constraints safely. RequestActorPolicy applies to its declared scope rather than
changing global thresholds for every weapon. Flora and scripted activators are
not NPCs; harvesting needs PLANCK only when it actually interacts with its actors.

## Input and action sequence

1. HIGGS supplies grip/trigger samples and edges with physical hand, analog/touch
   distinction and an input sequence ID. VRIK supplies current zone/action offers.
2. Frameworks and consumers propose actions using those observations. A hold or
   multi-tap recognizer reserves the sequence with a decision deadline.
3. The broker selects one consuming action under declared contexts, bindings and user-selected priority.
   Observers remain able to read the input, but cannot independently act on it.
4. The domain executor revalidates and performs the operation at its legal phase.
5. Results distinguish accepted, committed, completed, rejected and cancelled.
   They carry request/transaction IDs, the request issuer and its epoch, and the
   selected executor; an event forwarder remains a separate identity.

Acceptance is not successful completion. Cancellation and deferred default input
have declared behavior; synthetic gameplay requests never masquerade as physical
button edges. Menus, missing tracking, load and provider shutdown invalidate
pending work safely.

## User priority and mod order

The user should control which eligible action takes precedence. A grip beside
a plant and a wall can mean harvesting, grabbing an item or starting a climb.
The broker should expose priority for that context and explain the decision.

Mod order can be one supported way to express that choice. One proposed route is
compatibility-rule files sharing a path, so the user chooses the winning file
through MO2/Vortex file priority. An explicitly enabled per-action setting can
override that order. An optional adapter could import a chosen mod ordering into
the priority list. DLL-only mods have no ESP whose position establishes this
arbitration; the connection between user ordering and action selection must be
implemented rather than assumed.

For eligible new actions, the proposed resolution is an explicit user assignment
for the context, then the selected user priority list, then a stable action ID
for ties. A visible default profile handles missing custom rules. A consumer or
callback registration timestamp cannot silently override the user's choice.

The broker also issues provisional hold/multi-tap reservations under that priority.
They have a bounded decision window and declared preemption rules. A recognizer
cannot reserve first and make the preferred action ineligible on its own authority.
Each input sequence pins a priority-rule version. Configuration changes apply to
the next sequence or explicitly cancel the current one under a user-selected policy.

Priority applies to **eligible new actions**. Existing possession and committed
hand reservations are checked first: harvesting must not steal a hand already
supporting a climb. Release or interruption is an explicit action. Reordering
priority cannot duplicate a committed transfer. The user controls gameplay choice
while ownership, object validity and exactly-once commitment remain enforced.

## Shared physical operation

Each owned hand/weapon body has one final actuator, managed or explicitly delegated
by HIGGS. Contact constraints, weight/impedance objectives and an accepted embedded
attachment contribute to one physical recipe. They cannot be resolved by allowing
each callback to overwrite the pose in turn. Havok still solves world contacts;
PLANCK still controls NPC physical animation. A penetration provider owns wound
and extraction policy while PLANCK validates the NPC attachment endpoint.

A heavy lodged weapon touching a wall is a representative combined case. The
contract reports saturation, break or rejection rather than silently teleporting
the hand or losing the attachment. Damage and parry policy consume committed
physical observations and enter the documented hit decision path once.

A climbing support point belongs to the climbing action and reserves the named
hand. HIGGS accepts its palm constraint and scoped suppression of incompatible
new grabs; VRIK renders the coordinated pose. The climbing consumer retains the
player locomotion controller. Two-hand actions validate their footprint together
and do not drop another hand's item merely to simplify a new interaction.
Climbing starts only after its required hand reservation, physical support and
restrictions become effective; acceptance alone is insufficient. Acquisition
failure unwinds provisional resources before locomotion starts. Switching actions
waits for confirmed physical hand release.

## Compatibility and ABI lifecycle

Keep existing 001 interfaces and semantics unchanged. Negotiate additive services
and capabilities separately. Missing optional providers return a defined result
or use an explicitly documented legacy fallback. Old consumers that directly
mutate private state cannot automatically gain the new ownership guarantees.

Requests, reservations and subscriptions identify their owner. One consumer
cannot release another's suppression or reservation. Unloading requires stopping
new callbacks and draining in-flight calls. Snapshots use copied, size-tagged
records with explicit units, spaces, hand mapping and world/body generations;
new boundaries do not transfer STL objects or ownership of allocated memory.

The illustrative ABI is embedded in each provider section. Supporting records are
abbreviated; numeric values, layouts, calling convention and transport are not
final. Each provider exposes the same lifecycle operations:

**Pseudo-ABI — illustrative example, not a final interface.**

```cpp
enum class PhysicalHand : uint32_t { Left, Right };
enum class PoseSource : uint32_t { TrackedController, ResolvedPalm, RenderedIK };
enum class RequestStage : uint32_t {
    Accepted, Committed, Completed, Rejected, Cancelled
};

Status RegisterClient(const ClientIdentity*, ClientToken*);

Status ReleaseClient(ClientToken);

Status Subscribe(ClientToken, uint64_t eventMask, EventCallback,
                 void* context, Subscription*);

Status Unsubscribe(ClientToken, Subscription);

Status CopyEventPayload(ClientToken, SnapshotId, void* callerBuffer,
                        uint32_t bufferBytes, uint32_t* requiredBytes);

Status GetRequestResult(ClientToken, RequestId, RequestResult*);

Status CancelRequest(ClientToken, RequestId);

Status ReleaseLease(ClientToken, Lease, RequestId*);

// Every provider negotiates its own version, table size and capabilities.
// Event records name source/forwarder, issuer/epoch, executor, phase and thread.
// Pose/contact records name time, coordinate space, units and generations.
// RequestId is scoped by provider and epoch; TransactionId correlates participants.
```

Queued operations return a request ID. Temporary effects also return a provisional
lease that becomes effective on commitment and retires on rejection/cancellation.
ReleaseClient and Unsubscribe prevent new dispatch and confirm in-flight drainage;
callback-bearing registrations follow the same rule. Free code/context only after
every relevant provider confirms drainage. Do not block physics or a callback.

Copied records use caller buffers and report required capacity when insufficient.
Hit policies and eligibility callbacks are bounded and never mutate inventory.
Main-frame and physics-tick identities remain separate. Loading changes epochs;
runtime lease values are not persistent slot IDs.

## CommonLib responsibilities

CommonLib supplies generic engine types and verified operations: references,
geometry/transforms, locks, scene and physics access, ray queries and character
controller wrappers. Existing coverage should be reused; missing methods are
candidates only after signature, layout, address and behavior verification.
This is not a claim that the current library already covers every operation.

Input arbitration, hand possession, slots and hit policies remain runtime
services owned by the frameworks. Linking CommonLib into three DLLs creates
three copies of code, not a shared ownership service. Harvest, climbing and fall
rules remain consumer gameplay even when they use CommonLib engine methods.

## Mods used to assess ABI expressiveness

These published mods were used as concrete requirements for ABI expressiveness.
They remain external consumers; their gameplay does not become framework logic.

| Consumer | Requirement on the shared foundation |
|---|---|
| [Shields and 2H Weapons Unlocked](https://www.nexusmods.com/skyrimspecialedition/mods/184001) | VRIK zones/presentation with an explicit target hand and original/proxy item identity; its equipment adapter keeps the proxy policy |
| [Swap Drop and Hold Redux](https://www.nexusmods.com/skyrimspecialedition/mods/185816) | Input recognition reservations, explicit equip/release/hand transfer, exact grip relation, authoritative holster eligibility and final result |
| [Physical Collision VR](https://www.nexusmods.com/skyrimspecialedition/mods/186335) | Coherent rendered/collision/grab/hit transforms, contact snapshots and supported physical-drive integration; optional NPC contact/parry collaboration |
| [True Wield VR](https://www.nexusmods.com/skyrimspecialedition/mods/191123) | Grip geometry, mass/inertia, drive budgets, per-hand slide, two-hand state and measured motion; candidate damage policy through PLANCK |
| [Immersive Weapon Penetration VR](https://www.nexusmods.com/skyrimspecialedition/mods/184223) | Rich hit data, actor/bone/body lifecycle, validated attachment constraints, extraction outcomes and per-weapon interaction/visibility requests |
| [Immersive Harvesting VR](https://www.nexusmods.com/skyrimspecialedition/mods/186754) | Grip selection, contact/motion data, coordinated two-hand reservations, confirmed physical delivery of harvest results and haptics |
| [VR Climbing — Aelove Ver](https://www.nexusmods.com/skyrimspecialedition/mods/170321) | Climb-start priority, occupied-hand/support state, scoped HIGGS restrictions, visual pose coordination and world generations |

These are proposed requirements derived from those consumers, not promises that
their current binaries already implement this integration.

## Climbing and harvesting

[VR Climbing](https://www.nexusmods.com/skyrimspecialedition/mods/170321)
manages its own temporary HIGGS restrictions so a grip on a wall does not also
grab an item. It changes shared HIGGS settings while climbing and restores them
afterward. In effect, it creates its own way to reserve hand interactions for
its gameplay.

[Immersive Harvesting](https://www.nexusmods.com/skyrimspecialedition/mods/186754)
encounters the same grip input: beside a plant, the player may want to harvest
rather than climb. It adds a separate workaround by intervening in the climb-start
check. One consumer builds its own HIGGS interaction control, and another then
has to adapt to that control.

In the proposed cooperation, HIGGS provides this shared service. It knows which
hand is occupied, who owns the current action and which new actions are offered.
VR Climbing requests a hand for support; Immersive Harvesting requests it for
harvesting. HIGGS selects an eligible action under the user's priority, confirms
its start and reports its completion. Each consumer releases only its own
reservation, without restoring settings over another consumer's requests.

For example, beside a plant and a wall, the user can prefer harvesting over
starting a climb. A hand already supporting a climb must be released first.
An item held in the other hand must not be dropped merely to allow harvesting.
The shared mechanism provides these rules to all HIGGS consumers.

VR Climbing then focuses on climbing, and Immersive Harvesting on harvest rules
and results. They no longer need to recreate shared hand control or patch each
other for this conflict. VRIK supplies spatial hand information and the coordinated
visible pose; PLANCK participates when NPC physical interaction is involved.
Player movement and fall rules remain with the climbing consumer.
