# Body Pouches — 0.2.0

Potions within reach in Skyrim VR. Turn a VRIK body slot into a potion pouch:
draw a bottle with either hand, drink it through HIGGS, or return it to the slot.
No inventory menu, wheel or separate hotkey is needed for the pouch interaction.
Release archives contain the runtime files and required license notices only;
installation instructions are on the Nexus page and retained here in the source.

Russian: [README.ru.md](README.ru.md).

Author: [Vhodnoylogin](https://github.com/Vhodnoylogin).
Developed with assistance from Claude (Anthropic) and
[Codex (OpenAI)](https://github.com/codex).
Codex assisted with implementation, code review, verification, and release preparation.

## Features

- One exclusive pouch on the stomach slot (13) by default.
- A visible preview of the next matching potion.
- Hand-based assignment, drawing and return.
- Selection from your existing inventory by potion effects, including multi-effect potions.
- Separate assignments for each save, preserved through loading and complete restarts.
- Poison rejection; the pouch does not create a separate stock of items.

## Requirements and quick start

For Skyrim VR. Install the dependencies below, enable `BodyPouches.esp` and launch through SKSEVR. DevBench is not required to play.

- [SKSEVR](https://skse.silverlock.org/)
- [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
- [VRIK Player Avatar](https://www.nexusmods.com/skyrimspecialedition/mods/23416), 0.8.7
- [HIGGS](https://www.nexusmods.com/skyrimspecialedition/mods/43930)
- [SkyUI-VR](https://github.com/Odie/skyui-vr/releases) for VRIK MCM; install its listed requirements.
- Microsoft Visual C++ Redistributable x64 runtime (v14), required by the compiled DLL.


Enable VRIK stomach holster13, take a drinkable potion in hand and release it
inside that slot to assign the pouch. To draw, close the grip with an empty hand
inside the zone and withdraw while keeping it held. Bring the bottle to your mouth
to drink through HIGGS, or release inside the slot to return it. Save normally;
keep the ESS/SKSE pair together. Older saves without an assignment need one new
assignment and save.

See the [complete quick start](docs/quick-start.en.md) for configuration,
multi-effect selection, VRIK positioning and updating.

## Known limitations

The accepted baseline is the default exclusive potion pouch. The two hands can
reach its native activation zone at different visible points; the visible neck
is not the detection centre. This remaining ergonomic issue is under investigation.
Shared/multiple-slot configurations are advanced options with narrower acceptance.
Owner manual testing accepted placement, both-hand draw/return, drinking and
independent healing/magicka assignments after loading and full restarts.

## Credits and source

Thanks to the authors of VRIK, HIGGS, SKSEVR and CommonLibSSE-NG for the functionality
that makes this mod possible. Third-party integration notices are included in the archive.

Source: https://github.com/Vhodnoylogin/body-pouches (main; one mod at repository root).
[Changelog](CHANGELOG.md). Historical development notes are preserved separately in the source repository.

## Building from source

```text
cmake -S . -B build-core -DBODYPOUCHES_BUILD_PLUGIN=OFF
cmake --build build-core --config Release
cmake -S . -B build
cmake --build build --config Release --target BodyPouches
```

Release builds omit the DevBench inspection bridge. For development testing,
configure with `-DBODYPOUCHES_WITH_DEVBENCH=ON`; that build includes the API glue
and requires its license notice when distributed. The default is `OFF`.

## Proposed framework cooperation

The maintained [English concept](docs/trinity-synergy/concept-en.md), [Russian concept](docs/trinity-synergy/concept-ru.md) and [implementation ledger](docs/trinity-synergy/implementation-status.md) describe cooperation between HIGGS, VRIK, PLANCK and the separate Body Slots service. Body Pouches intends to adopt these services; their implementation and verification limits are recorded separately from the released mod.

For Body Slots' purpose—new slot types and responses, a dynamic slot set and managed replacements for the native fourteen—start with the illustrated [proposal for Shizof](docs/trinity-synergy/vrik-integration-proposal-en.md) ([Russian](docs/trinity-synergy/vrik-integration-proposal-ru.md)). It separates the target design from the current prototype.
