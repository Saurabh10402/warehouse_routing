#!/usr/bin/env python3
"""Pinned, local wired ns-3 prerequisites. No system install and no radio claim."""
import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import uuid
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from verify import ROOT, environment, execute

VERSION = '3.46.1'
URL = f'https://www.nsnam.org/releases/ns-{VERSION}.tar.bz2'
SHA256 = 'fd3255df035a52008638fb5d81f77685260fd607be107eaf8e5116bcf1fd2c03'
MODULES = 'core,network,mobility,flow-monitor,applications,internet,point-to-point'
TOOLS = ROOT / '.tools'
PREFIX = TOOLS / 'ns3-wired-install'


def source_tree(archive, parent):
    """Verify the pinned archive, validate members, and preserve existing source/builds."""
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise ValueError('ns-3 archive checksum mismatch; preserved for inspection: '+str(archive))
    source = parent / f'ns-{VERSION}'
    if source.exists():
        if not (source/'VERSION').is_file() or (source/'VERSION').read_text().strip() != VERSION:
            raise ValueError('Existing source tree has wrong/missing VERSION; no files replaced')
    with tarfile.open(archive) as package:
        members = []
        for entry in package.getmembers():
            parts = PurePosixPath(entry.name).parts
            if not parts or parts[0] != f'ns-{VERSION}' or '..' in parts or '\\' in entry.name:
                raise ValueError('Unexpected archive path: '+entry.name)
            if entry.issym() and entry.name in [f'ns-{VERSION}/doc/tutorial/source/figures', f'ns-{VERSION}/doc/manual/source/figures']:
                continue  # Two documentation aliases; never needed for source compilation.
            if not entry.isfile() and not entry.isdir():
                raise ValueError('Unexpected special archive member: '+entry.name)
            members.append(entry)
        if source.exists():
            for entry in members:
                if entry.isfile():
                    existing=parent/entry.name
                    if not existing.is_file() or existing.read_bytes()!=package.extractfile(entry).read():
                        raise ValueError('Existing source differs from pinned archive; preserved: '+str(existing))
            return source
        stage = parent / ('ns3-extract-'+uuid.uuid4().hex[:8])
        stage.mkdir()
        package.extractall(stage, members=members, filter='data')
    (stage/f'ns-{VERSION}').rename(source)
    stage.rmdir()  # Only our now-empty staging directory, never recursive cleanup.
    return source


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs',type=int,default=4)
    parser.add_argument('--offline',action='store_true',help='Require the pinned archive already in .tools')
    args=parser.parse_args()
    if args.jobs<1:parser.error('--jobs must be positive')
    if sys.version_info<(3,12):parser.error('Dependency extraction requires Python 3.12+; core runner still supports 3.10+')
    env=environment()
    for command in ['cmake','ninja','c++']:
        if not shutil.which(command,path=env['PATH']):
            raise SystemExit('Missing '+command+'. See docs/first-run.md for the target-platform setup.')
    TOOLS.mkdir(exist_ok=True)
    archive=TOOLS/f'ns-{VERSION}.tar.bz2'
    if not archive.exists():
        if args.offline:raise SystemExit('Offline mode needs '+str(archive))
        partial=TOOLS/(archive.name+'.'+uuid.uuid4().hex[:8]+'.partial')
        print('Downloading pinned ns-3 source: '+URL,flush=True)
        urllib.request.urlretrieve(URL,partial)
        if hashlib.sha256(partial.read_bytes()).hexdigest()!=SHA256:
            raise SystemExit('Downloaded archive checksum mismatch; partial file preserved')
        partial.rename(archive)
    source=source_tree(archive,TOOLS)
    build=TOOLS/'ns3-wired-build'
    # Isolated from the previous full/NetAnim build, preserving its failure evidence.
    # This release discovers reference test data by walking up from the runner executable.
    output=source/'build-wired'
    log=TOOLS/('ns3-setup-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8]+'.log')
    execute(['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
        '-DPython3_EXECUTABLE='+sys.executable,'-DNS3_ENABLED_MODULES='+MODULES,
        '-DNS3_TESTS=ON','-DNS3_EXAMPLES=OFF','-DNS3_PYTHON_BINDINGS=OFF',
        '-DNS3_WARNINGS=ON','-DNS3_WARNINGS_AS_ERRORS=ON',
        '-DNS3_OUTPUT_DIRECTORY='+str(output),'-DCMAKE_INSTALL_PREFIX='+str(PREFIX),
        '-DCMAKE_INSTALL_LIBDIR=lib'],env,log)
    execute(['cmake','--build',build,'-j',str(args.jobs)],env,log)
    runners=list((output/'utils').glob('ns*-test-runner*'))
    runners=[p for p in runners if p.is_file() and (p.suffix=='.exe' if os.name=='nt' else p.suffix=='')]
    if len(runners)!=1:raise SystemExit('Expected one upstream test runner, found '+str(runners))
    env['PATH']=str(output/'lib')+os.pathsep+env['PATH']
    test_log=log.with_name(log.stem+'-unit-tests.log')
    execute([runners[0],'--test-type=unit','--fullness=QUICK'],env,test_log)
    passed=sum(line.startswith('PASS ') for line in test_log.read_text(encoding='utf-8').splitlines())
    if passed==0:raise SystemExit('Upstream test runner reported no passing unit suites; refusing a false PASS')
    execute(['cmake','--install',build],env,log)
    receipt=dict(status='PASS',version=VERSION,archive_sha256=SHA256,url=URL,modules=MODULES,
        platform=platform.platform(),architecture=platform.machine(),python=sys.version,
        tests='upstream UNIT QUICK',passed_suites=passed,test_log=str(test_log),warnings_as_errors=True,log=str(log),
        scope='wired prerequisites only; Wi-Fi/NetAnim/NR not built')
    (PREFIX/'setup-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print('Wired ns-3 prerequisites PASS. Next: python scripts/run_ns3_smoke.py',flush=True)


if __name__=='__main__':main()
