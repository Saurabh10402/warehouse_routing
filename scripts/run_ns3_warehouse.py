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
from verify import ROOT, verify
from process_control import BatchGuard, positive_seconds, run_native
from ns3_warehouse import ARTIFACTS, cases, validate_output, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ns3-prefix',type=Path,default=ROOT/'.tools/ns3-wired-install')
    parser.add_argument('--baseline',type=Path,help='Checkpoint 009 native batch: compare original seven cases on all pre-existing fields')
    parser.add_argument('--case-timeout',type=positive_seconds,default=60.,help='Seconds allowed per native case/replay (default: 60); timeout is failure, never an automatic retry')
    args = parser.parse_args(); prefix = args.ns3_prefix.resolve()
    build, env = verify('default',ns3_prefix=prefix,require_ns3=True)
    executable = build/('ealr-ns3-fabric.exe' if sys.platform=='win32' else 'ealr-ns3-fabric')
    batch = ROOT/'results'/('ns3-warehouse-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(parents=True,exist_ok=False); (batch/'inputs').mkdir()
    (batch/'processes').mkdir()
    with BatchGuard(batch/'RUN_STATUS.json') as state:
        source = manifest(); reports = []; class_reports = []; declared = cases()
        # Freeze every input before the first experiment, including the three-seed matrix.
        for name, config in declared:
            (batch/'inputs'/(name+'.json')).write_text(json.dumps(config,indent=2)+'\n',encoding='utf-8')
        (batch/'declaration.json').write_text(json.dumps(dict(cases=[n for n,c in declared],
            controls=12,matrix_runs=36,seeds=[1,2,3],seed_scope='ECMP hash sensitivity only; deterministic offered traffic'),indent=2)+'\n')
        libraries = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (prefix/'lib').glob('*')
                     if p.is_file() and (p.suffix in ['.dll','.dylib'] or '.so' in p.name)}
        if not libraries: raise SystemExit('Missing library hashes; refusing incomplete provenance')
        if (prefix/'setup-receipt.json').exists(): shutil.copyfile(prefix/'setup-receipt.json',batch/'dependency-setup.json')
        provenance = dict(source=source,git_commit=git_commit(),platform=platform.platform(),architecture=platform.machine(),
            python=sys.version,command=sys.argv,executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
            ns3_libraries_sha256=libraries,input_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (batch/'inputs').glob('*.json')},
            case_timeout_seconds=args.case_timeout,process_logs='processes/*.log and *.json',
            scope='Wired FIFO/priority and Dijkstra/ECMP controls plus three-seed deterministic hash sensitivity; not stochastic replication or NR measurements')
        (batch/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')
        def run_case(name, path):
            state.active(name)
            run_native([executable,'--config',path,'--output',batch/name],env,
                       batch/'processes'/(name+'.log'),batch/'processes'/(name+'.json'),args.case_timeout,ROOT)
            state.completed()
        arrivals = {}; forwarding = {}
        for index,(name, config) in enumerate(declared):
            path = batch/'inputs'/(name+'.json')
            run_case(name,path)
            reports.append(dict(case=name,**validate_output(batch/name,config)))
            run_case(name+'-replay',path)
            validate_output(batch/(name+'-replay'),config)
            for artifact in ARTIFACTS:
                if (batch/name/artifact).read_bytes() != (batch/(name+'-replay')/artifact).read_bytes():
                    raise SystemExit('Native replay differs: '+name+'/'+artifact)
            if args.baseline and index<7:
                for artifact in ARTIFACTS:
                    if artifact=='summary.json':
                        if json.loads((batch/name/artifact).read_text()) != json.loads((args.baseline/name/artifact).read_text()):
                            raise SystemExit('FIFO baseline summary differs: '+name)
                    else:
                        old = rows(args.baseline/name/artifact); new = rows(batch/name/artifact)
                        if len(old)!=len(new) or any(any(actual.get(key)!=value for key,value in previous.items()) for previous,actual in zip(old,new)):
                            raise SystemExit('FIFO baseline fields differ: '+name+'/'+artifact)
            if name.startswith('matrix-'):
                packets = rows(batch/name/'packets.csv')
                common = name.rsplit('-',2)[0]
                offered = [[p[k] for k in ['id','flow','source','destination','priority','payload_bytes','created_ms','deadline_ms']] for p in packets]
                if common in arrivals and arrivals[common]!=offered: raise SystemExit('Offered traffic differs between policies: '+common)
                arrivals[common] = offered
                route_key = name.rsplit('-',1)[0]
                paths = [p['expected_path'] for p in packets]
                if route_key in forwarding and forwarding[route_key]!=paths: raise SystemExit('Scheduler changed routing: '+route_key)
                forwarding[route_key] = paths
                for scope in ['same','cross','edge']:
                    for priority in range(4):
                        selected = [p for p in packets if p['flow']==scope+'-p'+str(priority)]
                        delivered = [p for p in selected if p['status']=='delivered']
                        delays = [float(p['finished_ms'])-float(p['created_ms']) for p in delivered]
                        class_reports.append(dict(case=name,scope=scope,priority=priority,offered=len(selected),delivered=len(delivered),
                            deadline_misses=sum(p['deadline_met']=='0' for p in selected),
                            queue_drops=sum(p['status']=='queue_overflow' for p in selected),cutoff=sum(p['status']=='cutoff' for p in selected),
                            mean_delivered_delay_ms=sum(delays)/len(delays) if delays else ''))
        if manifest() != source: raise SystemExit('Source changed during batch; results preserved without PASS')
        with (batch/'comparison.csv').open('w',newline='',encoding='utf-8') as stream:
            writer = csv.DictWriter(stream,fieldnames=list(reports[0])); writer.writeheader(); writer.writerows(reports)
        with (batch/'scope_priority_comparison.csv').open('w',newline='',encoding='utf-8') as stream:
            writer = csv.DictWriter(stream,fieldnames=list(class_reports[0])); writer.writeheader(); writer.writerows(class_reports)
        state.passed(dict(suite='ns3-wired-warehouse',runs=len(reports),
            artifact_checks='PASS',replay='PASS (every case)',controls=12,matrix_runs=36,hash_seeds=[1,2,3],
            fifo_baseline='PASS (seven cases, all pre-existing fields)' if args.baseline else 'not requested',
            matched_policy_arrivals='PASS',scheduler_independent_routes='PASS',stochastic_evaluation=False,scope=provenance['scope'],case_timeout_seconds=args.case_timeout))
    print('PASS: all '+str(len(reports))+' deterministic ns-3 warehouse cases, independent raw audit and replay. OUTPUT: '+str(batch))


if __name__ == '__main__': main()
