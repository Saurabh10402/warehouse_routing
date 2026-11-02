import copy
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from compare_warehouse_routing import declarations, metrics, arrival_signature
from warehouse_layout import expand


class ComparisonTests(unittest.TestCase):
    def test_only_algorithm_and_seed_vary(self):
        base=expand(json.loads((Path(__file__).resolve().parents[1]/'configs/warehouse_layout_all_robots.json').read_text()))
        saved=copy.deepcopy(base)
        cases=declarations(base)
        self.assertEqual(len(cases),4)
        for name,c in cases:
            c['routing']=copy.deepcopy(base['routing'])
            c['simulation']['seed']=base['simulation']['seed']
            self.assertEqual(c,base)
        self.assertEqual(base,saved)

    def test_loss_denominator_and_exact_spine_tokens(self):
        packets=[dict(status='delivered',finished_ms='3',created_ms='1',deadline_met='1',observed_path='a>s1>b',flow='f'),
                 dict(status='queue_overflow',finished_ms='2',created_ms='1',deadline_met='0',observed_path='a>s10',flow='g')]
        m=metrics(packets,['s1','s10'])
        self.assertEqual((m['offered'],m['delivered'],m['deadline_misses']),(2,1,1))
        self.assertEqual((m['mean_ms'],m['p95_ms'],m['max_ms']),(2,2,2))
        self.assertEqual(m['spine_packets'],{'s1':1,'s10':1})
        self.assertIsNone(metrics([],[])['mean_ms'])

    def test_arrival_guard_ignores_outcome_but_detects_payload_change(self):
        p=dict(id='0',flow='f',source='a',destination='b',priority='1',payload_bytes='256',created_ms='0',deadline_ms='20')
        q=dict(p,status='delivered')
        self.assertEqual(arrival_signature([p]),arrival_signature([q]))
        q['payload_bytes']='255'
        self.assertNotEqual(arrival_signature([p]),arrival_signature([q]))


if __name__=='__main__':unittest.main()
