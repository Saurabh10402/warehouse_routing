"""Validate study grouping/denominators separately from algorithm implementation."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from warehouse_study import communications, scenarios, scope_metrics, POLICIES
from protected_queue_study import scenarios as qos_scenarios, POLICIES as QOS_POLICIES, configure_policy
import copy
import capacity_study

class WarehouseStudyTests(unittest.TestCase):
    def test_capacity_matrix_is_paired_and_uses_new_seeds(self):
        cases=capacity_study.scenarios()
        self.assertEqual(sum(len(seeds)*len(capacity_study.POLICIES) for _,_,seeds in cases),288)
        for name,base,seeds in cases:
            self.assertEqual(seeds,[4,5,6])
            for policy,_,_ in capacity_study.POLICIES:
                c=copy.deepcopy(base);capacity_study.configure_policy(c,policy)
                self.assertEqual(c['traffic'],base['traffic'])
                self.assertEqual(c['links'],base['links'])
            if name=='mixed-c1':
                rows={(r['endpoint'],r['direction']):r for r in capacity_study.endpoint_envelopes(base)}
                self.assertAlmostEqual(rows['zone1-ep0','outgoing']['p0_periodic_mbps'],0.2304)
                self.assertEqual(rows['zone1-ep0','outgoing']['p0_source_sigma_bytes'],288)
                self.assertAlmostEqual(rows['zone1-ep0','incoming']['p0_periodic_mbps'],0.1536)
            if name=='burst-c1':
                rows={(r['endpoint'],r['direction']):r for r in capacity_study.endpoint_envelopes(base)}
                self.assertEqual(rows['zone1-ep0','incoming']['p0_source_sigma_bytes'],3264)
                self.assertAlmostEqual(rows['zone1-ep0','incoming']['p0_periodic_mbps'],0.1536)

    def test_protected_study_and_one_factor_ablations(self):
        cases={name:c for name,c,_ in qos_scenarios()}
        self.assertEqual(len(cases),8)
        self.assertEqual(len(QOS_POLICIES),6)
        original=cases['destination_bottleneck']
        for name in ['no_reserved_buffer','equal_lower_class_weights']:
            modified=cases[name]
            self.assertEqual(original['traffic'],modified['traffic'])
            self.assertEqual(original['links'],modified['links'])
        configure_policy(cases['no_reserved_buffer'],'protectedPriority')
        self.assertEqual(cases['no_reserved_buffer']['simulation']['protectedQueues']['reservedEmergencyPackets'],0)
        configure_policy(cases['no_reserved_buffer'],'fifo')
        self.assertNotIn('protectedQueues',cases['no_reserved_buffer']['simulation'])

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

    def test_policing_and_reservation_losses_are_not_hidden(self):
        c=communications([4,4]);flow=c['traffic'][0]
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            with (root/'packets.csv').open('w',newline='') as f:
                writer=csv.DictWriter(f,['flow','priority','status','latency_ms']);writer.writeheader()
                for status in ['priority_policed','reserved_capacity','queue_overflow']:
                    writer.writerow(dict(flow=flow['id'],priority=0,status=status,latency_ms=''))
            row=scope_metrics(root,c)[0]
            self.assertEqual(row['sent'],3);self.assertEqual(row['deadline_miss_percent'],100)
            for status in ['priority_policed','reserved_capacity','queue_overflow']:self.assertEqual(row[status],1)

if __name__=='__main__':unittest.main()
