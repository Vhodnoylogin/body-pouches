# Body Slots on the current public HIGGS and VRIK APIs

Assessment date: 28 October 2026. Design basis: the maintained [English concept](concept-en.md) and [Russian concept](concept-ru.md) at Body Pouches commit `b29026bcf753df9eca06f85ab88ac3ea396a3064`.

**Conclusion:** an independent Body Slots system can own additional slots, equipment binding, geometry, presentation and saved configuration without adding holsters to VRIK. Its observation adapter still needs qualification against released VRIK. The complete concept also requires shared hand state, input grants and confirmed hand operations that the current public HIGGS interface does not provide. Those guarantees cannot be supplied by moving a second hand controller into Body Slots without changing the agreed responsibilities.

This is a source/API feasibility assessment, not implementation or game acceptance. It does not resume development, change the concept, or certify the existing experimental Body Slots DLL. “Possible without provider changes” includes Body Slots' own implementation and ordinary SKSE/CommonLib engine access; it does not mean that HIGGS or VRIK already implements the feature. No reconstructed VRIK source is needed for the proposed observation route or was read for this assessment.

## Checked public baseline

| Provider | Inspected evidence | Boundary |
|---|---|---|
| HIGGS | Public upstream `master` at [`93bf67b1bc4c4a11a20ccaef0d5012781d0d7eee`](https://github.com/adamhynek/higgs/commit/93bf67b1bc4c4a11a20ccaef0d5012781d0d7eee); source version 1.10.10.0; `IHiggsInterface001` | The relevant public interface, hand and API implementation files are unchanged from tag `v1.10.10`. The installed package records 1.10.10.0, but this review does not prove source/binary equivalence or execute the DLL |
| VRIK | Published [0.8.8 package](https://www.nexusmods.com/skyrimspecialedition/mods/23416?tab=files), main file 820033, shipped Papyrus sources/settings, and the public [28-method C++ 001 contract](https://github.com/Vhodnoylogin/body-pouches/blob/b29026bcf753df9eca06f85ab88ac3ea396a3064/contract/vrikinterface001.h) | The separately published C++ API package remains labelled 0.8.7. The [earlier update assessment](upstream-update-2026-10-26.md) found the 001 layout retained in 0.8.8; this pass does not repeat binary inspection or establish game compatibility |

The downloaded VRIK 0.8.8 archive has SHA-256 `62f592b4fa1f83e8c8a71edc74cf4d317cb71ae2658386d16c2c0167b92117db`. The public script observations below refer to `Scripts/VRIK.psc`, `Scripts/_vrik_qust_system.psc` and `SKSE/Plugins/vrikslots.ini` shipped in that archive.

## What Body Slots can provide without modifying HIGGS or VRIK

| Concept feature | Feasible route | Limit of the current evidence |
|---|---|---|
| Additional slots, stable definitions and instances, creation/removal API | Body Slots' own registry and versioned packages | No architectural dependence on VRIK's fourteen-entry holster arrays. Practical capacities still need explicit limits; this is not literally infinite allocation |
| Slot placement, rotation, independent access regions, enable flags and saved customization | Body Slots geometry/configuration plus SKSE serialization | These are its own state and persistence. They do not require a new VRIK setter or changing VRIK saves |
| Slot types and potion/food/tool/weapon filters | Body Slots policy bindings and bounded eligibility providers | Filtering an item is distinct from obtaining permission to use a hand or committing inventory changes |
| Equippable bags that activate one or several slots | Ordinary equipment records and assets, worn-equipment observation, model/node binding and Body Slots instance activation | Multiple identical bags need durable instance identity and split/merge/transfer rules; a base form ID alone is insufficient. This is equipment/provider work, not a missing VRIK slot API |
| Slot regions following the visible body or bag | Observe actual skeleton/equipment nodes and transform a local access region into world space | Binding, pose phase, scale, model replacement, tracking loss and calibration require validation. Missing anchors must make the slot unavailable |
| Entry, exit, hover and overlap offers | Body Slots evaluates geometry against qualified hand observations | Geometry can operate without performing an action. Observation uncertainty must not become a fabricated entry or an authorization |
| Slot visuals and item previews | Ordinary equipped models and a separate Body Slots scene presentation service | Existing VRIK Art helpers attach to native slots; independent visuals need their own implementation. Model readiness, scene lifetime, loading and preview versions remain work |
| Standard and custom slot behavior providers | Body Slots selects slot offers and dispatches one addressed request | A slot-only selection rule can be implemented locally. It cannot guarantee that HIGGS, VRIK gestures or an unrelated mod does not act on the same controller input |
| Offline placement and package export in Morphbench | Author definitions against a reference rig and export declarative packages | Neither provider needs to change for offline authoring. Runtime binding and reachability are separate qualifications |

SKSE/CommonLib already exposes scene nodes/transforms, attachment operations and serialization facilities needed to investigate these routes. Using them does not turn CommonLib into a slot or hand-state owner and does not require implementing the second-stage CommonLib consolidation now. Availability of an engine wrapper alone is not runtime qualification.

### Hand positions are publicly exposed, but not as the complete proposed pose contract

VRIK's C++ 001 interface has no general hand/body pose snapshot. However, its **public Papyrus API does expose `VrikGetHandX`, `VrikGetHandY` and `VrikGetHandZ`**, together with HMD coordinates (`VRIK.psc`, lines 258–264). The comment promises world-space coordinates. It does not specify whether these are rendered IK hands, controller targets or resolved physical palms, nor orientation, a common sample ID, phase, validity or generations. Separate scalar reads do not establish a coherent pose sample.

Those functions provide a possible basic point-zone observation route. This review demonstrates neither adequate polling latency nor a performance problem. For a native runtime, an independently written scene-node adapter is another route. The public [Body Pouches observation code](https://github.com/Vhodnoylogin/body-pouches/blob/b29026bcf753df9eca06f85ab88ac3ea396a3064/src/game/Body.cpp#L40-L88) illustrates the pattern, but its selected skeleton, transform field and update phase must be revalidated rather than inherited as a VRIK guarantee.

HIGGS supplies callbacks around VRIK/HIGGS updates. They help select when an adapter reads data; they do not themselves provide an input-matched pose snapshot, producer validity or tracking/calibration generations. Local adapter generations must not be described as generations supplied by VRIK.

## What the current HIGGS interface supports only partially

The [public interface](https://github.com/adamhynek/higgs/blob/93bf67b1bc4c4a11a20ccaef0d5012781d0d7eee/include/higgsinterface001.h) supplies physical possession queries, grab requests, two-handing observations, callbacks, rigid-body access and grab-relative transforms. This is enough for constrained legacy integrations, including the kind already used by Body Pouches, but not the concept's common authority.

| Required contract | Existing public facility | Why it is insufficient |
|---|---|---|
| Unified occupancy and availability | `IsHoldingObject`, `GetGrabbedObject`, `CanGrabObject`, `IsDisabled`, `IsTwoHanding` | These expose different predicates. There is no complete copied record containing equipment use, known/unknown observations, resource owner, reason and generation. `CanGrabObject` concerns grabbing, not permission for every hand action |
| Inventory-to-hand delivery | `GrabObject` and grab notifications | The request returns `void`, has one pending object per hand, and has no request identity, ownership, acceptance or correlated failure result. Inventory mutation and compensation remain the consumer's responsibility |
| Release, stash or consume observations | Dropped/stashed/consumed callbacks | Notifications lack an input-matched pose, item-instance transaction identity and declared commitment/completion stages. Stash/consume callbacks carry the base form rather than the removed reference |
| Temporary restriction | `DisableHand` / `EnableHand`; weapon-collision toggles | A shared Boolean has no owner or nesting. One consumer can clear another's restriction; disabling a hand can also end its established hold |
| Grip adjustment and phase hooks | `SetGrabTransform`, physical-body getters, pre/post update callbacks | These do not implement a coordinated world-support grip, pose goal, drive budget or owned constraint. Directly driving returned bodies from Body Slots would recreate the competing controller that the concept avoids |

The public [API implementation](https://github.com/adamhynek/higgs/blob/93bf67b1bc4c4a11a20ccaef0d5012781d0d7eee/src/pluginapi.cpp#L250-L318) shows the pending-grab storage and shared disable flags. The [hand update](https://github.com/adamhynek/higgs/blob/93bf67b1bc4c4a11a20ccaef0d5012781d0d7eee/src/hand.cpp#L2387-L2433) can clear a grab request without acquiring the item. The [held-object path](https://github.com/adamhynek/higgs/blob/93bf67b1bc4c4a11a20ccaef0d5012781d0d7eee/src/hand.cpp#L2935-L2950) demonstrates why disabling is not possession-preserving reservation. Callbacks also do not establish commitment merely by firing: grab notification precedes the held-state assignment, and consume/stash notification precedes activation.

### Additions required in HIGGS for the agreed architecture

1. **One closed hand-state model.** HIGGS produces occupancy/availability and observation completeness, including actual game-equipment use. A null grabbed-object reference must not become proof of `Empty`; generic `Ready` is not equivalent to `CanGrabObject`. `Reserved` needs an actual resource grant. `Unknown` represents insufficient evidence and cannot detect arbitrary hidden mod mechanics.
2. **Common input and action arbitration.** Input identities, recognizers, candidates, deadlines, user priorities, selected executor and default-action coordination must operate before conflicting actions commit. Current HIGGS 001 exposes no such broker. VRIK Papyrus does expose grip/trigger press queries and durations, but reading those from Body Slots does not make HIGGS the authority or prevent duplicate actions.
3. **Owner-scoped hand/input grants and restrictions.** Acquisition/release must compose across consumers, distinguish admission restriction from dropping an item, cover affected hands together and invalidate safely on load or shutdown.
4. **Explicit confirmed hand operations.** Grab/release/transfer and consumption/equipment routes need request correlation, revalidation and truthful acceptance/commitment/completion/failure. The assigned gameplay executor still owns inventory policy and compensation; HIGGS need not implement every inventory or food rule itself.
5. **Shared physical primitives and goals.** In particular, the climbing example requires a world-support grip, while coordinated weapon support, goals and constraints require common execution. Merely announcing an external custom grip would not standardize the mechanism. Player locomotion remains outside HIGGS.
6. **Coherent observations and service lifecycle.** Pose/input/release correlation, copied records, validity/epochs and callback removal/drainage are needed for the full ABI guarantees. A shared haptic schedule is also absent from public 001; private vibration calls do not provide arbitration.

A separate cooperative adapter could coordinate only consumers that voluntarily use it. It cannot impose ownership on HIGGS's own default actions or unchanged third-party DLLs through the current API. Putting hand classification or a physical driver into Body Slots would change the architecture, not complete the agreed HIGGS service.

## Which VRIK changes are necessary, useful or avoidable

| Desired result | Assessment |
|---|---|
| New independent Body Slots slots and visuals | **No VRIK slot-creation extension is required.** Body Slots creates these itself; it does not enlarge or register entries in the native fourteen |
| Basic body/hand observation | **A public route exists, with qualification work.** Papyrus coordinates and engine nodes are observations, not the complete proposed snapshot ABI |
| Provider-guaranteed pose semantics | **An additive VRIK contract or explicit documented guarantee is needed for that guarantee.** Coherent hand/body transforms, source/space/phase, validity and invalidation would remove adapter assumptions. It is an improvement to the observation boundary, not a prerequisite for authoring slot definitions |
| General HIGGS-owned pose goals and native VRIK input participation | **Needs a supported coordination boundary for the complete concept.** Current finger-range setters and native holster callbacks do not implement all-body/hand goal negotiation or a global input grant. A restricted installation may disable competing legacy features instead |
| Safe native14 migration preserving assignments | **Not established by current public interfaces.** A verified route must capture and preserve assignments before disabling native holsters, then transfer or restore them with validated mappings. Dedicated VRIK export/import and handover APIs are one solution; their necessity is not established. Manual recreation is a reduced route |

### Existing native holsters remain a separate compatibility problem

The public 001 contract can detect reach, invoke holster actions, assign native Art, suspend a native slot and intercept holster attempts. Suspension leaves the slot detectable and is runtime-only; it is not a universal controller-input lock or a documented suppression of all hover presentation/haptics. The settings can already adjust native positions and rotations: the restriction is fourteen native entries and their behavior, not immutable native placement.

The shipped `VRIK.psc` distinguishes `displayHolsters` (hide) from `enableHolsters` (fully disable and unequip). `visibleSlotN=0` likewise does not disable interaction. Importantly, `_vrik_qust_system.psc` calls initialization when global holster enablement is toggled; when disabled, initialization replaces the fourteen-element assignment array (lines 144–178). A transition must not assume this preserves saved assignments.

Neither public 001 nor the shipped Papyrus function list provides a ready snapshot/enumeration of assigned items. Future `VRIK_Holstered` / `VRIK_Unholstered` events are useful observations, not an import of existing saves. Native suspension and settings are shared mutable state without owner-scoped leases. Therefore automatic faithful native14 replacement remains separately unqualified. New Body Slots slots need not wait for it; coexistence must still account for competing native input.

## Consequences for the concept's examples

- **Body Pouches:** additional visible bags, independent regions, saved placement and potion-only slot policies can belong to Body Slots now. Dependable cross-mod input selection and inventory-to-grip/return results require the HIGGS additions above. Existing natural HIGGS consumption can be used; a general requested consumption operation is not exposed by current 001.
- **Shields and 2H Weapons Unlocked:** slot placement and visuals do not require new VRIK holsters. Equipment/proxy identity, actual equipment mutation and confirmed two-hand-safe transfers still need the equipment provider and common HIGGS hand-operation contract.
- **Swap Drop and Hold Redux:** a slot event can supply context, but it does not solve conflicting input, explicit release or coordinated swap/transfer. Those remain HIGGS work.
- **Climbing and harvesting:** Body Slots can supply spatial regions, but current 001 does not provide the common world-support grip and ownership contract that would replace separate hand-control mechanisms. Implementing another one inside Body Slots would contradict the concept.
- **Physical Collision VR, True Wield VR and Immersive Weapon Penetration:** independent slots do not deliver their shared physical-drive/contact/NPC-endpoint requirements. Those remain the HIGGS/PLANCK part of the broader concept; PLANCK's current API was not re-audited in this scoped review.

## Recommended development boundary when implementation is resumed

Build and qualify the independently owned Body Slots registry, equipment/model binding, geometry, presentation and persistence. Keep new slots independent of native14 migration. In parallel or beforehand, add the minimal HIGGS state/input/grant and confirmed-operation service needed for the first real slot action. VRIK pose API improvements can then replace a qualified observation adapter without moving slot ownership back to VRIK.

A legacy demonstration could use current interfaces with explicitly narrower guarantees, but it must not be presented as the complete HIGGS-authoritative synergy. This review makes no implementation commitment or runtime acceptance claim.
