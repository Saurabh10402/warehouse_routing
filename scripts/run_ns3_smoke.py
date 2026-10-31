#!/usr/bin/env python3
"""Strict build/tests plus a real routed packet fixture with reproducible evidence."""
import argparse
import hashlib
import json
import platform
import shutil
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path
from checkpoint import manifest, git_commit
from verify import ROOT, execute, verify


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ns3-prefix',type=Path,default=ROOT/'.tools/ns3-wired-install')
    args=parser.parse_args()
    build,env=verify('default',ns3_prefix=args.ns3_prefix,require_ns3=True)
    executable=build/('ealr-ns3-smoke.exe' if sys.platform=='win32' else 'ealr-ns3-smoke')
    output=ROOT/'results'/('ns3-smoke-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    output.mkdir(parents=True,exist_ok=False)
    log=output/'execution.log'
    execute([executable,'--output',output/'packet-check'],env,log)
    report=json.loads((output/'packet-check/smoke.json').read_text())
    if report['status']!='PASS':raise SystemExit('Packet fixture failed')
    # Replay the same actual ns-3 packets, independent of wall clock/output location.
    execute([executable,'--output',output/'replay'],env,log)
    if (output/'replay/smoke.json').read_bytes()!=(output/'packet-check/smoke.json').read_bytes():
        raise SystemExit('ns-3 smoke replay differed')
    libraries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (args.ns3_prefix/'lib').glob('*')
               if p.is_file() and (p.suffix in ['.dll','.dylib'] or '.so' in p.name)}
    if not libraries:raise SystemExit('No installed ns-3 shared libraries found for provenance')
    receipt=args.ns3_prefix/'setup-receipt.json'
    if receipt.exists():shutil.copyfile(receipt,output/'dependency-setup.json')
    provenance=dict(source=manifest(),git_commit=git_commit(),platform=platform.platform(),architecture=platform.machine(),
        command=sys.argv,python=sys.version,executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
        ns3_libraries_sha256=libraries,scope=report['model'])
    (output/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')
    (output/'RUN_STATUS.json').write_text(json.dumps(dict(status='PASS',suite='ns3-wired-smoke',runs=1,
        replay='PASS',checks='analytical UDP delivery/delay, constant positions, one/two-hop loss tests',
        scope='Prerequisite runtime validation only; schema-v2/NR integration pending'),indent=2)+'\n',encoding='utf-8')
    print('PASS: ns-3 wired packet runtime. OUTPUT: '+str(output))


if __name__=='__main__':main()
