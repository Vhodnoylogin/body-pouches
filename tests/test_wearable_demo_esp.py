"""Offline binary/link audit, not equipment/render qualification.

python -B tests/test_wearable_demo_esp.py --master <Skyrim.esm> --author <BodySlots/tools/author.py> --output <new external directory>
The template's neutral pelvic geometry is illustrative and IGNORED by the native13
bridge; VRIK remains its actual reach/visual placement owner. No game bytes copied.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path
import struct
import subprocess
import sys
import unittest
import zlib
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
ARGS = None
RESULT = {}

def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(data)
    return value.hexdigest()

def fields(payload):
    result = []
    offset = 0
    while offset < len(payload):
        if offset + 6 > len(payload):
            raise ValueError('truncated subrecord header')
        tag, size = struct.unpack_from('<4sH', payload, offset)
        offset += 6
        if tag == b'XXXX':
            if size != 4 or offset + 10 > len(payload):
                raise ValueError('bad extended subrecord')
            size = struct.unpack_from('<I', payload, offset)[0]
            tag = bytes(payload[offset + 4:offset + 8])
            offset += 10
        if offset + size > len(payload):
            raise ValueError('truncated subrecord')
        result.append((tag.decode('ascii'), bytes(payload[offset:offset + size])))
        offset += size
    return result

def one(record, tag):
    values = [data for name, data in record['fields'] if name == tag]
    if len(values) != 1:
        raise ValueError(f'{record["type"]} expected one {tag}, found {len(values)}')
    return values[0]

def zstring(data):
    if not data or data[-1] != 0 or b'\0' in data[:-1]:
        raise ValueError('invalid zstring')
    return data[:-1].decode('cp1252')

def walk(data, start=0, end=None, group=None):
    end = len(data) if end is None else end
    offset = start
    while offset < end:
        if offset + 24 > end:
            raise ValueError('truncated record/group header')
        tag, size = struct.unpack_from('<4sI', data, offset)
        if tag == b'GRUP':
            if size < 24 or offset + size > end:
                raise ValueError('invalid group extent')
            label, kind = struct.unpack_from('<4si', data, offset + 8)
            yield {'type': 'GRUP', 'label': label.decode('ascii') if kind == 0 else None, 'kind': kind}
            yield from walk(data, offset + 24, offset + size, label if kind == 0 else group)
            offset += size
        else:
            finish = offset + 24 + size
            if finish > end:
                raise ValueError('invalid record extent')
            flags, form = struct.unpack_from('<II', data, offset + 8)
            yield {'type': tag.decode('ascii'), 'form': form, 'flags': flags, 'group': group,
                   'version': struct.unpack_from('<H', data, offset + 20)[0],
                   'raw': bytes(data[offset:finish]), 'payload': bytes(data[offset + 24:finish])}
            offset = finish
    if offset != end:
        raise ValueError('record boundary mismatch')

def decoded(record):
    payload = record['payload']
    if record['flags'] & 0x40000:
        expected = struct.unpack_from('<I', payload)[0]
        if expected > 16 * 1024 * 1024:
            raise ValueError('master decompression bound')
        decoder = zlib.decompressobj()
        payload = decoder.decompress(payload[4:], expected + 1)
        if len(payload) != expected or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
            raise ValueError('master decompression mismatch')
    record['fields'] = fields(payload)
    return record

def master_metadata(path):
    # Traverse only RACE/FLOR/ARMO groups; parse the requested IDs and one
    # existing clothing record for corroboration. Retain metadata/hashes, no assets.
    wanted = {0x19, 0x13749, 0xD8E80}
    found = {}
    clothing = None
    with path.open('rb') as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as image:
        offset = 0
        while offset < len(image):
            tag, size = struct.unpack_from('<4sI', image, offset)
            if tag != b'GRUP':
                offset += 24 + size
                continue
            if size < 24 or offset + size > len(image):
                raise ValueError('master group boundary')
            label, kind = struct.unpack_from('<4si', image, offset + 8)
            if kind == 0 and label in (b'RACE', b'FLOR', b'ARMO'):
                for row in walk(image, offset + 24, offset + size, label):
                    if row['type'] == 'GRUP':
                        continue
                    if row['form'] not in wanted and not (row['type'] == 'ARMO' and clothing is None):
                        continue
                    decoded(row)
                    entry = {'type': row['type'], 'formId': f'{row["form"]:08X}',
                             'edid': zstring(one(row, 'EDID')), 'recordSha256': hashlib.sha256(row['raw']).hexdigest()}
                    if row['form'] in wanted:
                        if row['form'] == 0xD8E80:
                            entry['model'] = zstring(one(row, 'MODL'))
                        found[row['form']] = entry
                    elif clothing is None:
                        race = [value for name, value in row['fields'] if name == 'RNAM']
                        if len(race) == 1 and len(race[0]) == 4 and struct.unpack('<I', race[0])[0] == 0x19:
                            entry['race'] = '00000019'
                            disk_rating = one(row, 'DNAM')
                            entry['dnamBytes'] = len(disk_rating)
                            entry['dnamUnsigned32'] = struct.unpack('<I', disk_rating)[0]
                            entry['bod2Bytes'] = len(one(row, 'BOD2'))
                            clothing = entry
            offset += size
    return {'source': str(path), 'bytes': path.stat().st_size, 'sha256': digest(path),
            'race': found.get(0x19), 'rejectedDraftRace': found.get(0x13749),
            'coinPurse': found.get(0xD8E80), 'existingArmorWithRace': clothing}

class WearableDemo(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = list(walk((ARGS.output / 'BodyPouches.esp').read_bytes()))
        cls.records = {r['form']: decoded(r) for r in cls.rows if r['type'] != 'GRUP'}
        cls.bag = cls.records[0x01000803]
        cls.master = master_metadata(ARGS.master)
        RESULT['masterEvidence'] = cls.master

    def test_record_extents_versions_master_and_ids(self):
        self.assertEqual(set(self.records), {0, 0x01000800, 0x01000801, 0x01000802, 0x01000803})
        self.assertTrue(all(r['version'] == 44 and r['flags'] == 0 for r in self.records.values()))
        groups = [r for r in self.rows if r['type'] == 'GRUP']
        self.assertEqual({g['label'] for g in groups}, {'MGEF', 'SPEL', 'ARTO', 'ARMO'})
        for r in self.records.values():
            if r['form']:
                self.assertEqual(r['group'], r['type'].encode('ascii'))
        header = self.records[0]
        self.assertEqual(zstring(one(header, 'MAST')), 'Skyrim.esm')
        self.assertEqual(one(header, 'DATA'), bytes(8))
        version, count, next_id = struct.unpack('<fII', one(header, 'HEDR'))
        self.assertAlmostEqual(version, 1.7, places=6)
        self.assertEqual(count, len(self.rows) - 1)
        self.assertEqual(next_id, 0x804)

    def test_original_three_records_preserved_exactly(self):
        released = {r['form']: r for r in walk((ARGS.output / 'baseline.esp').read_bytes()) if r['type'] != 'GRUP'}
        for form in (0x01000800, 0x01000801, 0x01000802):
            self.assertEqual(self.records[form]['raw'], released[form]['raw'])
        RESULT['preservedRecordSha256'] = {f'{form:08X}': hashlib.sha256(self.records[form]['raw']).hexdigest()
                                          for form in (0x01000800, 0x01000801, 0x01000802)}

    def test_ability_effect_art_links_resolve(self):
        spell = self.records[0x01000802]
        effect_id = struct.unpack('<I', one(spell, 'EFID'))[0]
        self.assertEqual(self.records[effect_id]['type'], 'MGEF')
        effect = one(self.records[effect_id], 'DATA')
        self.assertEqual(len(effect), 152)
        art_id = struct.unpack_from('<I', effect, 96)[0]  # MGEF DATA hit-effect Art field, offset0x60.
        self.assertEqual(art_id, 0x01000800)
        self.assertEqual(self.records[art_id]['type'], 'ARTO')

    def test_wearable_clothing_mask_race_and_models(self):
        bag = self.bag
        self.assertEqual(bag['type'], 'ARMO')
        self.assertEqual(zstring(one(bag, 'EDID')), 'BodyPouchesDemoPotionBag')
        mask, armor_type = struct.unpack('<II', one(bag, 'BOD2'))
        self.assertEqual(mask, 0x40000000)  # 60-30; clothing, not a body/hand/shield replacement.
        self.assertEqual(armor_type, 2)
        self.assertEqual(struct.unpack('<I', one(bag, 'RNAM'))[0], 0x19)
        self.assertEqual(self.master['race']['type'], 'RACE')
        self.assertEqual(self.master['race']['edid'], 'DefaultRace')
        self.assertEqual(self.master['rejectedDraftRace']['edid'], 'WoodElfRace')
        self.assertIsNotNone(self.master['existingArmorWithRace'])
        self.assertEqual(self.master['coinPurse']['type'], 'FLOR')
        self.assertEqual(self.master['coinPurse']['edid'], 'CoinPurseLarge')
        self.assertEqual(self.master['coinPurse']['model'].lower(), r'plants\coinbaglarge.nif')
        for tag in ('MOD2', 'MOD4'):
            self.assertEqual(zstring(one(bag, tag)).lower(), self.master['coinPurse']['model'].lower())
        self.assertFalse(any(tag == 'MODL' for tag, _ in bag['fields']))  # ARMO MODL means ARMA link, not a path.
        self.assertFalse(any(r['type'] == 'ARMA' for r in self.rows))
        value, weight = struct.unpack('<If', one(bag, 'DATA'))
        self.assertEqual(value, 20)
        self.assertEqual(weight, 0.5)
        self.assertEqual(struct.unpack('<I', one(bag, 'DNAM'))[0], 0)
        self.assertEqual(len(one(bag, 'DNAM')), self.master['existingArmorWithRace']['dnamBytes'])
        self.assertEqual(self.master['existingArmorWithRace']['dnamBytes'], 4)
        self.assertEqual(self.master['existingArmorWithRace']['bod2Bytes'], 8)

    def test_template_and_real_author_package(self):
        project = json.loads((ROOT / 'demo/potion-bag-template.json').read_text())
        self.assertEqual(project['provider'], 'body-pouches-demo-template')
        self.assertEqual(len(project['slots']), 1)
        slot = project['slots'][0]
        self.assertEqual(slot['key'], 'potion-bag')
        self.assertEqual(slot['anchor'], 'NPC Pelvis [Pelv]')
        self.assertEqual(slot['position'], [0, 0, 0])
        self.assertEqual(set(slot['hands']), {'left', 'right'})
        self.assertEqual(slot['contents'], ['potion'])
        data = (ARGS.output / 'potion-bag.bslots').read_bytes()
        magic, version, header, size, count, payload, flags = struct.unpack_from('<8s6I', data)
        self.assertEqual((magic, version, header, size, count, payload, flags), (b'BSLOTS\r\n', 1, 112, 408, 1, 408, 0))
        self.assertEqual(len(data), 520)
        self.assertEqual(struct.unpack_from('<II', data, 112), (408, 1))
        self.assertEqual(struct.unpack_from('<IIII', data, 112 + 392), (3, 8, 0, 0))

    def test_fresh_output_guard_preserves_existing_file(self):
        target = ARGS.output / 'BodyPouches.esp'
        before = digest(target)
        result = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/make-wearable-demo.py'), str(target)], capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(digest(target), before)

    def test_parser_negative_controls(self):
        data = (ARGS.output / 'BodyPouches.esp').read_bytes()
        with self.assertRaises(ValueError):
            list(walk(data[:-1]))
        with self.assertRaises(ValueError):
            fields(b'RNAM\x04\x00\x49')

def main():
    global ARGS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--master', required=True, type=Path)
    parser.add_argument('--author', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    ARGS = parser.parse_args()
    ARGS.output = ARGS.output.resolve()
    if any((p / '.git').exists() for p in (ARGS.output, *ARGS.output.parents)):
        parser.error('output must be outside Git')
    if ARGS.output.exists() and any(ARGS.output.iterdir()):
        parser.error('output must be new or empty')
    ARGS.output.mkdir(parents=True, exist_ok=True)
    pins = [ROOT / p for p in ('tools/make-wearable-demo.py', 'tools/make-esp.py', 'demo/potion-bag-template.json', 'tests/test_wearable_demo_esp.py')] + [ARGS.author.resolve()]
    hashes = {str(p): digest(p) for p in pins}
    commands = [
        [sys.executable, '-B', str(ROOT / 'tools/make-esp.py'), str(ARGS.output / 'baseline.esp')],
        [sys.executable, '-B', str(ROOT / 'tools/make-wearable-demo.py'), str(ARGS.output / 'BodyPouches.esp')],
        [sys.executable, '-B', str(ARGS.author), str(ROOT / 'demo/potion-bag-template.json'), '--output', str(ARGS.output / 'potion-bag.bslots')]
    ]
    for index, command in enumerate(commands):
        run = subprocess.run(command, capture_output=True, text=True, timeout=30)
        (ARGS.output / f'generate-{index}.log').write_text(run.stdout + run.stderr, encoding='utf-8')
        if run.returncode:
            raise RuntimeError(run.stdout + run.stderr)
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(WearableDemo)
    with (ARGS.output / 'tests.log').open('w', encoding='utf-8') as stream:
        result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    for p in pins:
        if digest(p) != hashes[str(p)]:
            raise RuntimeError('Source changed during test: ' + str(p))
    RESULT.update({'sourcePins': hashes, 'commands': commands, 'testsRun': result.testsRun, 'failures': len(result.failures),
                   'errors': len(result.errors), 'skips': len(result.skipped), 'gameStarts': 0,
                   'artifacts': {name: {'bytes': (ARGS.output / name).stat().st_size, 'sha256': digest(ARGS.output / name)}
                                 for name in ('BodyPouches.esp', 'baseline.esp', 'potion-bag.bslots')},
                   'qualification': 'Offline ESP structure/master references and actual Body Slots author compiler only. No game assets copied. No worn/render/native13 gameplay qualification; template geometry illustrative and ignored by bridge.'})
    (ARGS.output / 'manifest.json').write_text(json.dumps(RESULT, indent=2) + '\n', encoding='utf-8')
    print(f'{result.testsRun} tests, {len(result.failures)} failures, {len(result.errors)} errors, {len(result.skipped)} skips: {ARGS.output}')
    return 0 if result.wasSuccessful() else 1

if __name__ == '__main__':
    raise SystemExit(main())
