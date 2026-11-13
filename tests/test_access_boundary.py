import sys,unittest,tempfile,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from audit_access_boundary import check_boundary
from access_study import time_ns

class BoundaryAudit(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=Path(self.temp.name)
        for name in ('input.json','original.log','original.csv'):
            shutil.copyfile(ROOT/'tests/fixtures/access-boundary'/name,self.folder/name)
        shutil.copyfile(self.folder/'original.csv',self.folder/'replay.csv')
        for label in ('original','replay'):(self.folder/(label+'.exit')).write_text('2')
        self.trace=(self.folder/'original.log').read_text();self.logs(self.trace)
    def logs(self,text):
        for label in ('original','replay'):(self.folder/(label+'.log')).write_text(text)
    def test_actual_losses_and_offered_denominator(self):
        a=check_boundary(self.folder)
        self.assertEqual(a['offeredTasks'],46)
        self.assertEqual(a['outcomes'],dict(delivered=43,authorizationTimeout=2,payloadUnreceivedByStop=1,observedPayloadDrops=1,deadlineMet=43,deadlineMissed=3))
        self.assertEqual(a['restorationCrossingTasks'],[14])
    def test_missing_drop_evidence(self):
        self.logs('\n'.join(l for l in self.trace.splitlines() if not l.startswith('IP_DROP ')))
        with self.assertRaises(ValueError):check_boundary(self.folder)
    def test_hidden_deadline_miss(self):
        self.logs(self.trace.replace('TASK_RESULT 18 sent 1 0 -1 -1 0','TASK_RESULT 18 sent 1 0 -1 -1 1'))
        with self.assertRaisesRegex(ValueError,'deadline misses'):check_boundary(self.folder)
    def test_protected_payload_leak(self):
        self.logs(self.trace+'\nTASK_EVENT 12 data-tx 2100000000\n')
        with self.assertRaisesRegex(ValueError,'leaked'):check_boundary(self.folder)
    def test_native_exit_not_hidden(self):
        for label in ('original','replay'):(self.folder/(label+'.exit')).write_text('0')
        with self.assertRaisesRegex(ValueError,'gate inconsistent'):check_boundary(self.folder)
    def test_fractional_start_not_truncated(self):
        self.logs(self.trace.replace('TASK_EVENT 16 start 1998100000','TASK_EVENT 16 start 1998000000'))
        with self.assertRaisesRegex(ValueError,'task start'):check_boundary(self.folder)
    def test_fractional_conversion(self):
        self.assertEqual(time_ns(1999.79),1999790000)
        self.assertEqual(time_ns(1000.5),1000500000)
        self.assertEqual(time_ns(0.0000005),1)

if __name__=='__main__':unittest.main()
