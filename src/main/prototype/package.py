#!/usr/bin/env python3
"""Package this prototype alone; no provider DLLs, copied game assets or tests."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

import make_plugin

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(*arguments):
    return subprocess.check_output(["git", "-C", str(ROOT), *arguments], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or output == ROOT or ROOT in output.parents:
        parser.error("output must be a fresh directory outside the source repository")
    receipt = json.loads(args.build_manifest.read_text(encoding="utf-8"))
    if receipt["sourceCommit"] != git("rev-parse", "HEAD") or git("status", "--porcelain"):
        parser.error("commit source changes, then build that clean revision before packaging")
    if receipt.get("sourceDirtyAtEnd") or receipt.get("testsRun") or receipt.get("dllLoaded") or receipt.get("gameStarts"):
        parser.error("expected a clean build-only receipt for this no-testing prototype")
    for name, expected in receipt["sourceSha256"].items():
        if sha(Path(name).read_bytes()) != expected:
            parser.error("build input changed since compilation: " + name)
    dll = Path(receipt["dll"]["path"]).read_bytes()
    if sha(dll) != receipt["dll"]["sha256"]:
        parser.error("compiled DLL differs from its build receipt")
    dependencies = {name: Path(path) for name, path in receipt["dependencySources"].items()}
    files = {
        "BodyPouches.esp": make_plugin.build(),
        "SKSE/Plugins/BodyPouches.dll": dll,
        "BodyPouches2-README.txt": (SOURCE / "resources/README.txt").read_bytes(),
        "BodyPouches2-LICENSE.txt": (SOURCE / "resources/LICENSE.txt").read_bytes(),
        "BodyPouches2-THIRD-PARTY-NOTICES.txt": (SOURCE / "resources/THIRD-PARTY-NOTICES.txt").read_bytes(),
        "licenses/VRInteractionFramework-MIT.txt": (Path(receipt["frameworkRoot"]) / "LICENSE").read_bytes(),
        "licenses/HIGGS-GPL-3.0.txt": (Path(receipt["higgsRoot"]) / "LICENSE").read_bytes(),
    }
    for name, path in dependencies.items():
        license_path = Path(receipt["dependencyLicenses"][name])
        files[f"licenses/{name}.txt"] = license_path.read_bytes()
    provenance = {
        "product": receipt["product"], "version": receipt["version"],
        "sourceRepository": receipt["sourceRepository"], "sourceCommit": receipt["sourceCommit"],
        "sourceUrl": f"https://github.com/Vhodnoylogin/body-pouches/tree/{receipt['sourceCommit']}",
        "requiredFrameworkSource": receipt["frameworkCommit"], "requiredHiggsSource": receipt["higgsCommit"],
        "gameAssets": "References Skyrim.esm and Plants\\CoinBagLarge.nif from the installed game; no game assets copied.",
        "testsRun": False, "gameStarts": 0, "publication": "local prototype only",
        "files": {name: {"bytes": len(data), "sha256": sha(data)} for name, data in sorted(files.items())},
    }
    files["BodyPouches2-PROVENANCE.json"] = (json.dumps(provenance, indent=2) + "\n").encode("utf-8")
    output.mkdir(parents=True)
    archive = output / f"BodyPouches2-{receipt['version']}.zip"
    with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED) as stream:
        for name, data in sorted(files.items()):
            member = zipfile.ZipInfo(name, (2026, 11, 18, 0, 0, 0))
            member.compress_type = zipfile.ZIP_DEFLATED
            stream.writestr(member, data)
    manifest = {**provenance, "archive": {"path": str(archive), "bytes": archive.stat().st_size, "sha256": sha(archive.read_bytes())}}
    (output / "prototype-package-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest["archive"], indent=2))


if __name__ == "__main__":
    main()
