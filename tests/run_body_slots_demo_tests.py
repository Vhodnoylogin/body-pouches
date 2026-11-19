"""Focused actual-provider demo consumer; no game/BodyPouches/VRIK DLL loads."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]

def environment():
    if shutil.which('cl.exe'):
        return dict(os.environ)
    locator = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    install = subprocess.check_output([str(locator), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    if not install:
        raise RuntimeError('MSVC x64 compiler is required')
    setup = Path(install) / 'VC/Auxiliary/Build/vcvars64.bat'
    result = subprocess.run(f'cmd.exe /d /s /c "call "{setup}" >nul && set"', check=True, capture_output=True, text=True)
    values = dict(os.environ)
    for line in result.stdout.splitlines():
        if '=' in line and not line.startswith('='):
            key, value = line.split('=', 1)
            values[key.upper()] = value
    return values

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, required=True, help='original public SKSE source root containing common/ and sksevr/')
    parser.add_argument('--body-slots-dll', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if any((p / '.git').exists() for p in (output, *output.parents)) or output == ROOT or ROOT in output.parents:
        parser.error('output must be outside Git checkouts')
    if output.exists() and (not output.is_dir() or any(output.iterdir())):
        parser.error('output must be new or empty')
    sdk = args.sdk_root.resolve()
    if not (sdk / 'common/ITypes.h').is_file() or not (sdk / 'sksevr/skse64/PluginAPI.h').is_file():
        parser.error('sdk-root must contain common/ and sksevr/skse64/')
    output.mkdir(parents=True)
    dll = output / 'BodySlots.dll'
    shutil.copy2(args.body_slots_dll.resolve(strict=True), dll)
    pins = [ROOT / p for p in ('src/vr/BodySlotsDemo.h', 'src/vr/BodySlotsDemo.cpp', 'tests/body_slots_demo_dll_tests.cpp',
                              'tests/body_slots_demo_failure_tests.cpp', 'tests/body-slots-demo-fixture/Windows.h', 'tests/run_body_slots_demo_tests.py')]
    pins += [ROOT / 'contract' / p for p in ('body_slots_api.h', 'body_slots_configuration_api.h', 'body_slots_instance_api.h', 'body_slots_instance_configuration_api.h', 'body_slots_instance_activation_api.h')]
    pins += [sdk / 'common/ITypes.h', sdk / 'sksevr/skse64/PluginAPI.h', sdk / 'sksevr/skse64_common/skse_version.h', dll]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in pins}
    env = environment()
    cl = shutil.which('cl.exe', path=env.get('PATH'))
    if not cl:
        raise RuntimeError('MSVC x64 compiler is required')
    host = output / 'SkyrimVR.exe'  # disposable synthetic SDK host, never the installed game.
    command = [cl, '/nologo', '/std:c++20', '/EHsc', '/MT', '/O2', '/W4', '/WX', '/permissive-', '/utf-8', '/D_CRT_SECURE_NO_WARNINGS',
               '/I' + str(ROOT / 'src'), '/I' + str(ROOT / 'contract'), '/I' + str(sdk), '/I' + str(sdk / 'sksevr'),
               '/Fo' + str(output) + os.sep, '/Fe:' + str(host), str(ROOT / 'tests/body_slots_demo_dll_tests.cpp'), str(ROOT / 'src/vr/BodySlotsDemo.cpp'), '/link', '/INCREMENTAL:NO', '/MACHINE:X64']
    compiled = subprocess.run(command, cwd=output, env=env, capture_output=True, timeout=120)
    (output / 'compile.log').write_bytes(compiled.stdout + compiled.stderr)
    if compiled.returncode:
        raise RuntimeError('Compilation failed: ' + str(output / 'compile.log'))
    run = subprocess.run([str(host), str(dll)], cwd=output, capture_output=True, text=True, timeout=60)
    (output / 'run.log').write_text(run.stdout + run.stderr, encoding='utf-8')
    unit = output / 'failure-tests'
    unit.mkdir()
    unit_exe = unit / 'failure-tests.exe'
    unit_command = [cl, '/nologo', '/std:c++20', '/EHsc', '/MT', '/O2', '/W4', '/WX', '/permissive-', '/utf-8', '/D_CRT_SECURE_NO_WARNINGS',
                    '/I' + str(ROOT / 'tests/body-slots-demo-fixture'), '/I' + str(ROOT / 'src'), '/I' + str(ROOT / 'contract'),
                    '/Fo' + str(unit) + os.sep, '/Fe:' + str(unit_exe), str(ROOT / 'tests/body_slots_demo_failure_tests.cpp'),
                    '/link', '/INCREMENTAL:NO', '/MACHINE:X64']
    unit_compile = subprocess.run(unit_command, cwd=unit, env=env, capture_output=True, timeout=120)
    (unit / 'compile.log').write_bytes(unit_compile.stdout + unit_compile.stderr)
    if unit_compile.returncode:
        raise RuntimeError('Failure fixture compilation failed: ' + str(unit / 'compile.log'))
    unit_run = subprocess.run([str(unit_exe)], cwd=unit, capture_output=True, text=True, timeout=60)
    (unit / 'run.log').write_text(unit_run.stdout + unit_run.stderr, encoding='utf-8')
    for p in pins:
        if hashes[str(p)] != hashlib.sha256(p.read_bytes()).hexdigest():
            raise RuntimeError('Source changed during fixture: ' + str(p))
    manifest = {'sourcePins': hashes, 'compiler': cl, 'compileCommand': command, 'compileExitCode': compiled.returncode,
                'runCommand': [str(host), str(dll)], 'exitCode': run.returncode, 'stdout': run.stdout, 'stderr': run.stderr,
                'failureCompileCommand': unit_command, 'failureCompileExitCode': unit_compile.returncode,
                'failureExitCode': unit_run.returncode, 'failureStdout': unit_run.stdout, 'failureStderr': unit_run.stderr,
                'gameStarts': 0, 'fullVrikDllLoads': 0, 'bodyPouchesDllLoads': 0,
                'boundary': 'Production adapter + actual BodySlots.dll public SKSE callbacks/tables. Explicit observed/worn inputs; no real equipment, native13, Art, rendering or inventory qualification.'}
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(run.stdout + run.stderr + unit_run.stdout + unit_run.stderr, end='')
    return run.returncode or unit_run.returncode

if __name__ == '__main__':
    raise SystemExit(main())
