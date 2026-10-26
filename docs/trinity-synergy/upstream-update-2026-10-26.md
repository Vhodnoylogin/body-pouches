# VRIK 0.8.8 and current PLANCK Git: effect on the cooperation work

Reviewed on 26 October 2026. The [concept](concept-en.md) is unchanged; this assessment explains how new upstream work affects the existing implementations and the [short proposal for Shizof](vrik-integration-proposal-en.md).

**Conclusion:** neither update makes Body Slots obsolete or introduces an observed public-ABI break for the pouch. The VRIK reconstruction still needs a focused 0.8.8 baseline migration; PLANCK needs integration of its latest source fixes while retaining our additional repairs. This review performs neither migration and establishes no new game compatibility.

## What was compared

| Component | Preserved input | New input |
|---|---|---|
| VRIK | Downloaded 0.8.7 archive; our reconstructed feature at `0ef71aab5adc348acf8ffff500453ed1bf299319` | Downloaded main-file 0.8.8 archive, including its matching DLL and PDB |
| PLANCK | Upstream baseline `cb54ad37a867c84d7e7cdbd48df0ca0e7fdeb7cd`; our development `cb78ef4399549a84c531c42704585aaab0062845` | Author's current Git `master` at `aed524a5e398f04f10f8f95469153362a88c3681`, source version 0.8.2 |

The PLANCK tag `v0.8.2` points to the earlier `371764728a10a299bbacd93db8ce45ab281059fb`, before the later fixes described below. No downloaded PLANCK release binary was inspected; those later changes are claims about current Git source.

## VRIK: a limited update, with an important input-layout change

Of twenty archive files, four changed: `VRIK.dll`, `VRIK.pdb`, and the source/compiled MCM script. No files were added or removed. The ESP, models, core Papyrus scripts and default INI files are byte-identical. The MCM diff adds Steam Frame handling and Axis3/Axis4 gesture choices. The [author's changelog](https://www.nexusmods.com/skyrimspecialedition/mods/23416) also describes support for externally supplied finger ranges on controllers without touch support; targeted binary inspection is consistent with that change.

The shipped PDBs record checksums for 27 project source files. Six recorded files changed: `main.cpp`, `vrikbody.cpp`, `vrikgestures.cpp`, `vrikinput.cpp`, `vriksettings.cpp` and `vriksettings.h`. Recorded checksums for the slot implementation and several interface implementation files are unchanged. Not every header has a checksum record, so this is not a complete list of every possible source edit.

Independently of those checksums, the complete **28-method `IVrikInterface001`** retains its resolved signatures, virtual offsets and actual vtable target names. Reach, native suspension, the holster-attempt callback and Art methods used by Body Pouches retain their positions. Selected class sizes also remain unchanged. This supports continued use of that binary interface; it does not prove complete semantic or runtime equivalence. Some compiled native-slot code differs despite unchanged recorded source, so the whole slot binary must not be described as identical.

The API's `getBuildNumber()` still returns **80700 in both releases**. The separate SKSE plugin version changes from `0x870` to `0x880`. Body Pouches already requires a minimum of 80700, so it does not reject 0.8.8. Requiring 80800 would incorrectly reject this actual binary.

The reconstructed input model does need attention. The PDB adds `SteamFrame=5` to the controller enum and two input entries, `Axis3=6` and `Axis4=7`, increasing `Input_Max` from six to eight. The two-hand input bank grows from 480 to 640 bytes; each press/touch enable bank grows from 36 to 48 bytes. A port must update enum values, array dimensions, strides, iteration bounds and inlined consumers together, plus the affected input, gesture, hand and MCM behavior. Simply changing a version number or copying one new controller case would be insufficient.

Full decompilation is unnecessary for this comparison. Archive hashes, PDB types/checksums, the actual vtable and bounded instruction inspection provide the required evidence. Updating the reconstruction would require targeted work on the affected areas and fresh regression evidence. The current private VRIK MRs remain explicitly **0.8.7-based**, rather than patches already ported to 0.8.8.

## PLANCK: upstream stability fixes complement our repairs

Against our old baseline, current Git changes three files: `include/main.h`, `include/version.h` and `src/main.cpp` (73 additions, 46 deletions). Public interface001, its getter source and the API implementation remain exact Git blobs.

- [Furniture cleanup](https://github.com/adamhynek/activeragdoll/commit/428e0b2539c919c1d57f639d1803e91c057fbdff) clears an animation graph's world on removal only when the actor is using furniture. An internal tracking flag is removed.
- [Ragdoll addition](https://github.com/adamhynek/activeragdoll/commit/77b6a69a8462feb00ee0b27023c23dbd85f2d334) consistently takes the animation-graph update lock before the Havok world lock.
- [Collision manifold updates](https://github.com/adamhynek/activeragdoll/commit/aed524a5e398f04f10f8f95469153362a88c3681) avoid the reverse lock order by using an actor-indexed view of the existing tracked ragdolls. Addition, removal and reset maintain that index.

These are upstream-provided fixes and should not be offered to the author as missing work. The new internal index is not the proposed public actor-endpoint or lifetime API.

Our additional movement/look-at reset and locked-read repairs, nested-hit context restoration, and reset fence for deferred cleanup are not implemented by these commits. They remain relevant. A read-only mechanical merge of our development and current upstream succeeds, preserving both sets of changes. That is evidence of a clean text merge, not a tested integration. Both existing fake-engine fixture templates would need the new ragdoll index and reset assertions before compiling the newly merged `ResetObjects`; the fixtures do not qualify the new engine lock ordering.

Upstream history was rewritten: the old constraint-restoration patch was replayed under another commit ID. Matching stable patch IDs show the patch was retained. Integrating current Git should preserve this history fact instead of treating it as a lost fix or discarding our work.

## Consequences for the current goals

| Work | Effect and next required evidence |
|---|---|
| Standalone Body Slots | Definitions, instances, saved configuration and activation have no direct dependency on PLANCK or VRIK's private input layout. No service rewrite is indicated by these updates |
| Released Body Pouches 0.2.0 | No observed 001 ABI break requires an immediate rebuild. Existing gameplay acceptance belongs to the previous runtime; 0.8.8 interaction still needs checking |
| Private wearable demo | Its native13 callback/Art path remains structurally available. Test the prepared archive against released 0.8.8: actual equipment, visible bag, both hands, inventory changes, unequip/re-equip and save/load. It remains offline checked, not game accepted |
| Shared-pouch input and new hardware | Legacy direct Grip/Trigger polling is separate from VRIK's new controller support. Do not advertise Steam Frame compatibility without an actual hardware/input check |
| Private VRIK/Body Slots review | Preserve the existing exact trees and history; identify the VRIK side as 0.8.7-based. Migrate the input/hand baseline before presenting a replacement VRIK DLL as up to date |
| PLANCK development | Integrate latest Git source, retain our bounded repairs, adapt fixtures, then repeat source/native/runtime qualification as appropriate. The old 0.8.2 tag alone omits later source fixes |
| CommonLib | Existing engine wrappers and consistent graph/world lock ordering remain second-stage candidates. Tracking ownership and gameplay decisions remain in the mods; no library change was made |

The owner deferred broader HIGGS/PLANCK implementation pending author feedback. This assessment does not resume those goals. No source branch was merged or rebased, no DLL was loaded or rebuilt, and no mod was installed or tested in Skyrim. The public concept remains unchanged; the next author discussion concerns the responsibility split and the preferred small VRIK integration boundary.

Archive provenance: 0.8.7 SHA-256 `3ccc4780cd1d9218a0a11500a75e46b8312ed431568493bf18c7e2d3881c28bd`; 0.8.8 SHA-256 `62f592b4fa1f83e8c8a71edc74cf4d317cb71ae2658386d16c2c0167b92117db`. Private raw debug/disassembly evidence remains outside this public repository.
