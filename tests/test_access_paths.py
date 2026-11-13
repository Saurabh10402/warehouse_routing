import json
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from access_paths import plan

class Paths(unittest.TestCase):
    def setUp(self):
        self.c=json.loads((ROOT/'configs/warehouse_access_small.json').read_text())
    def test_local_and_protected_paths(self):
        tasks=plan(self.c)['tasks']
        self.assertIsNone(tasks[0]['authorization'])
        self.assertFalse(any(n.startswith('spine') for n in tasks[0]['payload']))
        for t in tasks[1:]:
            self.assertEqual(t['authorization']['request'][-1],'main-server-model')
            self.assertTrue(any(n.startswith('spine') for n in t['authorization']['request']))
            self.assertNotIn('external-boundary',t['payload'])
            self.assertIn('core-pgw',t['payload'])
    def test_without_direct_link_uses_spine(self):
        self.c['fabric']['directZoneLinks']=[]
        self.assertTrue(any(n.startswith('spine') for n in plan(self.c)['tasks'][0]['payload']))
    def test_nr_has_no_direct_gnb_core_link(self):
        result=plan(self.c)
        for link in result['links']:
            ends=[link['from'],link['to']]
            self.assertFalse(any('-gnb-' in e for e in ends) and any(e.startswith('core-') for e in ends))
        path=result['tasks'][1]['payload']
        self.assertLess(path.index('zone-A-switch-0'),path.index('core-sgw'))

if __name__=='__main__': unittest.main()
