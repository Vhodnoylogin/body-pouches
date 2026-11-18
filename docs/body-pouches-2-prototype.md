# Body Pouches 2 prototype

The prototype replaces Body Pouches' native VRIK slot integration with a consumer of VR Interaction Framework. One equippable potion bag activates one framework-owned slot on the right hip. Body Pouches retains potion acceptance, effect-based assignment, inventory selection and assignment persistence. Framework owns the slot, geometry, saved placement and presentation; HIGGS owns input arbitration, transfer into the hand, storage and drinking. Unchanged public VRIK supplies the body observation. This player bag needs no PLANCK or PPB capability.

## Scope

The bag is `BodyPouches.esp:000803`, one unique ARMO on biped slot60. The inventory/world model references Skyrim's `Plants\CoinBagLarge.nif`, the vanilla coin-purse visual identified through the owner's IED reference. Framework displays that same model while the equipment gate is satisfied. No game/IED asset, reconstructed VRIK implementation, VRIK Art ability or native holster allocation is included. A body-anchor binding starts at pelvis-local position `(18, 0, -4)`, with a radius10 access sphere. Visual positioning and ergonomics remain unqualified.

Equipping the bag enables new offers. Removing it stops new offers without deleting assignment or claiming that an already delivered potion was returned. A stored drinkable potion assigns the bag's effect filter; withdrawal selects a matching potion from ordinary player inventory. Food, poison and a game-equipped weapon are rejected. Saved placement belongs to Framework; the bag's logical assignment belongs to Body Pouches. Old native-slot assignment records are not migrated. Several copies of this armor form share the declared singleton key and do not become independent containers.

An empty-hand grip press withdraws a bottle. A trigger press with a drinkable potion at the bag stores it, assigning an unassigned bag or returning a matching bottle. The original grip release remains owned by its original action and cannot also store the bottle. Separate grip/trigger actions avoid requiring a second grab gesture. This prototype deliberately reuses HIGGS's shared executor through Framework's ordinary-inventory SDK helper instead of adding a competing drop detector or controller hook. Prototype2 supersedes the first locally assembled fresh-release prototype; neither was tested. The old native13 demo and released0.2.0 paths remain separate build choices.

## Building

Production sources are in `src/main/prototype/`. The separate CMake entry consumes only the public framework tables and its pinned SDK helper. It reuses existing pure Body Pouches potion rules and its form-description boundary, together with the completed RC4 build's CommonLib/fmt/spdlog libraries. It does not compile legacy input, VRIK holster or inventory-mutation code. Generated ESP/DLL/ZIP outputs stay outside Git.

Run `python src/main/prototype/build.py --output <external-build> --rc4-native <completed-RC4-native> --framework-root <framework-checkout> --higgs-root <HIGGS-checkout>`. The exact framework source is `88e29961747f3e28e33e1027ab93c1da9b5a278a`; the runtime RC4 implementation remains `a9b3ac9c7d1cfc3bebc05f5bb8bfbbbfe06bfd61`. HIGGS contracts are pinned to `fd226f6546877fae8b82380ce16aa800000d12ad`. The RC4 cache must identify compatible x64 Release CommonLib/fmt/spdlog libraries and their existing source directories. The script configures, compiles and links only; no test, LoadLibrary probe, deployment or game start is performed.

After committing the source, build that clean revision and run `python src/main/prototype/package.py --build-manifest <external-build>/prototype-build-manifest.json --output <new-external-package-directory>`. The runtime archive contains this prototype's DLL, generated ESP, brief instructions, provenance and required notices. Framework, HIGGS and VRIK remain separate dependencies. The ESP generator reuses the repository's maintained record encoder and creates only the bag's record; no generated game file is committed.

## Delivery state

This is a prototype source/build delivery. No tests were run at the owner's request. Native linking is not proof of SKSE initialization, worn rendering, successful transfer, comfortable input or save/load behavior. No MO2 installation, automatic-test order, Nexus upload, publication or resumed provider PR work is part of this change. Keep the released Body Pouches installation mutually exclusive with this prototype when later authorized for a game run.
