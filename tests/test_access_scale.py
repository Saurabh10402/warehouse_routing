import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from run_access_authorization import capabilities, native_run_limit
from run_access_seed_comparison import make_plan
from wireless_layout import expand

class IntermediateScale(unittest.TestCase):
    def setUp(self):self.c=json.loads((ROOT/'configs/access_eight_zone_control.json').read_text())
    def test_explicit_guard_required(self):
        with self.assertRaisesRegex(ValueError,'64'):capabilities(self.c)
        self.assertEqual(capabilities(self.c,scale=True)['totalDevices'],99)
    def test_ceiling_remains(self):
        self.c['inventory']['zones'][0]['robots']['wifi']=210
        with self.assertRaisesRegex(ValueError,'300'):capabilities(self.c,scale=True)
    def test_all_sources_each_phase(self):
        robots={d['id'] for d in expand(self.c['inventory'])['devices'] if d['role']=='robot'}
        self.assertEqual(len(robots),70)
        self.assertEqual(len(self.c['tasks']),306)
        for phase in ('before','during','after'):
            tasks=[t for t in self.c['tasks'] if t['id'].startswith(phase+'-')]
            self.assertEqual({t['source'] for t in tasks},robots)
    def test_128_probe_inventory_and_phase_coverage(self):
        c=json.loads((ROOT/'configs/access_128_device_probe.json').read_text())
        self.assertEqual(capabilities(c,scale=True)['totalDevices'],127)
        robots={d['id'] for d in expand(c['inventory'])['devices'] if d['role']=='robot'}
        self.assertEqual(len(robots),98)
        self.assertEqual(len(c['tasks']),390)
        for phase,low,high in [('before',1200,3000),('during',3300,5000),('after',5300,7000)]:
            tasks=[t for t in c['tasks'] if t['id'].startswith(phase+'-')]
            self.assertEqual(len(tasks),130)
            self.assertEqual({t['source'] for t in tasks},robots)
            self.assertTrue(all(low<=t['startMs']<high for t in tasks))

    def test_200_probe_and_runtime_bounds(self):
        c=json.loads((ROOT/'configs/access_200_device_probe.json').read_text())
        with self.assertRaisesRegex(ValueError,'64'):capabilities(c)
        self.assertEqual(capabilities(c,scale=True)['totalDevices'],199)
        robots={d['id'] for d in expand(c['inventory'])['devices'] if d['role']=='robot'}
        self.assertEqual(len(robots),170)
        self.assertEqual(len(c['tasks']),606)
        for phase,low,high in [('before',1200,3000),('during',3300,5000),('after',5300,7000)]:
            tasks=[t for t in c['tasks'] if t['id'].startswith(phase+'-')]
            self.assertEqual(len(tasks),202)
            self.assertEqual({t['source'] for t in tasks},robots)
            self.assertTrue(all(low<=t['startMs']<high for t in tasks))
        self.assertEqual(native_run_limit(19),180)
        self.assertEqual(native_run_limit(128,True),300)
        self.assertEqual(native_run_limit(129,True),600)
        self.assertEqual(native_run_limit(199,True),600)

    def test_300_probe_and_runtime_boundary(self):
        c=json.loads((ROOT/'configs/access_300_device_probe.json').read_text())
        with self.assertRaisesRegex(ValueError,'64'):capabilities(c)
        self.assertEqual(capabilities(c,scale=True)['totalDevices'],299)
        robots={d['id'] for d in expand(c['inventory'])['devices'] if d['role']=='robot'}
        self.assertEqual(len(robots),270)
        self.assertEqual(len(c['tasks']),906)
        for phase,low,high in [('before',1200,3000),('during',3300,5000),('after',5300,7000)]:
            ts=[t for t in c['tasks'] if t['id'].startswith(phase+'-')]
            self.assertEqual(len(ts),302)
            self.assertEqual({t['source'] for t in ts},robots)
            self.assertTrue(all(low<=t['startMs']<high for t in ts))
        self.assertEqual(native_run_limit(200,True),600)
        self.assertEqual(native_run_limit(201,True),1200)
        self.assertEqual(native_run_limit(299,True),1200)

    def test_comparison_does_not_silently_raise_limit(self):
        with self.assertRaises(ValueError):make_plan(self.c)
        self.assertEqual(len(make_plan(self.c,scale=True)),8)

if __name__=='__main__':unittest.main()
