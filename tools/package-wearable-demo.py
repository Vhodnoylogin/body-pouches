"""Build and package the private singleton demo outside Git; never install or publish."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def git(path, *args):
    return subprocess.check_output(['git', '-C', str(path), *args], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--body-slots-source', type=Path, required=True)
    parser.add_argument('--body-slots-dll', type=Path, required=True)
    parser.add_argument('--body-slots-sha256', required=True)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or any((p / '.git').exists() for p in (output, *output.parents)):
        parser.error('output must be a new directory outside Git checkouts')
    sdk = args.body_slots_source.resolve(strict=True)
    provider = args.body_slots_dll.resolve(strict=True)
    if sha(provider) != args.body_slots_sha256.lower():
        parser.error('Body Slots DLL does not match the explicitly selected hash')
    if git(ROOT, 'status', '--porcelain') or git(sdk, 'status', '--porcelain'):
        parser.error('commit source changes before making a review package')
    source_commit = git(ROOT, 'rev-parse', 'HEAD')
    provider_commit = git(sdk, 'rev-parse', 'HEAD')
    build = args.build.resolve(strict=True)
    cache = (build / 'CMakeCache.txt').read_text(encoding='utf-8')
    settings = dict(line.split('=', 1) for line in cache.splitlines() if '=' in line and not line.startswith('//'))
    if Path(settings.get('CMAKE_HOME_DIRECTORY:INTERNAL', '')).resolve() != ROOT:
        parser.error('build belongs to a different source checkout')
    if settings.get('BODYPOUCHES_WEARABLE_DEMO:BOOL') != 'ON':
        parser.error('the selected build is not the wearable demo')
    inspect = settings.get('BODYPOUCHES_WITH_DEVBENCH:BOOL') == 'ON'
    output.mkdir(parents=True)
    command = [args.cmake, '--build', str(build), '--config', 'Release', '--target', 'BodyPouches', '--parallel', '4']
    result = subprocess.run(command, capture_output=True, timeout=300)
    (output / 'build.log').write_bytes(result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError('demo build failed; see build.log')
    if git(ROOT, 'status', '--porcelain') or git(sdk, 'status', '--porcelain') or git(ROOT, 'rev-parse', 'HEAD') != source_commit or git(sdk, 'rev-parse', 'HEAD') != provider_commit:
        raise RuntimeError('source changed during packaging build')
    author = module('body_slots_demo_author', sdk / 'tools/author.py')
    esp = module('body_pouches_demo_esp', ROOT / 'tools/make-wearable-demo.py')
    template = ROOT / 'demo/potion-bag-template.json'
    files = {
        'BodyPouches.esp': esp.build(),
        'SKSE/Plugins/BodyPouches.dll': (build / 'Release/BodyPouches.dll').read_bytes(),
        'SKSE/Plugins/BodySlots.dll': provider.read_bytes(),
        'SKSE/Plugins/BodySlots/body-pouches-demo.bslots': author.compile_project(author.load_project(template)),
        'README-PRIVATE-DEMO.md': (ROOT / 'docs/wearable-demo.md').read_bytes(),
        'OpenVR.LICENSE.txt': (ROOT / 'contract/OpenVR.LICENSE.txt').read_bytes(),
        'BodySlots-PRIVATE-STATUS.txt': b'Unpublished foundation. Licensing remains to be finalized before publication. Private review only; no game qualification or public redistribution grant is implied.\n',
    }
    if inspect:
        files['DevBenchAPI.LICENSE.txt'] = (ROOT / 'contract/DevBenchAPI.LICENSE.txt').read_bytes()
    manifest = {
        'label': 'Body Pouches private wearable-bag demo 01', 'publication': 'private review only',
        'sourceRepository': git(ROOT, 'remote', 'get-url', 'origin'), 'sourceCommit': source_commit,
        'bodySlotsRepository': git(sdk, 'remote', 'get-url', 'origin'), 'bodySlotsSourceCommit': provider_commit,
        'bodySlotsDllSha256': args.body_slots_sha256.lower(), 'buildCommand': command,
        'wearableDemo': True, 'devBenchObservation': inspect, 'nativeVrikSlot': 13, 'equipmentSlot': 60,
        'templateSha256': sha(template), 'gameStarts': 0, 'gameAcceptance': 'pending',
        'files': {name: {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()} for name, data in files.items()},
    }
    files['PROVENANCE.json'] = (json.dumps(manifest, indent=2) + '\n').encode('utf-8')
    archive = output / 'BodyPouches-private-wearable-demo-01.zip'
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as stream:
        for name, data in sorted(files.items()):
            item = zipfile.ZipInfo(name, (2026, 10, 25, 0, 0, 0))
            item.compress_type = zipfile.ZIP_DEFLATED
            stream.writestr(item, data)
    # All paths are our fixed allowlist. Check CRC, exact contents and a clean extraction.
    extracted = output / 'verified-extraction'
    with zipfile.ZipFile(archive) as stream:
        if stream.namelist() != sorted(files) or stream.testzip() is not None:
            raise RuntimeError('archive structure/CRC mismatch')
        for name, data in files.items():
            if stream.read(name) != data:
                raise RuntimeError('archive content mismatch: ' + name)
        stream.extractall(extracted)
    for name, data in files.items():
        if (extracted / name).read_bytes() != data:
            raise RuntimeError('extraction mismatch: ' + name)
    if sha(provider) != args.body_slots_sha256.lower():
        raise RuntimeError('provider DLL changed during packaging')
    manifest['archive'] = {'name': archive.name, 'bytes': archive.stat().st_size, 'sha256': sha(archive)}
    manifest['allowlistAndCleanExtraction'] = True
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(manifest['archive']))


if __name__ == '__main__':
    main()
