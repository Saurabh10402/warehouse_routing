"""Bound a single native process; keep file-backed logs and honest batch status."""
import json
import math
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


def write_json(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def positive_seconds(value):
    seconds = float(value)
    if not math.isfinite(seconds) or seconds <= 0:
        raise ValueError('timeout must be finite and greater than zero')
    return seconds


class CommandFailure(RuntimeError):
    def __init__(self, record):
        self.record = record
        super().__init__(f"{record['status']}: {record['reason']}; log: {record['log']}")


def run_native(command, env, log, receipt, timeout_seconds, cwd):
    """Native executables do not spawn descendants. Supervise that one process only.

    Output goes directly to disk, not a pipe whose reader could wait indefinitely.
    A timeout is failure even if the executable already wrote plausible result files.
    Killing/reaping has a separate five-second bound; an unreaped PID is reported.
    """
    timeout_seconds = positive_seconds(timeout_seconds)
    command = list(map(str, command)); log = Path(log).resolve()
    record = dict(status='STARTING', command=command, log=str(log),
                  timeout_seconds=timeout_seconds, utc=datetime.now(timezone.utc).isoformat())
    write_json(receipt, record)
    print('+ ' + ' '.join(command), flush=True)
    started = time.monotonic(); process = None; interrupted = None
    with log.open('x', encoding='utf-8') as output:
        output.write('+ ' + ' '.join(command) + '\n'); output.flush()
        try:
            process = subprocess.Popen(command, cwd=cwd, env=env, stdout=output,
                                       stderr=subprocess.STDOUT)
            record.update(status='RUNNING', pid=process.pid); write_json(receipt, record)
            code = process.wait(timeout=timeout_seconds)
            record.update(status='EXITED' if code == 0 else 'FAILED', returncode=code,
                          reason='process exited successfully' if code == 0 else f'exit code {code}')
        except subprocess.TimeoutExpired:
            record.update(status='TIMEOUT', reason=f'exceeded {timeout_seconds:g} seconds; partial results are not validated')
        except KeyboardInterrupt as error:
            interrupted = error; record.update(status='INTERRUPTED', reason='user interruption')
        except OSError as error:
            record.update(status='FAILED', reason=str(error))
        finally:
            if process is not None and process.poll() is None:
                try:
                    process.kill(); process.wait(timeout=5)
                except (OSError, subprocess.TimeoutExpired) as error:
                    record['cleanup_error'] = str(error)
            record['reaped'] = process is None or process.poll() is not None
            if process is not None: record['returncode'] = process.poll()
            record['elapsed_seconds'] = time.monotonic() - started
            output.write('\nSUPERVISOR: ' + json.dumps(record) + '\n'); output.flush()
            write_json(receipt, record)
    if interrupted is not None: raise interrupted
    if record['status'] != 'EXITED': raise CommandFailure(record)
    return record


class BatchGuard:
    """A persisted RUNNING state survives hard interruption; PASS must be explicit."""
    def __init__(self, path):
        self.path = Path(path); self.state = dict(status='RUNNING', completed_processes=0)

    def __enter__(self):
        write_json(self.path, self.state); return self

    def active(self, name):
        self.state['active_process'] = name; write_json(self.path, self.state)

    def completed(self):
        self.state['completed_processes'] += 1; write_json(self.path, self.state)

    def passed(self, fields):
        self.state.update(fields, status='PASS'); self.state.pop('active_process', None)
        write_json(self.path, self.state)

    def __exit__(self, kind, error, traceback):
        if kind is not None:
            status = error.record['status'] if isinstance(error, CommandFailure) else (
                'INTERRUPTED' if isinstance(error, KeyboardInterrupt) else 'FAILED')
            self.state.update(status=status, reason=str(error)); write_json(self.path, self.state)
        elif self.state['status'] != 'PASS':
            self.state.update(status='FAILED', reason='batch ended before all acceptance checks completed')
            write_json(self.path, self.state)
            raise RuntimeError(self.state['reason'])
        return False
