#!/usr/bin/env python3
"""Run every declared deterministic native wired warehouse acceptance case and replay."""
import argparse
import csv
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
from ns3_warehouse import ARTIFACTS, cases, validate_output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ns3-prefix',type=Path,default=ROOT/'.tools/ns3-wired-install')
    args = parser.parse_args(); prefix = args.ns3_prefix.resolve()
    build, env = verify('default',ns3_prefix=prefix,require_ns3=True)
    executable = build/('ealr-ns3-fabric.exe' if sys.platform=='win32' else 'ealr-ns3-fabric')
    batch = ROOT/'results'/('ns3-warehouse-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(parents=True,exist_ok=False); (batch/'inputs').mkdir()
    source = manifest(); reports = []
    for name, config in cases():
        path = batch/'inputs'/(name+'.json'); path.write_text(json.dumps(config,indent=2)+'\n',encoding='utf-8')
        execute([executable,'--config',path,'--output',batch/name],env,batch/'execution.log')
        reports.append(dict(case=name,**validate_output(batch/name,config)))
    name, config = cases()[0]
    execute([executable,'--config',batch/'inputs'/(name+'.json'),'--output',batch/'replay'],env,batch/'execution.log')
    validate_output(batch/'replay',config)
    for artifact in ARTIFACTS:
        if (batch/name/artifact).read_bytes() != (batch/'replay'/artifact).read_bytes():
            raise SystemExit('Native warehouse replay differs: '+artifact)
    if manifest() != source: raise SystemExit('Source changed during batch; results preserved without PASS')
    libraries = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (prefix/'lib').glob('*')
                 if p.is_file() and (p.suffix in ['.dll','.dylib'] or '.so' in p.name)}
    if not libraries: raise SystemExit('Missing library hashes; refusing incomplete provenance')
    if (prefix/'setup-receipt.json').exists(): shutil.copyfile(prefix/'setup-receipt.json',batch/'dependency-setup.json')
    provenance = dict(source=source,git_commit=git_commit(),platform=platform.platform(),architecture=platform.machine(),
        python=sys.version,command=sys.argv,executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
        ns3_libraries_sha256=libraries,input_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (batch/'inputs').glob('*.json')},
        scope='Deterministic wired FIFO/Dijkstra acceptance cases; not a stochastic seed study or NR measurements')
    (batch/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')
    with (batch/'comparison.csv').open('w',newline='',encoding='utf-8') as stream:
        writer = csv.DictWriter(stream,fieldnames=list(reports[0])); writer.writeheader(); writer.writerows(reports)
    (batch/'RUN_STATUS.json').write_text(json.dumps(dict(status='PASS',suite='ns3-wired-warehouse',runs=len(reports),
        artifact_checks='PASS',replay='PASS',stochastic_evaluation=False,scope=provenance['scope']),indent=2)+'\n',encoding='utf-8')
    print('PASS: all '+str(len(reports))+' deterministic ns-3 warehouse cases, independent raw audit and replay. OUTPUT: '+str(batch))


if __name__ == '__main__': main()
