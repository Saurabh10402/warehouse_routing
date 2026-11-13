import copy,json,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from access_study import validate
from run_access_authorization import check_radio
from run_access_seed_comparison import make_plan,comparison

class RadioSeeds(unittest.TestCase):
    def setUp(self):self.c=json.loads((ROOT/'configs/access_ecmp_control.json').read_text())
    def test_invalid_randomness(self):
        for value in (0,-1,True,1.5,2147483648):
            for key in ('seed','run'):
                c=copy.deepcopy(self.c);c['radioRandomness']={'seed':1,'run':1};c['radioRandomness'][key]=value
                with self.assertRaises(ValueError):validate(c)
        self.c['radioRandomness']={'seed':1}
        with self.assertRaises(ValueError):validate(self.c)
    def test_effective_native_settings(self):
        self.assertEqual(check_radio(self.c,'RADIO_RNG 1 1\n'),{'seed':1,'run':1})
        self.c['radioRandomness']={'seed':2,'run':3}
        for trace in ('RADIO_RNG 1 1\n','','RADIO_RNG 2 3\nRADIO_RNG 2 3\n'):
            with self.assertRaises(ValueError):check_radio(self.c,trace)
    def test_paired_inputs_and_no_mutation(self):
        before=copy.deepcopy(self.c);plan=make_plan(self.c);self.assertEqual(before,self.c)
        self.assertEqual(len(plan),8)
        for i in range(0,8,2):
            left=copy.deepcopy(plan[i][1]);right=copy.deepcopy(plan[i+1][1])
            self.assertEqual(left['fabric']['routing'],'shortest-path');self.assertEqual(right['fabric']['routing'],'ecmp')
            left['fabric']['routing']='ecmp';self.assertEqual(left,right)
    def test_failed_cases_remain_in_batch(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            with patch('run_access_seed_comparison.run',side_effect=RuntimeError('controlled failure')):
                with self.assertRaisesRegex(RuntimeError,'cases failed'):
                    comparison(ROOT/'configs/access_ecmp_control.json',root,root,root)
            batch=next(root.iterdir());status=json.loads((batch/'status.json').read_text());rows=json.loads((batch/'summary.json').read_text())
            self.assertEqual((status['status'],status['completed'],status['failed']),('FAILED',8,8))
            self.assertTrue(all(row['offered']==36 and row['status']=='FAILED' for row in rows))
    def test_subprocess_failure_is_retained(self):
        import subprocess
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            with patch('run_access_seed_comparison.run',side_effect=subprocess.CalledProcessError(7,['test-command'])):
                with self.assertRaises(RuntimeError):comparison(ROOT/'configs/access_ecmp_control.json',root,root,root)
            status=json.loads((next(root.iterdir())/'status.json').read_text())
            self.assertEqual((status['status'],status['failed']),('FAILED',8))
    def test_interruption_is_not_success(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            with patch('run_access_seed_comparison.run',side_effect=KeyboardInterrupt):
                with self.assertRaises(KeyboardInterrupt):comparison(ROOT/'configs/access_ecmp_control.json',root,root,root)
            status=json.loads((next(root.iterdir())/'status.json').read_text())
            self.assertEqual(status['status'],'INTERRUPTED')

if __name__=='__main__':unittest.main()
