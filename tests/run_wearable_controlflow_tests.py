"""Execute extracted production Mod method bodies at controlled engine/provider boundaries.

This is a CPU control-flow fixture. It does not load Skyrim, BodyPouches, VRIK or
BodySlots DLLs, and does not qualify actual equipment, rendering or inventory.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
sys.dont_write_bytecode = True


def extract(text, marker, path):
    point = text.index(marker)
    begin = text.rfind('\n', 0, point) + 1
    brace = text.index('{', point)
    depth, state, escaped, index = 0, 'code', False, brace
    while index < len(text):
        char = text[index]
        following = text[index:index + 2]
        if state == 'line':
            if char == '\n': state = 'code'
        elif state == 'block':
            if following == '*/': state = 'code'; index += 1
        elif state in ('"', "'"):
            if escaped: escaped = False
            elif char == '\\': escaped = True
            elif char == state: state = 'code'
        elif following == '//': state = 'line'; index += 1
        elif following == '/*': state = 'block'; index += 1
        elif char in ('"', "'"): state = char
        elif char == '{': depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0:
                line = text.count('\n', 0, begin) + 1
                return f'#line {line} "{path.as_posix()}"\n' + text[begin:index + 1], line
        index += 1
    raise RuntimeError('Unclosed production function: ' + marker)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1], help='source checkout; defaults to this runner repository')
    parser.add_argument('--output', type=Path, required=True, help='new external output directory')
    args = parser.parse_args()
    root = args.repo.resolve(strict=True)
    output = args.output.resolve()
    if any((p / '.git').exists() for p in (output, *output.parents)):
        parser.error('output must be outside Git checkouts')
    if output.exists() and (not output.is_dir() or any(output.iterdir())):
        parser.error('output must be new or empty')
    output.mkdir(parents=True, exist_ok=True)
    template_path = Path(__file__).with_name('wearable_controlflow_fixture.cpp.in')
    env_helper_path = root / 'tests/run_body_slots_demo_tests.py'
    spec = importlib.util.spec_from_file_location('body_pouches_demo_tools', env_helper_path)
    helper = importlib.util.module_from_spec(spec); spec.loader.exec_module(helper)
    env = helper.environment()
    compiler = shutil.which('cl.exe', path=env.get('PATH'))
    if not compiler: raise RuntimeError('MSVC x64 compiler is required')
    source_path = root / 'src/Mod.cpp'
    adapter_path = root / 'src/vr/BodySlotsDemo.cpp'
    source = source_path.read_text(encoding='utf-8-sig')
    adapter = adapter_path.read_text(encoding='utf-8-sig')
    names = ['DemoEquipment::ProcessEvent', 'RefreshDemoEquipment', 'DemoAllows', 'PouchAt', 'CancelPendingRelease',
             'SettleOutsideDrop', 'ResolvePendingReleases', 'QueueStowDropped', 'InvalidateWorld', 'Draw',
             'StowDropped', 'HandleHolsterAttempt', 'OnDropped', 'OnGrabbed']
    methods = {}; ranges = {}
    for name in names:
        methods[name], ranges[name] = extract(source, 'Mod::' + name + '(', source_path)
    tokens = '\n'.join(extract(adapter, marker, adapter_path)[0] for marker in
                       ('BodySlotsDemo::Token::Valid(', 'operator==(const BodySlotsDemo::Token&'))
    template = template_path.read_text(encoding='utf-8-sig')
    all_methods = '\n\n'.join(methods.values())
    keys = sorted(set(re.findall(r'Keys::(\w+)', all_methods)))
    events = sorted(set(re.findall(r'Diagnostics::Event::(\w+)', all_methods)))
    template = template.replace('@@KEYS@@', '\n'.join(f'inline constexpr const char* {name}="{name}";' for name in keys))
    template = template.replace('@@EVENTS@@', ','.join(events)).replace('@@TOKENS@@', tokens)
    core_names = ['PouchSet.h', 'PouchSet.cpp', 'Pouch.h', 'Filter.h', 'Item.h', 'Pack.h', 'Decision.h', 'PendingGrab.h', 'CurrentUpdateRelease.h', 'LoadGate.h']
    pins = [source_path, root/'src/Mod.h', root/'CMakeLists.txt', root/'tools/make-wearable-demo.py', adapter_path,
            root/'src/vr/BodySlotsDemo.h', env_helper_path, template_path, Path(__file__).resolve()]
    pins += [root/'src/core'/name for name in core_names]
    pins += sorted((root/'contract').glob('body_slots*.h'))
    hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in pins}
    outcomes = []

    def build_run(label, selected, case=None, expected_failure=False):
        folder = output/label; folder.mkdir()
        copied = '\n\n'.join(selected.values())
        (folder/'production_methods.inc').write_text(copied+'\n', encoding='utf-8')
        cpp = folder/'controlflow.cpp'
        cpp.write_text(template.replace('@@METHODS@@', '#include "production_methods.inc"'), encoding='utf-8')
        executable = folder/'controlflow.exe'
        command = [compiler, '/nologo', '/std:c++20', '/EHsc', '/MT', '/Od', '/W4', '/WX', '/permissive-', '/utf-8',
                   '/DBODYPOUCHES_WEARABLE_DEMO=1', '/I'+str(root/'src'), '/I'+str(root/'contract'),
                   '/Fo'+str(folder)+os.sep, '/Fe:'+str(executable), str(cpp), str(root/'src/core/PouchSet.cpp'),
                   '/link', '/INCREMENTAL:NO', '/MACHINE:X64']
        compiled = subprocess.run(command, cwd=folder, env=env, capture_output=True, timeout=120)
        (folder/'compile.log').write_bytes(compiled.stdout+compiled.stderr)
        if compiled.returncode: raise RuntimeError('Compilation failed: '+str(folder/'compile.log'))
        run_command = [str(executable)] + ([case] if case else [])
        run = subprocess.run(run_command, cwd=folder, capture_output=True, text=True, encoding='utf-8', timeout=30)
        (folder/'run.log').write_text(run.stdout+run.stderr, encoding='utf-8')
        expected = run.returncode != 0 and 'FAIL '+str(case)+':' in run.stderr if expected_failure else run.returncode == 0
        outcomes.append({'name':label,'case':case,'negativeControl':expected_failure,'compileCommand':command,
                         'compileExitCode':compiled.returncode,'runCommand':run_command,'exitCode':run.returncode,
                         'stdout':run.stdout,'stderr':run.stderr,'expectedResultObserved':expected})
        if not expected: raise RuntimeError('Unexpected control-flow result: '+str(folder/'run.log'))
        print(label+': '+run.stdout.strip()+(' (negative control rejected)' if expected_failure else ''))

    build_run('production', methods)
    controls = [
        ('recapture_original_event', 'OnDropped', 'release.bagToken = bagToken;',
         'release.bagToken = (static_cast<void>(bagToken), mod._demoSlots.CurrentToken());', 'original_event_capture'),
        ('recapture_at_queue', 'QueueStowDropped', '\n\t{\n', '\n\t{\n\t\tbagToken = _demoSlots.CurrentToken();\n', 'queue_no_recapture'),
        ('clear_removed_potion', 'RefreshDemoEquipment', '\n\t\tif (!_demoActive || previous != current.token) {',
         '\n\t\tif (!_demoActive || previous != current.token) {\n\t\t\t_pendingGrab[0].Clear(); _pendingGrab[1].Clear();', 'removed_survives'),
        ('omit_local_reset', 'InvalidateWorld', '_demoSlots.Reset();', '/* negative control: omitted original reset */', 'reset_before_callbacks'),
    ]
    for label,name,before,after,case in controls:
        changed = dict(methods)
        if changed[name].count(before)!=1: raise RuntimeError('Negative control patch no longer exact: '+label)
        changed[name] = changed[name].replace(before, after)
        build_run(label, changed, case, True)
    for path in pins:
        if hashlib.sha256(path.read_bytes()).hexdigest()!=hashes[str(path)]:raise RuntimeError('Source changed while running: '+str(path))
    manifest = {'sourcePins':hashes,'extractedMethods':ranges,'compiler':compiler,'outcomes':outcomes,
                'allExpectedResultsObserved':True,'gameStarts':0,'engineDllLoads':0,
                'boundary':'Exact production Mod bodies and production Core/PouchSet + Token methods; controlled engine, task queue, hand and copied-admission boundary doubles. No full plugin/DLL, real equipment, native reach, Art, render or inventory qualification.'}
    (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
