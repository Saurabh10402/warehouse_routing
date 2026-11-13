import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from access_study import validate
from run_access_authorization import capabilities

class OutageConfiguration(unittest.TestCase):
    def setUp(self):self.c=json.loads((ROOT/'configs/access_spine_outage_control.json').read_text())
    def test_supported(self):self.assertEqual(capabilities(self.c)['totalDevices'],19)
    def test_invalid_outages(self):
        for key,value in [('spine','spine-9'),('downMs',False),('downMs',float('nan')),('upMs',2000),('upMs',60001)]:
            with self.subTest(key=key,value=value):
                c=copy.deepcopy(self.c);c['outage'][key]=value
                with self.assertRaises(ValueError):validate(c)
    def test_no_ignored_schedule(self):
        self.c['fabric']['failureStudy']='none'
        with self.assertRaises(ValueError):capabilities(self.c)
    def test_task_start_bounds(self):
        for value in (0,999,60001,float('inf'),True):
            self.c['tasks'][0]['startMs']=value
            with self.assertRaises(ValueError):validate(self.c)
    def test_missing_schedule(self):
        del self.c['outage']
        with self.assertRaises(ValueError):capabilities(self.c)

from audit_access_auth import check
import tempfile
import shutil

class OutageAudit(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=Path(self.temp.name)
        fixture=ROOT/'tests/fixtures/access-spine-outage'
        for name in ('input.json','original.log','original.csv'):shutil.copyfile(fixture/name,self.folder/name)
        shutil.copyfile(self.folder/'original.csv',self.folder/'replay.csv')
        for label in ('original','replay'):(self.folder/(label+'.exit')).write_text('0')
        self.trace=(self.folder/'original.log').read_text()
        self.logs(self.trace)
    def logs(self,text):
        for label in ('original','replay'):(self.folder/(label+'.log')).write_text(text)
    def test_actual_control(self):
        self.assertEqual([p['spines'] for p in check(self.folder)['outage']['phases']],
                         [['spine-0'],['spine-1'],['spine-0']])
    def test_failed_spine_packet_rejected(self):
        self.logs(self.trace+'\nWIRE spine-0 core-sgw 10.0.0.1 10.0.0.2 2152 99 2500000000\n')
        with self.assertRaisesRegex(ValueError,'failed spine'):check(self.folder)
    def test_missing_restore_rejected(self):
        self.logs('\n'.join(line for line in self.trace.splitlines() if not line.startswith('SPINE_EVENT spine-0 up')))
        with self.assertRaisesRegex(ValueError,'outage events'):check(self.folder)
    def test_missing_rebuild_rejected(self):
        self.logs('\n'.join(line for line in self.trace.splitlines() if not line.startswith('ROUTE_REBUILD spine-0')))
        with self.assertRaisesRegex(ValueError,'route rebuild'):check(self.folder)
    def test_start_time_mismatch_rejected(self):
        self.logs(self.trace.replace('TASK_EVENT 0 start 1200000000','TASK_EVENT 0 start 1200000100'))
        with self.assertRaisesRegex(ValueError,'start time'):check(self.folder)
    def test_replay_mismatch_rejected(self):
        (self.folder/'replay.log').write_text(self.trace+'extra')
        with self.assertRaisesRegex(ValueError,'replay differs'):check(self.folder)

if __name__=='__main__':unittest.main()

