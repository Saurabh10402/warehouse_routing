import copy
import json
from pathlib import Path
import sys
import unittest
import tempfile
import subprocess
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from run_access_authorization import capabilities, timed_command

class Capabilities(unittest.TestCase):
    def setUp(self):self.c=json.loads((ROOT/'configs/access_authorization_control.json').read_text())
    def test_supported_modes(self):
        for mode in ('allow','deny','timeout'):
            self.c['externalService']['response']=mode
            self.assertEqual(capabilities(self.c)['totalDevices'],19)
    def test_no_silent_policy_downgrade(self):
        for key,value in [('failureStudy','single-spine-outage')]:
            c=copy.deepcopy(self.c);c['fabric'][key]=value
            with self.assertRaises(ValueError):capabilities(c)
    def test_scale_rejected(self):
        self.c['inventory']['zones'][0]['robots']['wifi']=100
        with self.assertRaises(ValueError):capabilities(self.c)
    def test_timing_bound(self):
        self.c['externalService']['processingDelayMs']=60001
        with self.assertRaises(ValueError):capabilities(self.c)

class TimingFailureEvidence(unittest.TestCase):
    def test_elapsed_saved_without_swallowing_errors(self):
        for error in (RuntimeError('native exit2'),subprocess.TimeoutExpired('native',1200),KeyboardInterrupt()):
            with self.subTest(error=type(error).__name__), tempfile.TemporaryDirectory() as tmp:
                path=Path(tmp)/'timing.json';timings={'original':3.0}
                with patch('run_access_authorization.command',side_effect=error), patch('run_access_authorization.time.monotonic',side_effect=[10.0,14.5]):
                    with self.assertRaises(type(error)):
                        timed_command([],Path(tmp)/'log',Path(tmp),1200,timings,'replay',path)
                self.assertEqual(json.loads(path.read_text()),{'nativeSeconds':{'original':3.0,'replay':4.5},'perRunLimitSeconds':1200})
    def test_success_keeps_exit_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch('run_access_authorization.command',return_value=2),patch('run_access_authorization.time.monotonic',side_effect=[1.0,3.0]):
                self.assertEqual(timed_command([],Path(tmp)/'log',Path(tmp),1200,{},'original',Path(tmp)/'timing.json',allowed=(0,2)),2)

if __name__=='__main__':unittest.main()
