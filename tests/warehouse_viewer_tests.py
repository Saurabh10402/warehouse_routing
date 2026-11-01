"""Exporter acceptance checks against a small previously audited native batch."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from visualize_warehouse import export

BATCH=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None


class ViewerTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.batch=self.root/'batch'
        shutil.copytree(BATCH,self.batch)

    def test_offline_export_and_escaping(self):
        p=self.batch/'config.json';c=json.loads(p.read_text());c['name']='</script><script>alert(1)</script>';p.write_text(json.dumps(c))
        output=self.root/'viewer.html';report=export(self.batch,output);s=output.read_text(encoding='utf-8')
        self.assertEqual(report['delivered'],30)
        self.assertNotIn(c['name'],s)
        self.assertNotIn('<script src=',s)
        self.assertNotIn('/*WAREHOUSE_DATA*/',s)
        self.assertIn('\\u003c/script\\u003e',s)
        with self.assertRaises(FileExistsError):export(self.batch,output)

    def test_incomplete_batch_rejected(self):
        (self.batch/'RUN_STATUS.json').write_text('{"status":"RUNNING"}')
        with self.assertRaises(ValueError):export(self.batch,self.root/'out.html')
        self.assertFalse((self.root/'out.html').exists())

    def test_tampered_trace_rejected(self):
        p=self.batch/'run/packets.csv';s=p.read_text();s=s.replace('delivered','no_route',1);p.write_text(s)
        with self.assertRaises((AssertionError,ValueError,RuntimeError)):export(self.batch,self.root/'out.html')
        self.assertFalse((self.root/'out.html').exists())


if __name__=='__main__':
    if BATCH is None:raise SystemExit('Pass a validated small 30-packet demo batch')
    unittest.main()
