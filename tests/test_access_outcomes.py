import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from audit_access_outcomes import check_outcomes
from audit_access_auth import check as strict_check
from run_access_authorization import run


class OutcomeAudit(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=Path(self.temp.name)
        self.fixture('access-boundary',2)

    def fixture(self,name,code):
        for file in ('input.json','original.log','original.csv'):
            shutil.copyfile(ROOT/'tests/fixtures'/name/file,self.folder/file)
        shutil.copyfile(self.folder/'original.csv',self.folder/'replay.csv')
        self.trace=(self.folder/'original.log').read_text()
        self.logs(self.trace)
        for label in ('original','replay'):(self.folder/(label+'.exit')).write_text(str(code))

    def logs(self,text):
        for label in ('original','replay'):(self.folder/(label+'.log')).write_text(text)

    def test_boundary_outcomes_keep_offered_denominator(self):
        a=check_outcomes(self.folder)
        self.assertEqual(a['offeredTasks'],46)
        self.assertEqual(a['outcomes']['delivered'],43)
        self.assertEqual(a['outcomes']['authorizationTimeout'],2)
        self.assertEqual(a['outcomes']['payloadUnreceivedByStop'],1)
        self.assertEqual(a['outcomes']['deadlineMissed'],3)
        self.assertFalse(a['deliveryControlPassed'])

    def test_unobserved_loss_is_not_relabelled_as_observed_drop(self):
        self.logs('\n'.join(l for l in self.trace.splitlines() if not l.startswith('IP_DROP ')))
        a=check_outcomes(self.folder)
        self.assertEqual(a['outcomes']['observedPayloadDrops'],0)
        self.assertEqual(a['outcomes']['payloadUnreceivedByStop'],1)

    def test_successful_ecmp_fixture(self):
        self.fixture('access-ecmp',0)
        a=check_outcomes(self.folder)
        self.assertTrue(a['allTasksDelivered'] and a['allDeadlinesMet'] and a['deliveryControlPassed'])

    def test_real_late_approval_remains_blocked(self):
        self.fixture('access-outcomes-late',2)
        a=check_outcomes(self.folder)
        self.assertEqual(a['outcomes']['authorizationTimeout'],6)
        self.assertEqual(a['outcomes']['deadlineMet'],30)
        self.assertEqual(sum(t['ignoredLateResponse'] for t in a['tasks']),6)
        self.assertFalse(a['deliveryControlPassed'])
        with self.assertRaisesRegex(ValueError,'Native control failed'):strict_check(self.folder)

    def test_late_approval_cannot_release_payload(self):
        self.fixture('access-outcomes-late',2)
        lines=self.trace.splitlines()
        ignored=next(l.split() for l in lines if l.startswith('TASK_EVENT ') and 'ignored-response' in l)
        i,now=ignored[1],ignored[3]
        self.logs(self.trace+f'\nTASK_EVENT {i} approved {now}\nTASK_EVENT {i} data-tx {now}\n')
        with self.assertRaises(ValueError):check_outcomes(self.folder)

    def test_wrong_timeout_rejected(self):
        self.fixture('access-outcomes-late',2)
        line=next(l for l in self.trace.splitlines() if ' timeout ' in l)
        p=line.split();p[3]=str(int(p[3])+1)
        self.logs(self.trace.replace(line,' '.join(p)))
        with self.assertRaisesRegex(ValueError,'timeout'):check_outcomes(self.folder)

    def test_drop_and_delivery_contradiction_rejected(self):
        drop=next(l for l in self.trace.splitlines() if l.startswith('IP_DROP ') and l.split()[5]=='20018')
        p=drop.split();p[5]='20000'
        self.logs(self.trace+'\n'+' '.join(p))
        with self.assertRaisesRegex(ValueError,'terminal payload drop'):check_outcomes(self.folder)

    def test_missing_replay_rejected(self):
        (self.folder/'replay.log').write_text(self.trace+'extra\n')
        with self.assertRaisesRegex(ValueError,'replay'):check_outcomes(self.folder)

    def test_missing_task_rejected(self):
        self.logs('\n'.join(l for l in self.trace.splitlines() if not l.startswith('TASK_RESULT 12 ')))
        with self.assertRaisesRegex(ValueError,'accounting'):check_outcomes(self.folder)

    def test_duplicate_task_rejected(self):
        self.logs(self.trace+'\n'+next(l for l in self.trace.splitlines() if l.startswith('TASK_RESULT 12 ')))
        with self.assertRaisesRegex(ValueError,'Duplicate'):check_outcomes(self.folder)

    def test_hidden_deadline_miss_rejected(self):
        self.logs(self.trace.replace('TASK_RESULT 18 sent 1 0 -1 -1 0','TASK_RESULT 18 sent 1 0 -1 -1 1'))
        with self.assertRaisesRegex(ValueError,'deadline misses'):check_outcomes(self.folder)

    def test_payload_without_approval_rejected(self):
        self.logs(self.trace+'\nTASK_EVENT 12 data-tx 2100000000\n')
        with self.assertRaisesRegex(ValueError,'Unauthorized'):check_outcomes(self.folder)

    def test_false_native_success_rejected(self):
        for label in ('original','replay'):(self.folder/(label+'.exit')).write_text('0')
        with self.assertRaisesRegex(ValueError,'gate inconsistent'):check_outcomes(self.folder)

    def test_unknown_event_rejected(self):
        self.logs(self.trace+'\nTASK_EVENT 12 magic-delivery 2100000000\n')
        with self.assertRaisesRegex(ValueError,'Unknown'):check_outcomes(self.folder)

    def test_mutually_exclusive_api_modes(self):
        with self.assertRaisesRegex(ValueError,'not both'):
            run(Path('missing'),Path('missing'),Path('missing'),Path('missing'),boundary=True,measure=True)


if __name__=='__main__':unittest.main()
