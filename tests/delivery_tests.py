import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from verify_delivery import verify


class DeliveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
        (self.root/'OPEN_WAREHOUSE.html').write_text('<a href="view.html">View</a>')
        (self.root/'view.html').write_text('<h1>Offline view</h1>')
        self.record()

    def record(self):
        files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in self.root.iterdir() if p.name!='DELIVERY_MANIFEST.json'}
        (self.root/'DELIVERY_MANIFEST.json').write_text(json.dumps(dict(files=files)))

    def test_valid_and_changed_payload(self):
        self.assertEqual(verify(self.root)['pages'],2)
        (self.root/'view.html').write_text('modified')
        with self.assertRaises(ValueError):verify(self.root)

    def test_missing_link_even_with_valid_hashes(self):
        (self.root/'OPEN_WAREHOUSE.html').write_text('<a href="absent.html">View</a>');self.record()
        with self.assertRaises(ValueError):verify(self.root)

    def test_external_dependency_and_path_escape(self):
        for link in ['https://example.com/library.js','../outside.html']:
            (self.root/'OPEN_WAREHOUSE.html').write_text('<script src="'+link+'"></script>');self.record()
            with self.assertRaises(ValueError):verify(self.root)


if __name__=='__main__':unittest.main()
