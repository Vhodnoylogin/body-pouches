# Body Pouches 0.2.0

Potions within reach in Skyrim VR. Assign a body slot with a bottle, draw the
next matching potion with your hand, drink through HIGGS, or return it to the slot.
The default pouch uses VRIK's stomach slot (13), reachable by either hand.

## Requirements and installation

- Skyrim VR.
- [SKSEVR](https://skse.silverlock.org/)
- [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
- [VRIK Player Avatar](https://www.nexusmods.com/skyrimspecialedition/mods/23416), 0.8.7
- [HIGGS](https://www.nexusmods.com/skyrimspecialedition/mods/43930)
- [SkyUI-VR](https://github.com/Odie/skyui-vr/releases) for VRIK MCM; install its listed requirements.
- Microsoft Visual C++ Redistributable x64 runtime (v14), required by the compiled DLL.

Install the archive through your mod manager and enable both the mod and
`BodyPouches.esp`. Launch through SKSEVR. The archive contains no framework DLLs
and does not replace VRIK/HIGGS or your personal VRIK configuration.
PLANCK is not required by the potion mechanic. DevBench is not required to play.
This release targets Skyrim VR; desktop SE/AE support is not claimed.

## First pouch

1. In VRIK's MCM, ensure the stomach holster (13) is enabled. VRIK requires at
   least one permitted weapon type for a slot to be detectable. Body Pouches
   claims this slot for potions in the default exclusive mode, so do not use it
   as a weapon holster at the same time.
2. Hold a drinkable potion through HIGGS. Bring the holding hand into the slot
   and release the grip inside it. Merely touching the bottle to your body
   does not assign it. VRIK's entry vibration helps locate the zone.
3. With an empty hand, close the grip inside the slot and withdraw while holding
   it. A matching potion is placed into the hand. Bring it to your mouth to
   drink using HIGGS, or release it inside the slot to return it.
4. Save normally. Keep the `.ess` and paired `.skse` file together when moving
   saves. Each save remembers its own pouch assignment.

The pouch selects from your existing inventory; it is not a separate container.
Its displayed bottle is a preview of the next matching potion, not another item.
An empty stock hides the bottle but retains the assignment.

An assignment uses all effects of its seed potion. A healing/magicka seed accepts
potions with healing **or** magicka, not only the strongest effect. Matching
potions are ordered by fewest effects, then smallest maximum effect magnitude,
then plugin/form identity. Poisons are rejected. A potion with no effects uses
its exact form instead.

## Position and settings

Use VRIK's native holster adjustment to position the slot for your body. The
bottle inherits the slot rotation. Lock holsters in VRIK after adjustment to
avoid moving them accidentally during a long grip. Do not copy another person's
calibration as if it were a universal fit.

On first launch the mod creates `SKSE/Plugins/bodypouches/bodypouches.json` in
the virtual Data directory (often your mod manager's overwrite). Defaults:

```json
{
  "language": "english",
  "logLevel": "info",
  "mayEnableSlots": false,
  "pouches": [{ "slot": 13, "mode": "exclusive" }]
}
```

Edit settings with the game closed. `mayEnableSlots` is an explicit opt-in for
enabling configured slots in memory; it defaults to false. An empty `pouches`
array disables pouches. Shared and multiple-slot configurations are advanced
options; the accepted baseline of this release is the default exclusive pouch.

## Updating and known limitations

Saves from before 0.1.21 have no recorded assignment: assign once and save again.
Do not infer an assignment from a bottle image left by an older save. Missing
forms or unsupported records leave the affected assignment empty and are logged.

The two hands can reach the native zone at different visible points. Detection
uses VRIK's hand/slot geometry, not the visible bottle's neck or surface. This
remaining ergonomic issue is under investigation. Fully leave the zone and
release the grip before a fresh draw; rapid repeat gestures may meet VRIK's
native cooldown. Body Pouches does not enlarge its native detection radius.

Owner manual testing accepted the placement, both-hand draw/return, drinking,
and distinct healing/magicka assignments through loading and complete restarts.
That does not claim exhaustive coverage of every configuration, race or headset.
