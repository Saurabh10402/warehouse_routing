"""Exercise the real CLI, independent audit, rejection and result preservation."""
import copy
import csv
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from ns3_warehouse import cases, validate_output

EXE = sys.argv.pop(1)


class NativeCliTests(unittest.TestCase):
    def test_real_artifacts_replay_guards_and_corruption_detection(self):
        with tempfile.TemporaryDirectory(prefix='ns3 warehouse ') as tmp:
            root = Path(tmp); config = cases()[0][1]; path = root/'input.json'
            path.write_text(json.dumps(config),encoding='utf-8'); output = root/'result'
            run = subprocess.run([EXE,'--config',path,'--output',output],capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            self.assertEqual(validate_output(output,config)['delivered'],3)
            before = {p.name:p.read_bytes() for p in output.iterdir()}
            repeated = subprocess.run([EXE,'--config',path,'--output',output],capture_output=True,text=True)
            self.assertNotEqual(repeated.returncode,0)
            self.assertEqual(before,{p.name:p.read_bytes() for p in output.iterdir()})
            summary = json.loads((output/'summary.json').read_text()); summary['offered_packets'] += 1
            (output/'summary.json').write_text(json.dumps(summary))
            with self.assertRaisesRegex(ValueError,'summary application counts'): validate_output(output,config)
            (output/'summary.json').write_bytes(before['summary.json'])
            with (output/'hops.csv').open(newline='') as stream: hops = list(csv.DictReader(stream))
            hops[0]['wire_bytes'] = '100'
            with (output/'hops.csv').open('w',newline='') as stream:
                writer = csv.DictWriter(stream,fieldnames=list(hops[0])); writer.writeheader(); writer.writerows(hops)
            with self.assertRaisesRegex(ValueError,'PPP wire byte count'): validate_output(output,config)

    def test_unsupported_settings_fail_without_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name, change in [('priority',lambda c:c['simulation'].update(scheduler='strictPriority')),
                                 ('ecmp',lambda c:c['routing'].update(algorithm='ecmp')),
                                 ('events',lambda c:c.update(events=[dict(type='link',target='leafA--spine',timeMs=5,available=False)]))]:
                config = copy.deepcopy(cases()[0][1]); change(config); path = root/(name+'.json')
                path.write_text(json.dumps(config)); output = root/name
                run = subprocess.run([EXE,'--config',path,'--output',output],capture_output=True,text=True)
                self.assertNotEqual(run.returncode,0); self.assertIn('not',run.stderr); self.assertFalse(output.exists())


if __name__ == '__main__': unittest.main()
