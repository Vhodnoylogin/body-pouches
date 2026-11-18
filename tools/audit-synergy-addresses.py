"""Prepare an evidence-only CommonLib migration inventory; changes no providers.

Requires pefile. Source bindings, Address Library matches, CommonLib callsites,
PE sections and unwind ranges are different evidence levels, never interchangeable.
The tool does not decrypt code, execute Skyrim, or infer unknown signatures.
"""
import argparse
import bisect
import collections
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
import pefile

RELOC = re.compile(r"\b(RelocAddr|RelocPtr)\s*<([^;{}\n]+?)>\s+([A-Za-z_]\w*)\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def head(path):
    return subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()


def source_state(path):
    changes = subprocess.check_output(["git", "-C", str(path), "status", "--porcelain", "--untracked-files=all"], text=True).splitlines()
    return {"head": head(path), "dirty": bool(changes), "workingTreeChanges": changes}


def library_index(root, by_id):
    index = collections.defaultdict(list)
    for folder in ("include", "src"):
        for path in sorted((root / folder).rglob("*")):
            if not path.is_file() or path.suffix not in (".h", ".cpp"):
                continue
            source = path.read_text(encoding="utf-8", errors="replace")
            # First argument is the SE id. Its database row supplies a candidate
            # VR mapping; status and source provenance are retained separately.
            for match in re.finditer(r"(?:RELOCATION_ID|REL::ID|REL::VariantID)\s*[({]\s*(\d+)", source):
                row = by_id.get(int(match.group(1)))
                if row is not None:
                    index[row["rva"]].append({"path": path.relative_to(root).as_posix(), "line": source.count("\n", 0, match.start()) + 1, "id": row["id"]})
            for match in re.finditer(r"(?:REL::VariantID|RELOCATION_OFFSET)\s*[({]\s*[^,)]+,\s*[^,)]+,\s*(0x[0-9a-fA-F]+)\s*[)}]", source):
                index[int(match.group(1), 16)].append({"path": path.relative_to(root).as_posix(), "line": source.count("\n", 0, match.start()) + 1, "directVR": True})
    return index


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--higgs", type=Path, required=True)
    parser.add_argument("--planck", type=Path, required=True)
    parser.add_argument("--commonlib", type=Path, required=True)
    parser.add_argument("--address-library", type=Path, required=True, help="Pinned VR Address Library checkout with database.csv")
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    db = args.address_library / "database.csv"
    rows = []
    for row in csv.DictReader(db.open(encoding="utf-8-sig", newline="")):
        try:
            absolute = int(row["vr"], 16)
            if absolute:
                rows.append({"id": int(row["id"]), "rva": absolute - 0x140000000, "status": row.get("status", ""), "name": row.get("name", "")})
        except (ValueError, TypeError):
            continue
    by_rva = collections.defaultdict(list)
    for row in rows:
        by_rva[row["rva"]].append(row)
    common = library_index(args.commonlib, {row["id"]: row for row in rows})
    pe = pefile.PE(str(args.image), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXCEPTION"]])
    functions = [(entry.struct.BeginAddress, entry.struct.EndAddress) for entry in pe.DIRECTORY_ENTRY_EXCEPTION]
    starts = [pair[0] for pair in functions]
    bindings = []
    pins = {}
    unparsed = []
    for provider, root in (("HIGGS", args.higgs), ("PLANCK", args.planck)):
        header_paths = sorted((root / "include").rglob("*.h"))
        headers = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in header_paths)
        for path in header_paths:
            pins[provider + "/" + path.relative_to(root).as_posix()] = sha(path)
        typedefs = {match.group(1): " ".join(match.group(0).split()) for match in re.finditer(r"typedef\s+[^;{}]*?\(\s*\*\s*(_\w+)\s*\)\s*\([^;{}]*?\)\s*;", headers)}
        for path in sorted((root / "src").rglob("*.cpp")):
            source = path.read_text(encoding="utf-8", errors="replace")
            # Normalize only the equivalent auto-initializer spelling. Line
            # boundaries are unchanged, so citations still address original files.
            normalized = re.sub(r"\bauto\s+(\w+)\s*=\s*(RelocAddr|RelocPtr)\s*<([^;\n]+?)>\s*\(",r"\2<\3> \1(",source)
            matches = [match for match in RELOC.finditer(normalized) if not normalized[normalized.rfind("\n",0,match.start())+1:match.start()].lstrip().startswith("//")]
            if not matches:
                continue
            relative = path.relative_to(root).as_posix()
            pins[provider + "/" + relative] = sha(path)
            for match in matches:
                rva = int(match.group(4), 16)
                line = normalized.count("\n", 0, match.start()) + 1
                section = pe.get_section_by_rva(rva)
                at = bisect.bisect_right(starts, rva) - 1
                unwind = functions[at] if at >= 0 and functions[at][0] <= rva < functions[at][1] else None
                type_name = match.group(2).strip()
                signature = typedefs.get(type_name)
                if signature is None and type_name == "Fn":
                    local = list(re.finditer(r"using\s+Fn\s*=\s*([^;]+);", normalized[:match.start()]))
                    if local:
                        signature = " ".join(local[-1].group(1).split())
                sites = common.get(rva, [])
                wrapper_sites = [site for site in sites if not site["path"].startswith("include/RE/Offsets_")]
                classification = "existing-wrapper-candidate" if wrapper_sites else "library-metadata-only" if sites else "extension-or-further-audit"
                bindings.append({"provider": provider, "path": relative, "line": line, "symbol": match.group(3), "relocation": match.group(1), "type": type_name, "signature": signature, "rva": f"0x{rva:X}", "imageSection": section.Name.decode().rstrip("\0") if section else None, "executableSection": bool(section and section.Characteristics & 0x20000000), "unwindRange": {"begin": f"0x{unwind[0]:X}", "end": f"0x{unwind[1]:X}", "atEntry": unwind[0] == rva} if unwind else None, "addressLibraryCandidates": by_rva.get(rva, []), "commonlibSites": sites, "migrationClass": classification, "instructionVerified": False, "runtimeVerified": False})
            # A completeness signal: keep nonliteral/arithmetic declarations for
            # explicit audit rather than treating this inventory as exhaustive.
            represented = {normalized.count("\n", 0, m.start()) + 1 for m in matches}
            for number, text in enumerate(source.splitlines(), 1):
                if re.search(r"\bReloc(?:Addr|Ptr)\s*<", text) and number not in represented and "extern " not in text and not text.lstrip().startswith("//"):
                    unparsed.append({"provider": provider, "path": relative, "line": number, "declaration": text.strip()})
    groups = collections.defaultdict(list)
    for item in bindings:
        groups[item["rva"]].append({"provider": item["provider"], "symbol": item["symbol"], "type": item["type"], "signature": item["signature"]})
    shared = {rva: items for rva, items in groups.items() if len({item["provider"] for item in items}) > 1}
    result = {"schemaVersion": 1, "purpose": "CommonLib stage2 preparation only; no library implementation", "sources": {"HIGGS": head(args.higgs), "PLANCK": head(args.planck), "CommonLib": head(args.commonlib), "AddressLibrary": head(args.address_library)}, "inputHashes": {"image": sha(args.image), "addressDatabase": sha(db)}, "image": {"preferredBase": hex(pe.OPTIONAL_HEADER.ImageBase), "sections": [{"name": s.Name.decode().rstrip("\0"), "rva": hex(s.VirtualAddress), "entropy": round(s.get_entropy(), 3)} for s in pe.sections]}, "limits": ["Source symbol/signature is not runtime ABI qualification", "Address Library confidence is retained, not upgraded", "CommonLib metadata is distinct from a reusable wrapper", "Encrypted on-disk text prevents honest instruction verification", "Raw hook literals and vtable slot invocations need separate review"], "sourceSha256": pins, "counts": dict(collections.Counter(item["migrationClass"] for item in bindings)), "bindings": bindings, "sharedAddresses": shared, "unparsedDeclarations": unparsed}
    result["sourceStates"] = {name: source_state(root) for name, root in (("HIGGS", args.higgs), ("PLANCK", args.planck), ("CommonLib", args.commonlib), ("AddressLibrary", args.address_library))}
    result["limits"].append("A dirty provider checkout is identified explicitly; its file hashes, not HEAD alone, identify the inspected source")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"bindings": len(bindings), "uniqueAddresses": len(groups), "sharedAddresses": len(shared), "unparsed": len(unparsed), "classes": result["counts"]}))


if __name__ == "__main__":
    main()
