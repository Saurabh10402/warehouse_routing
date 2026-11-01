"""Generate, build, run and independently audit the editable warehouse demonstration."""
import argparse
import hashlib
import json
import platform
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path
from checkpoint import manifest, git_commit
from verify import ROOT, verify
from warehouse_layout import expand
from ns3_warehouse import validate_output, ARTIFACTS
from process_control import BatchGuard, run_native, positive_seconds


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--layout',type=Path,default=ROOT/'configs/warehouse_layout.json')
    p.add_argument('--ns3-prefix',type=Path,default=ROOT/'.tools/ns3-wired-install')
    p.add_argument('--case-timeout',type=positive_seconds,default=600)
    a=p.parse_args()
    spec=json.loads(a.layout.read_text(encoding='utf-8')); config=expand(spec)
    build,env=verify('default',ns3_prefix=a.ns3_prefix,require_ns3=True)
    exe=build/('ealr-ns3-fabric.exe' if sys.platform=='win32' else 'ealr-ns3-fabric')
    batch=ROOT/'results'/('layout-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(); (batch/'processes').mkdir()
    with BatchGuard(batch/'RUN_STATUS.json') as state:
        for name,data in [('layout.json',spec),('config.json',config),('provenance.json',dict(source=manifest(),git_commit=git_commit(),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),scope='Wired representative-flow demonstration; not all-device load or multi-seed evaluation',devices=len(config['nodes']),flows=len(config['traffic'])))]:
            (batch/name).write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
        libraries={str(f.relative_to(a.ns3_prefix)):hashlib.sha256(f.read_bytes()).hexdigest()
                   for f in (a.ns3_prefix/'lib').glob('*') if f.is_file() and (f.suffix in ['.dll','.dylib'] or '.so' in f.name)}
        if not libraries: raise RuntimeError('Missing ns-3 shared libraries for provenance')
        (batch/'runtime.json').write_text(json.dumps(dict(platform=platform.platform(),python=sys.version,command=sys.argv,
            libraries=libraries,input_sha256=hashlib.sha256((batch/'config.json').read_bytes()).hexdigest()),indent=2)+'\n')
        reports=[]
        for name in ['run','replay']:
            state.active(name)
            run_native([exe,'--config',batch/'config.json','--output',batch/name],env,batch/'processes'/(name+'.log'),batch/'processes'/(name+'.json'),a.case_timeout,ROOT)
            state.completed(); reports.append(validate_output(batch/name,config))
        for name in ARTIFACTS:
            if (batch/'run'/name).read_bytes()!=(batch/'replay'/name).read_bytes(): raise RuntimeError('Replay mismatch: '+name)
        (batch/'audit.json').write_text(json.dumps(reports,indent=2)+'\n')
        state.passed(dict(suite='editable-layout',runs=1,replay='PASS',artifact_checks='PASS',devices=len(config['nodes']),zones=len(config['zones'])))
    print('PASS: '+str(batch))


if __name__=='__main__': main()
