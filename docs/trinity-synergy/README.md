# HIGGS VRIK PLANCK PPB and Body Slots synergy concept

This directory is the single source of the cooperation concept and its two
language versions:

- [English concept](concept-en.md)
- [Русская концепция](concept-ru.md)
- [Development checkpoint — 3 November 2026](checkpoint-2026-11-03.md): current source, native/offline checks and region research.
- [Paused checkpoint — 2 November 2026](checkpoint-2026-11-02.md): saved source, native evidence and next steps.
- [Области взаимодействия и стандартизация слотов](interaction-regions-review-ru.md): исследование всех20модов, различия геометрии/действий и предложение общего контракта.
- [Implementation status](implementation-status.md): delivered code, verification and remaining gaps.
- [CommonLib assessment](commonlib-assessment.md): current technical reuse and extension candidates for the second stage.
- [Consumer requirements](consumer-contract-matrix.md): an illustrative set of VR interactions used to assess API expressiveness.
- [Практическая проверка API на 20 модах](consumer-api-review-ru.md): what each consumer can simplify and which contracts still prevent migration.
- [Wearable physics and equipment modes](wearables-and-equipment-modes.md): moving bags/scabbards, original VRIK cross-hand behavior and the original-weapon/original-shield research boundary.
- [Open upstream issues and PRs](upstream-open-work-2026-11-02.md): the 2 November review of existing proposals, fixes and additional API requirements.
- [PPB provider audit](ppb-provider-audit.md) and [proposals for telord](ppb-author-requests.md) ([русская версия](ppb-author-requests-ru.md)): source-backed integration requirements, explicit provider responsibilities and a separate, unsent discussion agenda.

Body Pouches intends to migrate to this architecture. The proposal is hosted
here because Body Pouches is maintained by its owner independently of the
framework repositories. Hosting the concept does not mean the new services are
implemented or agreed with the framework authors.

[Body Pouches 0.2.0 is released on Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/194796).
Both concepts now describe how shared services could simplify its integration
and support persistent configurable slots, independent previews and wearable
bags. Those extensions are proposals, not features of the released mod.

Edit the full concept only here and keep both languages aligned. Framework
repositories contain short summaries and their own architectural requirements,
with links to this directory; they do not maintain copies of the full concept.
The function and event declarations are illustrative pseudo-ABI, not a released SDK.

PPB supplies anatomical geometry, body fitting and touch evidence in the proposed architecture. HIGGS owns hand use, PLANCK live NPC physics, VRIK body pose and Body Slots all slot mechanics. The public PPB API already provides NPC capsule/contact observations; player-body surfaces and coordinated physical publication remain proposed extensions.
