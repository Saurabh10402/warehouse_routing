"""Acceptance guards exercised against completed native outage observations."""
import csv
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from run_warehouse_layout import outage_report,spine_outage

BATCH=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None


class OutageTests(unittest.TestCase):
    def test_observed_failover_and_local_paths(self):
        r=outage_report(BATCH/'run','shared-spine-1')
        self.assertEqual(len(r['affected_flows']),6)
        self.assertEqual(r['local_peer_packets_without_spines'],40)
        self.assertEqual([r['phases'][p]['delivered'] for p in ['before','outage','recovered']],[48,48,24])

    def test_false_success_rejected(self):
        with (BATCH/'run/packets.csv').open(newline='') as f: rows=list(csv.DictReader(f))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'packets.csv'
            # A packet reported as using the failed spine during the outage cannot pass.
            candidate=next(p for p in rows if p['flow']=='A-cross-zone' and float(p['created_ms'])==40)
            candidate['observed_path']=candidate['observed_path'].replace('shared-spine-0','shared-spine-1')
            with path.open('w',newline='') as f:
                w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
            with self.assertRaises(ValueError):outage_report(Path(tmp),'shared-spine-1')

    def test_no_redundant_spine_rejected(self):
        with self.assertRaises(ValueError):spine_outage(dict(nodes=[dict(id='s',role='spine')]))


if __name__=='__main__':
    if BATCH is None:raise SystemExit('Pass the completed 500-device outage batch')
    unittest.main()
