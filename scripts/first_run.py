#!/usr/bin/env python3
"""Build/test the independent core and validate one small stationary warehouse example."""
import hashlib
import json
import platform
import sys
import uuid
from datetime import datetime, timezone
from verify import ROOT, execute, verify
from checkpoint import manifest, git_commit
from run_all_experiments import validate_artifacts


def main():
    build,env=verify('bootstrap')
    executable=build/('ealr-fabric.exe' if sys.platform=='win32' else 'ealr-fabric')
    config=ROOT/'configs/warehouse_communications.json'
    output=ROOT/'results'/('first-run-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    output.mkdir(parents=True,exist_ok=False)
    execute([executable,'--config',config,'--output',output/'example'],env,output/'execution.log')
    metrics,_=validate_artifacts(output/'example',json.loads(config.read_text()))
    if metrics['received_packets']!=180 or metrics['lost_packets']!=0 or metrics['deadline_miss_percent']!=0:
        raise SystemExit('First-run fixture failed expected 180/180 delivery with zero deadline misses; inspect '+str(output))
    provenance=dict(source=manifest(),git_commit=git_commit(),platform=platform.platform(),architecture=platform.machine(),
        python=sys.version,executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
        config_sha256=hashlib.sha256(config.read_bytes()).hexdigest(),scope='stationary abstract links; not ns-3 or NR')
    (output/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')
    (output/'RUN_STATUS.json').write_text(json.dumps(dict(status='PASS',suite='first-run',runs=1,
        checks='180/180 delivery, zero deadline misses, raw artifact validation',scope=provenance['scope']),indent=2)+'\n',encoding='utf-8')
    print('PASS: first-run warehouse fixture, 180/180 delivered, zero deadline misses. OUTPUT: '+str(output))


if __name__=='__main__':main()
