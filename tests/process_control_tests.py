"""Exercise real hung/failed children and prevent premature batch success."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from process_control import BatchGuard, CommandFailure, positive_seconds, run_native


class ProcessControlTests(unittest.TestCase):
    def test_timeout_reaps_child_keeps_partial_artifact_and_marks_batch(self):
        with tempfile.TemporaryDirectory(prefix='native process ') as tmp:
            root = Path(tmp); started = time.monotonic()
            code = "from pathlib import Path; import time; Path('partial.txt').write_text('unvalidated'); print('partial diagnostic',flush=True); time.sleep(30)"
            with self.assertRaises(CommandFailure) as failure:
                with BatchGuard(root/'RUN_STATUS.json') as state:
                    state.active('hung-case')
                    run_native([sys.executable,'-c',code],os.environ.copy(),root/'process.log',root/'process.json',2,root)
                    state.completed()
                    state.passed({})
            self.assertEqual(failure.exception.record['status'],'TIMEOUT')
            self.assertLess(time.monotonic()-started,15)
            receipt=json.loads((root/'process.json').read_text())
            self.assertTrue(receipt['reaped']); self.assertIsNotNone(receipt['returncode'])
            self.assertIn('partial diagnostic',(root/'process.log').read_text())
            self.assertEqual((root/'partial.txt').read_text(),'unvalidated')
            status=json.loads((root/'RUN_STATUS.json').read_text())
            self.assertEqual(status['status'],'TIMEOUT'); self.assertEqual(status['completed_processes'],0)
            self.assertEqual(status['active_process'],'hung-case')

    def test_nonzero_exit_and_launch_failure_are_not_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            commands=[[sys.executable,'-c',"print('failure diagnostic'); raise SystemExit(7)"],[str(root/'missing-executable')]]
            for i,command in enumerate(commands):
                with self.assertRaises(CommandFailure):
                    with BatchGuard(root/f'status{i}.json'):
                        run_native(command,os.environ.copy(),root/f'log{i}.txt',root/f'process{i}.json',5,root)
                self.assertEqual(json.loads((root/f'status{i}.json').read_text())['status'],'FAILED')
            self.assertEqual(json.loads((root/'process0.json').read_text())['returncode'],7)
            self.assertIn('failure diagnostic',(root/'log0.txt').read_text())

    def test_successful_exit_does_not_bypass_artifact_validation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            with self.assertRaisesRegex(ValueError,'bad artifact'):
                with BatchGuard(root/'status.json') as state:
                    run_native([sys.executable,'-c',"print('PASS-looking output')"],os.environ.copy(),root/'ok.log',root/'ok.json',5,root)
                    state.completed()
                    raise ValueError('bad artifact')
            self.assertEqual(json.loads((root/'ok.json').read_text())['status'],'EXITED')
            self.assertEqual(json.loads((root/'status.json').read_text())['status'],'FAILED')
            with self.assertRaisesRegex(RuntimeError,'before all acceptance'):
                with BatchGuard(root/'unfinished.json'): pass
            self.assertEqual(json.loads((root/'unfinished.json').read_text())['status'],'FAILED')
            with BatchGuard(root/'success.json') as state:
                state.completed(); state.passed(dict(artifact_checks='PASS'))
            self.assertEqual(json.loads((root/'success.json').read_text())['status'],'PASS')

    def test_interruption_and_invalid_timeout(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'status.json'
            with self.assertRaises(KeyboardInterrupt):
                with BatchGuard(path): raise KeyboardInterrupt()
            self.assertEqual(json.loads(path.read_text())['status'],'INTERRUPTED')
        for value in [0,-1,float('nan'),float('inf'),float('-inf')]:
            with self.assertRaises(ValueError): positive_seconds(value)
        runner=Path(__file__).resolve().parents[1]/'scripts/run_ns3_warehouse.py'
        result=subprocess.run([sys.executable,str(runner),'--case-timeout','nan'],capture_output=True,text=True,timeout=10)
        self.assertNotEqual(result.returncode,0); self.assertIn('--case-timeout',result.stderr)


if __name__ == '__main__': unittest.main()
