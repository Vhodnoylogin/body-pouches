# Open upstream work — HIGGS and PLANCK

Read-only assessment on **2 November 2026**. Complete open-item collections and discussions were read; counts and the three item states were rechecked at **18:25 UTC**. Scope covers GitHub issues/PRs in the authors' repositories and our forks, not Nexus discussions or closed historical work. The GitHub connector was sufficient; no permission fallback was needed.

| Repository | Open issues | Open PRs |
|---|---:|---:|
| [adamhynek/higgs](https://github.com/adamhynek/higgs) | 1 | 1 |
| [Vhodnoylogin/higgs](https://github.com/Vhodnoylogin/higgs) | 0 | 0 |
| [adamhynek/activeragdoll](https://github.com/adamhynek/activeragdoll) | 1 | 0 |
| [Vhodnoylogin/activeragdoll](https://github.com/Vhodnoylogin/activeragdoll) | 0 | 0 |

Both forks have issue tracking disabled. Empty fork queues do not establish absence of defects. All three upstream items have no discussion replies; PR6 also has no review submissions/threads or returned head-commit statuses/check runs. Silence is not upstream approval.

## HIGGS issue7: body zones and input ownership

[Issue7 — public API for interaction zones on the body](https://github.com/adamhynek/higgs/issues/7), opened by Vhodnoylogin on **15 September 2026**, remains open. It requests body-anchored zones, content operations, enter/exit/trigger notifications, shared handover and grip ownership. This is our earlier requirements proposal, not independent third-party endorsement. There is no implementation patch to merge.

The input/handover intent overlaps the current [HIGGS hand contracts](https://github.com/Vhodnoylogin/higgs/blob/e716f8bb9a7c2f7a5d4740866362e5edcb37715a/include/higgshandapi.h), sessions, haptics and typed receipts. Its proposed ownership is outdated: zone/content registration belongs to **Body Slots**, while HIGGS owns hand state/use, input and physical possession; VRIK owns body/virtual poses, PLANCK NPC physics and PPB anatomy. Reading posed game nodes after VRIK is useful, but is not itself a VRIK virtual-pose contract or permission for another skeletal driver.

**Recommendation:** retain it as a requirements discussion. Any separately authorized follow-up should present the [current role split](concept-en.md#responsibilities) and copied lifecycle contracts, rather than adding another zone registry or raw-pointer API to HIGGS. Do not close it as implemented upstream; the [implementation ledger](implementation-status.md) distinguishes delivered source from remaining work and acceptance.

## HIGGS PR6: CustomMenu compatibility

[PR6 — Add CustomMenu & Mark it as game stopping](https://github.com/adamhynek/higgs/pull/6), by mikastamm, has been open since **25 January 2026**, non-draft and unmerged. Its complete [diff](https://github.com/adamhynek/higgs/pull/6/files) adds exactly two entries in `src/menu_checker.cpp`: track `CustomMenu` and include it among game-stopping menus. Head commit is [`2edd54f`](https://github.com/mikastamm/higgs/commit/2edd54f90159dd4a0a442cf470b19f301414868c); GitHub reports clean mergeability. No test or CI evidence accompanies it.

This remains an independent uncovered compatibility fix: `CustomMenu` is absent in current upstream and our pinned HIGGS source. A non-mutating patch check succeeded against our checkout; no textual conflict with the ABI/native changes was found.

The new broker adds a semantic caveat. [Controller input is masked before the menu early return](https://github.com/Vhodnoylogin/higgs/blob/e716f8bb9a7c2f7a5d4740866362e5edcb37715a/src/hand.cpp#L4189), and [active grants/consumed sequences are masked again](https://github.com/Vhodnoylogin/higgs/blob/e716f8bb9a7c2f7a5d4740866362e5edcb37715a/src/hand_broker.cpp#L910). Menu observation does not itself release a grant. Consequently, these two lines can help legacy held-input handling without proving UI exit controls work during broker-owned uses. Blanket bypass could instead leak a consumed trigger back to gameplay. This is source-derived, not a game reproduction.

**Recommendation:** consider a separate compatibility patch after UI Extensions open/close tests with one/both hands occupied, active grants and matching Down/Up sequences. Qualify paused and non-pausing custom menus rather than assuming all instances behave alike. Do not describe this PR as complete broker/menu routing support.

## PLANCK issue2: cosmetic sword/shield contact

[Issue2 — sword clang against the player's own shield](https://github.com/adamhynek/activeragdoll/issues/2), by Zod1234, has been open since **11 December 2024**, with no author reply. It asks for gentle own-equipment impacts, separate from damage, and whether relative-speed settings can enable them. There is no pending PLANCK PR or patch to import.

The obstacle precedes speed thresholds. PLANCK [disables contact between two nonmoveable/keyframed HIGGS hand/weapon bodies, and skips held-object-pair hit processing](https://github.com/Vhodnoylogin/activeragdoll/blob/87d7f2e5054953d55a43cd074fe526ee82d5568e/src/main.cpp#L2190). [Contact-point relative velocity and combat thresholds occur later](https://github.com/Vhodnoylogin/activeragdoll/blob/87d7f2e5054953d55a43cd074fe526ee82d5568e/src/main.cpp#L2296). Lowering global hit thresholds cannot restore an earlier filtered contact and would affect unrelated combat.

Geometry fitting, endpoint/drive policies and hand-part attribution do not solve that admission rule. HIGGS's [contact sampling](https://github.com/Vhodnoylogin/higgs/blob/e716f8bb9a7c2f7a5d4740866362e5edcb37715a/src/hand_observations_native.cpp#L44) and coordinated haptics are foundations, but do not guarantee final contact response. The current contact record gives body linear velocities, not both contact-point velocities or complete bilateral equipped-item/material semantics; rotational impacts need more information.

**Recommendation:** treat this as a useful additional API example, not a demonstrated release blocker. Explore a small opt-in pair/contact-response contract coordinated by HIGGS before PLANCK's suppression, separating cosmetic evidence/audio/haptics, optional physical response and combat damage. Preserve current combat defaults. Validate equipped versus held items, handedness, secondary grips/fingers, taps versus rubbing, angular motion, duplicate sounds/haptics and lifecycle retirement. Keyframed contacts alone cannot promise controller resistance.

No outreach, merge, patch application, installation or game test was performed. This review does not claim author agreement or game qualification; code/build evidence remains separate from runtime acceptance.
