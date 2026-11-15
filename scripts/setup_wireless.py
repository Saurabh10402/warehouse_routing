#!/usr/bin/env python3
"""Create a separate pinned ns-3/NR Linux build; never reuse an existing destination."""
import argparse
import json
import platform
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NS3 = 'ea50b72ab79b1ec5912c66d5b384effa829f18bc'
NR = 'f29ebd33450c49af934ea5dde8606f855c22c2a6'


def setup(args):
    if platform.system() != 'Linux':
        raise ValueError('Run inside Ubuntu 24.04 (Windows: Ubuntu in WSL2).')
    missing = [tool for tool in ('git', 'cmake', 'ninja', 'g++') if not shutil.which(tool)]
    missing += [str(p) for p in (Path('/usr/include/eigen3/Eigen/Core'),
                                Path('/usr/include/sqlite3.h')) if not p.is_file()]
    if missing:
        raise ValueError('Missing prerequisites: ' + ', '.join(missing) +
                         '. See docs/wireless-installation.md.')
    target = args.destination.expanduser().resolve()
    if target.exists():
        raise ValueError('Destination already exists; choose a new directory. Nothing overwritten.')
    if args.jobs < 1 or args.jobs > 8:
        raise ValueError('--jobs must be between 1 and 8.')
    target.mkdir(parents=True)
    logs = target / 'setup-logs'
    logs.mkdir()
    receipt = dict(status='RUNNING', startedUtc=datetime.now(timezone.utc).isoformat(),
                   platform=platform.platform(), ns3=NS3, nr=NR, commands=[])
    receipt_path = target / 'setup-status.json'

    def save():
        receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')

    def run(label, command, cwd=ROOT):
        receipt['stage'] = label
        receipt['commands'].append(command)
        save()
        print(label + ': ' + str(logs / (label + '.log')), flush=True)
        with (logs / (label + '.log')).open('x') as out:
            subprocess.run(command, cwd=cwd, stdout=out, stderr=subprocess.STDOUT, check=True)

    def clone(label, origin, revision, destination):
        run(label + '-clone', ['git', 'clone', '--no-checkout', origin, str(destination)])
        run(label + '-checkout', ['git', '-C', str(destination), 'checkout', '--detach', revision])
        actual = subprocess.check_output(['git', '-C', str(destination), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != revision:
            raise ValueError('Incorrect dependency revision: ' + label)

    try:
        source, build = target / 'ns-3', target / 'build'
        clone('ns3', args.ns3_origin, NS3, source)
        clone('nr', args.nr_origin, NR, source / 'contrib/nr')
        run('test-patch', [sys.executable, str(ROOT / 'scripts/apply_radio_test_patch.py'), str(source)])
        run('configure', ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                         '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DNS3_ASSERT=ON', '-DNS3_LOG=ON',
                         '-DNS3_TESTS=ON', '-DNS3_EXAMPLES=ON', '-DNS3_EIGEN=ON', '-DNS3_SQLITE=ON',
                         '-DNS3_ENABLED_MODULES=nr,wifi,flow-monitor,applications,internet,point-to-point'])
        if args.configure_only:
            receipt['status'] = 'CONFIGURED_NOT_BUILT'
        else:
            run('build', ['cmake', '--build', str(build), '-j', str(args.jobs)])
            run('upstream-quick', [sys.executable, 'test.py', '--no-build', '--fullness=QUICK',
                                  '--text=' + str(logs / 'upstream-quick.txt')], source)
            run('warehouse-smoke', [sys.executable, str(ROOT / 'scripts/run_access_authorization.py'),
                                   str(ROOT / 'configs/access_ecmp_control.json'), '--outcome-measurement',
                                   '--ns3-source', str(source), '--build', str(build)])
            receipt['status'] = 'VALIDATED'
        receipt.update(ns3Source=str(source), build=str(build))
    except BaseException as error:
        receipt.update(status='INTERRUPTED' if isinstance(error, KeyboardInterrupt) else 'FAILED',
                       error=str(error))
        raise
    finally:
        receipt['finishedUtc'] = datetime.now(timezone.utc).isoformat()
        save()
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--configure-only', action='store_true', help='Stop before compilation; not installation acceptance')
    parser.add_argument('--ns3-origin', default='https://gitlab.com/nsnam/ns-3-dev.git', help='Official origin or local Git mirror for offline validation')
    parser.add_argument('--nr-origin', default='https://gitlab.com/cttc-lena/nr.git')
    setup(parser.parse_args())
