import copy
import json
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from warehouse_layout import expand


class LayoutTests(unittest.TestCase):
    def setUp(self):
        self.spec=json.loads((Path(__file__).resolve().parents[1]/'configs/warehouse_layout.json').read_text())

    def test_counts_and_attachment(self):
        c=expand(self.spec)
        self.assertEqual(len(c['nodes']),500)
        self.assertEqual(len(c['traffic']),24)
        self.assertEqual(c,expand(self.spec))
        for n in c['nodes']:
            if n['role']=='endpoint':
                adjacent=[l for l in c['links'] if n['id'] in (l['from'],l['to'])]
                self.assertEqual(len(adjacent),1)

    def test_default_matches_delivered_configuration(self):
        expected=json.loads((Path(__file__).resolve().parents[1]/'configs/warehouse_8zone_500.json').read_text())
        self.assertEqual(expand(self.spec),expected)

    def test_all_robot_coverage_and_destinations(self):
        self.spec['traffic']={'mode':'allRobots'}
        c=expand(self.spec);nodes={n['id']:n for n in c['nodes']}
        self.assertEqual(len(c['traffic']),482)
        self.assertEqual({f['source'] for f in c['traffic']},{n['id'] for n in c['nodes'] if n['role']=='endpoint'})
        for f in c['traffic']:
            self.assertNotIn(f['source'],f['destinations'])
            self.assertEqual(len(f['destinations']),1)
            if f['id'].endswith('cross-zone'):self.assertNotEqual(nodes[f['source']]['zone'],nodes[f['destinations'][0]]['zone'])

    def test_scope_and_invalid_traffic(self):
        self.spec['traffic']={'mode':'allRobots','scope':'edge-service','serviceCandidates':2,'bytes':512,'priority':0,'intervalMs':10}
        c=expand(self.spec)
        self.assertTrue(all(f['bytes']==512 and f['priority']==0 and f['intervalMs']==10 and len(f['destinations'])==2 for f in c['traffic']))
        for key,value in [('mode','wrong'),('scope','wrong'),('bytes',1500),('priority',4),('intervalMs',0),('intervalMs',True),('serviceCandidates',17),('extra',1)]:
            s=copy.deepcopy(self.spec);s['traffic'][key]=value
            with self.assertRaises(ValueError):expand(s)
        self.spec['zones']=[dict(id='Solo',robots=1,switches=1,edgeServers=0)]
        self.spec['traffic']={'mode':'allRobots','scope':'same-zone'}
        with self.assertRaises(ValueError):expand(self.spec)

    def test_editable_zone_and_device_counts(self):
        self.spec['zones']=self.spec['zones'][:2]
        self.spec['zones'][0].update(robots=3,switches=2,edgeServers=0)
        c=expand(self.spec)
        self.assertEqual(len(c['zones']),2)
        self.assertEqual(len(c['nodes']),70)
        self.assertEqual(sum(n['role']=='leaf' for n in c['nodes']),3)

    def test_reject_invalid_counts_names_and_unknown_fields(self):
        for field,value in [('robots',-1),('switches',0),('edgeServers',True),('robots',1.5),('extra',1)]:
            s=copy.deepcopy(self.spec);s['zones'][0][field]=value
            with self.assertRaises(ValueError):expand(s)
        self.spec['zones'][1]['id']='A'
        with self.assertRaises(ValueError):expand(self.spec)


if __name__=='__main__':unittest.main()
