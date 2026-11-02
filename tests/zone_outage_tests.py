import csv
import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from warehouse_layout import expand
from zone_outage import configure, report


class ZoneOutageTests(unittest.TestCase):
    def test_configuration(self):
        c=expand(json.loads((Path(__file__).resolve().parents[1]/'configs/warehouse_layout_all_robots.json').read_text()))
        self.assertEqual(configure(c,'A'),'zone-A-switch-0')
        self.assertEqual([e['available'] for e in c['events']],[False,True])
        with self.assertRaises(ValueError):configure(c,'missing')
        c['nodes'].append(dict(id='extra',role='leaf',zone='A'))
        with self.assertRaises(ValueError):configure(c,'A')

    def test_isolation_recovery_and_loss_denominator(self):
        c=dict(nodes=[dict(id=n,zone=z) for n,z in [('a','A'),('b','B'),('c','C')]],traffic=[
            dict(id='local',source='a',destinations=['a']),dict(id='incoming',source='b',destinations=['a']),
            dict(id='unaffected',source='b',destinations=['c'])])
        packets=[dict(id=str(i),flow=flow,created_ms=str(t),status='no_route' if t==40 and flow!='unaffected' else 'delivered',
                      deadline_met='0' if t==40 and flow!='unaffected' else '1')
                 for i,(flow,t) in enumerate((f,t) for f in ['local','incoming','unaffected'] for t in [20,40,80])]
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            def save():
                with (root/'packets.csv').open('w',newline='') as f:
                    w=csv.DictWriter(f,fieldnames=list(packets[0]));w.writeheader();w.writerows(packets)
            save();r=report(root,c,'A')
            self.assertEqual(r['phases']['outage']['affected']['deadline_misses'],2)
            self.assertEqual(r['phases']['outage']['unaffected']['delivered'],1)
            packets[1]['status']='delivered';save()
            with self.assertRaises(ValueError):report(root,c,'A')


if __name__=='__main__':unittest.main()
