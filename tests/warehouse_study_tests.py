"""Validate study grouping/denominators separately from algorithm implementation."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from warehouse_study import communications, scenarios, scope_metrics, POLICIES

class WarehouseStudyTests(unittest.TestCase):
    def test_primary_matrix(self):
        cases=scenarios()
        self.assertEqual(len(cases),11)
        self.assertEqual({algorithm for _,algorithm,_ in POLICIES},{'dijkstra','ecmp'})
        self.assertEqual(len(POLICIES),4)
        for _,c,seeds in cases:
            self.assertEqual(seeds,[1,2,3])
            self.assertFalse(any('energy' in n for n in c['nodes']))
            self.assertTrue(all(n['access']=='wired' for n in c['nodes']))
        uneven=communications([2,7,3])
        ids={n['id'] for n in uneven['nodes']}
        self.assertTrue(all(d in ids for w in uneven['traffic'] for d in w['destinations']))

    def test_loss_retains_scope_and_deadline_denominator(self):
        c=communications([4,4]);flow=c['traffic'][0];cross=c['traffic'][1]
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            with (root/'packets.csv').open('w',newline='') as f:
                writer=csv.DictWriter(f,['flow','priority','status','latency_ms','destination'])
                writer.writeheader()
                writer.writerows([
                    dict(flow=flow['id'],priority=0,status='delivered',latency_ms=9,destination=flow['destinations'][0]),
                    dict(flow=flow['id'],priority=0,status='delivered',latency_ms=10,destination=flow['destinations'][0]),
                    dict(flow=flow['id'],priority=0,status='no_route',latency_ms='',destination=''),
                    dict(flow=cross['id'],priority=0,status='no_route',latency_ms='',destination='')])
            rows={r['scope']:r for r in scope_metrics(root,c)}
            self.assertEqual(rows['same_zone']['sent'],3)
            self.assertAlmostEqual(rows['same_zone']['deadline_miss_percent'],200/3)
            self.assertEqual(rows['same_zone']['mean_ms'],9.5)
            self.assertEqual(rows['cross_zone']['deadline_miss_percent'],100)
            self.assertEqual(rows['cross_zone']['mean_ms'],'')

if __name__=='__main__':unittest.main()
