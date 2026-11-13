import csv,json,shutil,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from audit_access_auth import check
from audit_access_coverage import check_coverage
from run_access_authorization import capabilities

class Coverage(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.folder=Path(self.temp.name)
        for name in ('input.json','original.csv','original.log'):shutil.copyfile(ROOT/'tests/fixtures/access-ecmp'/name,self.folder/name)
        for suffix in ('log','csv'):shutil.copyfile(self.folder/('original.'+suffix),self.folder/('replay.'+suffix))
        for label in ('original','replay'):(self.folder/(label+'.exit')).write_text('0')
        self.audit=check(self.folder)
    def test_actual_inventory_and_partial_coverage(self):
        a=check_coverage(self.folder,self.audit)
        self.assertEqual((a['localDevices'],a['actualNodesIncludingExternalServer'],a['robots']),(19,20,8))
        self.assertFalse(a['allRobotsOfferedInEveryPhase'])
    def test_reject_wired_substitution(self):
        p=self.folder/'original.csv';p.write_text(p.read_text().replace('ns3::NrUeNetDevice','ns3::PointToPointNetDevice'))
        with self.assertRaisesRegex(ValueError,'radio type'):check_coverage(self.folder,self.audit)
    def test_reject_missing_device(self):
        p=self.folder/'original.csv';p.write_text('\n'.join(p.read_text().splitlines()[:-1]))
        with self.assertRaisesRegex(ValueError,'inventory'):check_coverage(self.folder,self.audit)
    def test_reject_wrong_native_count(self):
        p=self.folder/'original.log';p.write_text(p.read_text().replace('actualNodes=20','actualNodes=50'))
        with self.assertRaisesRegex(ValueError,'node/task'):check_coverage(self.folder,self.audit)
    def test_reject_incomplete_task_results(self):
        self.audit['tasks'].pop()
        with self.assertRaisesRegex(ValueError,'coverage result'):check_coverage(self.folder,self.audit)
    def test_four_zone_input(self):
        c=json.loads((ROOT/'configs/access_four_zone_control.json').read_text())
        self.assertEqual(capabilities(c)['totalDevices'],49)
        self.assertEqual(len(c['tasks']),144)
        for prefix in ('before-','during-','after-'):
            self.assertEqual(len({t['source'] for t in c['tasks'] if t['id'].startswith(prefix)}),32)

if __name__=='__main__':unittest.main()
