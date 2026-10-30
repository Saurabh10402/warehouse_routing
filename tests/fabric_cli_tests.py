"""Exercise the real executable, persisted CSVs, invalid inputs and non-overwrite behavior."""
import csv
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

EXE=Path(sys.argv.pop(1));ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from run_all_experiments import validate_artifacts

class CliTests(unittest.TestCase):
    def test_protected_configuration_and_queue_exports(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);config=ROOT/'configs/warehouse_protected.json'
            result=subprocess.run([str(EXE),'--config',str(config),'--output',str(root/'valid')],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            c=json.loads(config.read_text());metrics,_=validate_artifacts(root/'valid',c)
            self.assertGreater(metrics['transmitted_packets'],0)
            for name,value in [('emergencyRateFraction',1),('lowerClassWeights',[1,0,1]),
                               ('lowerClassWeights',[1,1]),('lowerClassWeights',[1.5,1,1])]:
                bad=json.loads(config.read_text());bad['simulation']['protectedQueues'][name]=value
                path=root/'bad.json';path.write_text(json.dumps(bad))
                rejected=subprocess.run([str(EXE),'--config',str(path),'--output',str(root/'invalid')],capture_output=True)
                self.assertNotEqual(rejected.returncode,0);self.assertFalse((root/'invalid').exists())

    def test_export_and_repeat_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);config=ROOT/'configs/warehouse_small.json'
            for name in ['one','two']:
                result=subprocess.run([str(EXE),'--config',str(config),'--output',str(root/name)],capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stderr)
                metrics,classes=validate_artifacts(root/name,json.loads(config.read_text()))
                self.assertEqual(metrics['received_packets'],36)
                self.assertEqual(len(classes),4)
            self.assertEqual((root/'one/packets.csv').read_bytes(),(root/'two/packets.csv').read_bytes())
            before=(root/'one/config.json').read_bytes()
            result=subprocess.run([str(EXE),'--config',str(config),'--output',str(root/'one')],capture_output=True)
            self.assertNotEqual(result.returncode,0)
            self.assertEqual((root/'one/config.json').read_bytes(),before)

    def test_invalid_json_and_unknown_field(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);path=root/'input.json'
            c=json.loads((ROOT/'configs/warehouse_small.json').read_text())
            for data in ['{bad',json.dumps(dict(c,mobility='randomWaypoint'))]:
                path.write_text(data)
                result=subprocess.run([str(EXE),'--config',str(path),'--output',str(root/'out')],capture_output=True)
                self.assertNotEqual(result.returncode,0)
                self.assertFalse((root/'out').exists())

if __name__=='__main__':unittest.main()
