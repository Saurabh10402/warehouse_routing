"""Generate, build, run and independently audit the editable warehouse demonstration."""
import argparse
import csv
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


def spine_outage(config):
    """Declared demonstration: second shared spine down at 30 ms, up at 70 ms."""
    spines=[n['id'] for n in config['nodes'] if n['role']=='spine']
    if len(spines)<2: raise ValueError('Spine outage demo needs at least two shared spines')
    target=spines[1]
    config['events']=[dict(type='node',target=target,timeMs=30,available=False),
                      dict(type='node',target=target,timeMs=70,available=True)]
    return target


def outage_report(directory, target):
    with (directory/'packets.csv').open(newline='',encoding='utf-8') as f: packets=list(csv.DictReader(f))
    phases={name:[] for name in ['before','outage','recovered']}
    for p in packets:
        t=float(p['created_ms']);phase='before' if t<30 else ('outage' if t<70 else 'recovered')
        phases[phase].append(p)
        if p['status']!='delivered':raise ValueError('Outage demonstration unexpectedly lost a packet')
        if phase=='outage' and target in p['observed_path'].split('>'):raise ValueError('Down spine appeared in an outage packet path')
    affected=[]
    for flow in sorted({p['flow'] for p in packets}):
        groups=[[p for p in phases[phase] if p['flow']==flow] for phase in phases]
        if not all(groups):raise ValueError('Missing before/outage/recovery samples')
        before,during,after=[g[0]['observed_path'].split('>') for g in groups]
        if target in before:
            if not any(n.startswith('shared-spine-') for n in during):raise ValueError('Missing alternate spine')
            if before!=after:raise ValueError('Recovered route differs from original stable ECMP route')
            affected.append(dict(flow=flow,before=before,outage=during,recovered=after))
    if not affected:raise ValueError('No offered flow used the failed spine; this is not a rerouting demonstration')
    return dict(failed_spine=target,down_ms=30,up_ms=70,affected_flows=affected,
                local_peer_packets_without_spines=sum(p['flow'].endswith('-same-zone') and 'shared-spine-' not in p['observed_path'] for p in packets),
                phases={phase:dict(offered=len(rows),delivered=sum(p['status']=='delivered' for p in rows)) for phase,rows in phases.items()},
                scope='Immediate centralized route updates; spaced packets avoid the event boundaries; not convergence or full-load validation')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--layout',type=Path,default=ROOT/'configs/warehouse_layout.json')
    p.add_argument('--ns3-prefix',type=Path,default=ROOT/'.tools/ns3-wired-install')
    p.add_argument('--case-timeout',type=positive_seconds,default=600)
    p.add_argument('--spine-outage',action='store_true',help='Second shared spine fails at 30 ms and recovers at 70 ms')
    a=p.parse_args()
    spec=json.loads(a.layout.read_text(encoding='utf-8')); config=expand(spec)
    target=spine_outage(config) if a.spine_outage else None
    build,env=verify('default',ns3_prefix=a.ns3_prefix,require_ns3=True)
    exe=build/('ealr-ns3-fabric.exe' if sys.platform=='win32' else 'ealr-ns3-fabric')
    batch=ROOT/'results'/('layout-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(parents=True,exist_ok=False); (batch/'processes').mkdir()
    with BatchGuard(batch/'RUN_STATUS.json') as state:
        for name,data in [('layout.json',spec),('config.json',config),('provenance.json',dict(source=manifest(),git_commit=git_commit(),executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),scope='Wired configured traffic; deterministic demonstration, not a capacity guarantee or multi-seed evaluation',traffic_mode=spec.get('traffic',{}).get('mode','representative'),active_sources=len({f['source'] for f in config['traffic']}),devices=len(config['nodes']),flows=len(config['traffic'])))]:
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
            if target:
                (batch/(name+'-outage.json')).write_text(json.dumps(outage_report(batch/name,target),indent=2)+'\n')
        for name in ARTIFACTS:
            if (batch/'run'/name).read_bytes()!=(batch/'replay'/name).read_bytes(): raise RuntimeError('Replay mismatch: '+name)
        (batch/'audit.json').write_text(json.dumps(reports,indent=2)+'\n')
        state.passed(dict(suite='editable-layout',runs=1,replay='PASS',artifact_checks='PASS',devices=len(config['nodes']),zones=len(config['zones'])))
    print('PASS: '+str(batch))


if __name__=='__main__': main()
