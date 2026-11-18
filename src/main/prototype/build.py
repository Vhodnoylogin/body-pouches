#!/usr/bin/env python3
"""Compile and link Body Pouches 2 only; never load, install or test the DLL."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[2]
FRAMEWORK_COMMIT = "085317c1655e5960d820bd57d5b88f546f46146f"
HIGGS_COMMIT = "0e9053fe4beb657ef51c7bf2e2bc031091d815cc"
HIGGS_HEADERS = ("higgsinterface001.h", "higgsabitypes.h", "higgshandapi.h", "higgsinput.h",
                 "higgstransactions.h", "higgsphysics.h", "higgsobservations.h", "higgsnativeinterop.h")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(root, *arguments):
    return subprocess.check_output(["git", "-C", str(root), *arguments], text=True).strip()


def source_state(root):
    return {"root": str(root), "revision": git(root, "rev-parse", "HEAD"),
            "status": git(root, "status", "--porcelain")}


def dependency_sources(native):
    cache = (native / "CMakeCache.txt").read_text(encoding="utf-8")
    return {name.lower(): Path(path) for name, path in re.findall(
        r"^FETCHCONTENT_SOURCE_DIR_(COMMONLIBSSE|FMT|SPDLOG|RAPIDCSV):PATH=(.+)$", cache, re.MULTILINE)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--rc4-native", type=Path, required=True)
    parser.add_argument("--framework-root", type=Path, required=True)
    parser.add_argument("--higgs-root", type=Path, required=True)
    parser.add_argument("--json-root", type=Path, help="Pinned nlohmann_json source (defaults to RC4 dependency cache sibling)")
    parser.add_argument("--cmake", type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if output == ROOT or ROOT in output.parents:
        parser.error("build output must be outside the source repository")
    native = args.rc4_native.resolve(strict=True)
    framework = args.framework_root.resolve(strict=True)
    higgs = args.higgs_root.resolve(strict=True)
    for label, root, expected in (("Framework", framework, FRAMEWORK_COMMIT), ("HIGGS", higgs, HIGGS_COMMIT)):
        if git(root, "rev-parse", "HEAD") != expected or git(root, "status", "--porcelain"):
            parser.error(f"{label} must be a clean checkout at {expected}")
    cmake = args.cmake or shutil.which("cmake")
    if not cmake:
        vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        installation = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-property", "installationPath"], text=True).strip()
        cmake = Path(installation) / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
    dependencies = dependency_sources(native)
    json_root = (args.json_root or dependencies["commonlibsse"].parent / "nlohmann_json-src").resolve(strict=True)
    json_include = json_root / "single_include"
    if not (json_include / "nlohmann/json.hpp").is_file():
        parser.error("--json-root must contain single_include/nlohmann/json.hpp")
    dependencies["nlohmann_json"] = json_root
    licenses = {name: path / ("LICENSE.MIT" if name == "nlohmann_json" else "LICENSE")
                for name, path in dependencies.items()}
    source_roots = {"bodyPouches": ROOT, "framework": framework, "higgs": higgs}
    source_states = {name: source_state(root) for name, root in source_roots.items()}
    source_commit = source_states["bodyPouches"]["revision"]
    inputs = [p for p in SOURCE.rglob("*") if p.is_file() and "__pycache__" not in p.parts]
    inputs += list((ROOT / "src/core").glob("*.cpp")) + list((ROOT / "src/core").glob("*.h"))
    inputs += [ROOT / "src/game/Forms.cpp", ROOT / "src/game/Forms.h", ROOT / "tools/make-esp.py"]
    inputs += list((framework / "include").glob("*.h"))
    inputs += [framework / "examples/ordinary_inventory_provider.h", framework / "examples/ordinary_inventory_provider.cpp"]
    inputs += [higgs / "include" / name for name in HIGGS_HEADERS]
    inputs += [native / "CMakeCache.txt"]
    inputs += [native / f"_deps/{directory}/Release/{library}.lib" for directory, library in (
        ("commonlibsse-build", "CommonLibSSE"), ("fmt-build", "fmt"), ("spdlog-build", "spdlog"))]
    inputs += [framework / "LICENSE", higgs / "LICENSE"]
    inputs += list(licenses.values()) + [json_include / "nlohmann/json.hpp"]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    output.mkdir(parents=True, exist_ok=True)
    commands = [
        [str(cmake), "-S", str(SOURCE), "-B", str(output / "native"), "-G", "Visual Studio 18 2026", "-A", "x64",
         f"-DRC4_NATIVE={native}", f"-DFRAMEWORK_ROOT={framework}", f"-DHIGGS_ROOT={higgs}", f"-DJSON_INCLUDE={json_include}"],
        [str(cmake), "--build", str(output / "native"), "--config", "Release", "--target", "BodyPouches2Prototype", "--parallel", "4"],
    ]
    executions = []
    for label, command in zip(("configure", "build"), commands):
        completed = subprocess.run(command, capture_output=True, timeout=1200)
        log = output / f"{label}.log"
        log.write_bytes(completed.stdout + completed.stderr)
        executions.append({"name": label, "argv": command, "exitCode": completed.returncode,
                           "log": {"path": str(log), "sha256": sha(log)}})
        if completed.returncode:
            raise RuntimeError(f"{label} failed; read {output / (label + '.log')}")
    for path, expected in pins.items():
        if sha(Path(path)) != expected:
            raise RuntimeError(f"build input changed: {path}")
    if source_states != {name: source_state(root) for name, root in source_roots.items()}:
        raise RuntimeError("source revision or working-tree state changed during compilation")
    dll = output / "native/Release/BodyPouches.dll"
    receipt = {
        "product": "Body Pouches 2", "version": "2.0.0-prototype.8",
        "sourceRepository": git(ROOT, "remote", "get-url", "origin"), "sourceCommit": source_commit,
        "sourceDirtyAtEnd": bool(git(ROOT, "status", "--porcelain")),
        "frameworkRoot": str(framework), "frameworkCommit": FRAMEWORK_COMMIT,
        "higgsRoot": str(higgs), "higgsCommit": HIGGS_COMMIT,
        "dependencySources": {name: str(path) for name, path in dependencies.items()},
        "dependencyLicenses": {name: str(path) for name, path in licenses.items()},
        "sourceSha256": pins, "commands": executions,
        "sourceStates": source_states,
        "dll": {"path": str(dll), "bytes": dll.stat().st_size, "sha256": sha(dll)},
        "testsRun": False, "gameStarts": 0, "dllLoaded": False,
        "qualification": "Compiled and linked only. Owner requested no testing; gameplay is unqualified.",
    }
    (output / "prototype-build-manifest.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt["dll"], indent=2))


if __name__ == "__main__":
    main()
