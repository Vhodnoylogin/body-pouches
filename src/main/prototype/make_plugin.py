#!/usr/bin/env python3
"""Compile the prototype's one equippable bag; no VRIK spell/art or native slot.

The vanilla coin purse is referenced, not redistributed. VR Interaction Framework
renders the worn holder independently of the ARMO's inventory/world model.
"""
import argparse
import importlib.util
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("body_pouches_esp_records", ROOT / "tools/make-esp.py")
records = importlib.util.module_from_spec(spec)
spec.loader.exec_module(records)

BAG_LOCAL_ID = 0x803
BIPED_SLOT = 60
MODEL = r"Plants\CoinBagLarge.nif"


def build():
    armor = records.record("ARMO", records.SELF | BAG_LOCAL_ID, [
        records.sub("EDID", records.zstring("BodyPouches2PotionBag")),
        records.sub("OBND", bytes(12)),
        records.sub("FULL", records.zstring("Potion Bag (Body Pouches 2)")),
        records.sub("MOD2", records.zstring(MODEL)),
        records.sub("MOD4", records.zstring(MODEL)),
        records.sub("BOD2", struct.pack("<II", 1 << (BIPED_SLOT - 30), 2)),
        records.sub("RNAM", struct.pack("<I", 0x19)),  # DefaultRace in Skyrim.esm
        records.sub("DESC", records.zstring("An experimental wearable potion pouch powered by VR Interaction Framework.")),
        records.sub("DATA", struct.pack("<If", 20, 0.5)),
        records.sub("DNAM", struct.pack("<I", 0)),
    ])
    header = records.record("TES4", 0, [
        records.sub("HEDR", struct.pack("<fII", records.HEDR_VERSION, 2, BAG_LOCAL_ID + 1)),
        records.sub("CNAM", records.zstring("Vhodnoylogin")),
        records.sub("SNAM", records.zstring("Body Pouches 2 prototype; one framework-owned potion bag")),
        records.sub("MAST", records.zstring(records.MASTER)),
        records.sub("DATA", bytes(8)),
    ])
    return header + records.group("ARMO", [armor])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    with args.output.open("xb") as output:
        output.write(build())
    print(f"Created {args.output}; BodyPouches.esp:{BAG_LOCAL_ID:06X}, slot {BIPED_SLOT}")


if __name__ == "__main__":
    main()
