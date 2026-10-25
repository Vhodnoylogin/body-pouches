"""Build the private singleton wearable-bag demo; preserve the released record IDs.

The bag is a real inventory/equipment ARMO on biped slot 60. Its visible worn
model is supplied by the existing VRIK Art path, not an additional armor-addon
loader. The world model references Skyrim's own CoinPurseLarge resource; no game
mesh or IED binary is copied into this package. This is a native14 bridge demo.
"""
import argparse
import importlib.util
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True

spec = importlib.util.spec_from_file_location("body_pouches_released_esp", Path(__file__).with_name("make-esp.py"))
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)

BAG_ID = 0x803
BAG_BIPED_SLOT = 60
BAG_MODEL = r"Plants\CoinBagLarge.nif"
BAG_RACE = 0x00000019  # Skyrim.esm RACE DefaultRace (0x13749 is WoodElfRace).


def armor():
    return base.record("ARMO", base.SELF | BAG_ID, [
        base.sub("EDID", base.zstring("BodyPouchesDemoPotionBag")),
        base.sub("OBND", bytes(12)),
        base.sub("FULL", base.zstring("Potion Bag (Body Slots Demo)")),
        base.sub("MOD2", base.zstring(BAG_MODEL)),
        base.sub("MOD4", base.zstring(BAG_MODEL)),
        base.sub("BOD2", struct.pack("<II", 1 << (BAG_BIPED_SLOT - 30), 2)),
        base.sub("RNAM", struct.pack("<I", BAG_RACE)),
        base.sub("DESC", base.zstring("A private wearable-bag integration demonstration.")),
        base.sub("DATA", struct.pack("<If", 20, 0.5)),
        base.sub("DNAM", struct.pack("<I", 0)),
    ])


def build():
    groups = [base.group("MGEF", [base.effect()]), base.group("SPEL", [base.ability()]),
              base.group("ARTO", [base.art()]), base.group("ARMO", [armor()])]
    header = base.record("TES4", 0, [
        base.sub("HEDR", struct.pack("<fII", base.HEDR_VERSION, len(groups) * 2, BAG_ID + 1)),
        base.sub("CNAM", base.zstring("Vhodnoylogin")),
        base.sub("SNAM", base.zstring("Private Body Slots wearable-bag demonstration; native14 bridge")),
        base.sub("MAST", base.zstring(base.MASTER)), base.sub("DATA", bytes(8)),
    ])
    return header + b"".join(groups)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    # Every build writes a fresh artifact; never replace the released ESP.
    with args.output.open("xb") as stream:
        stream.write(build())
    print(f"{args.output}: singleton armor 0x{BAG_ID:03X}, biped slot {BAG_BIPED_SLOT}")


if __name__ == "__main__":
    main()
