#!/usr/bin/env python3
"""One-command build, tests, fair experiments, raw traces, provenance and charts."""
import argparse
import copy
import csv
import hashlib
import importlib.util
import json
import statistics
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path

from checkpoint import manifest, git_commit
from generate_examples import warehouse
from verify import ROOT, execute, verify
from warehouse_study import POLICIES, scenarios as warehouse_scenarios, scope_metrics
from protected_queue_study import POLICIES as QOS_POLICIES, scenarios as qos_scenarios, configure_policy
import capacity_study
if (ROOT/'.tools/plotting').is_dir():
    sys.path.insert(0,str(ROOT/'.tools/plotting'))

def legacy_scenarios(quick=False):
    cases=[]
    def small():return json.loads((ROOT/'configs/warehouse_small.json').read_text(encoding='utf-8'))
    def add(name, config, stochastic=False): cases.append((name,config,[1,2,3] if stochastic and not quick else [1]))
    add('baseline',small())
    c=small()
    for link in c['links']:link['capacityMbps']=1;link['queuePackets']=20
    c['simulation']['durationMs']=200;c['simulation']['drainMs']=100
    for flow in c['traffic']:flow.update(stopMs=190,intervalMs=1,bytes=1000)
    add('congestion',c)
    fifo=copy.deepcopy(c);fifo['simulation']['scheduler']='fifo';add('priority_off',fifo)
    mix=copy.deepcopy(c)
    for i,flow in enumerate(mix['traffic']):flow['intervalMs']=[10,5,2,1][i]
    add('priority_mix',mix)
    no_congestion=copy.deepcopy(c);no_congestion['routing']['gamma']=0;add('congestion_weight_off',no_congestion)
    c=small();c['nodes'][1]['energy']=dict(batteryPercent=12,criticalPercent=10,txPercentPerByte=0.005,rxPercentPerByte=0)
    add('low_battery',c)
    no_energy=copy.deepcopy(c);no_energy['routing']['beta']=0;add('energy_weight_off',no_energy)
    tradeoff=copy.deepcopy(c)
    tradeoff['nodes'][1]['energy'].update(batteryPercent=35,txPercentPerByte=0.00001)
    tradeoff['links'][2]['delayMs']=0.3;tradeoff['links'][3]['delayMs']=0.3
    add('energy_latency_tradeoff',tradeoff)
    for name,events in [
        ('uplink_failure',[dict(type='link',target='leaf0--spine0',timeMs=300,available=False),dict(type='link',target='leaf0--spine0',timeMs=700,available=True)]),
        ('spine_failure',[dict(type='node',target='spine0',timeMs=300,available=False),dict(type='node',target='spine0',timeMs=700,available=True)]),
        ('edge_failure',[dict(type='node',target='edge0',timeMs=300,available=False)]),
        ('complete_outage',[dict(type='node',target=f'spine{i}',timeMs=300,available=False) for i in range(2)]),
        ('safety_cloud_off',[dict(type='cloud',timeMs=100,available=False),dict(type='temperature',target='edge0',zone='zone0',timeMs=300,temperatureCelsius=80),dict(type='reset',target='edge0',zone='zone0',timeMs=700,temperatureCelsius=60)])]:
        c=warehouse([10,12]);c['events']=events;add(name,c,True)
    add('uneven',json.loads((ROOT/'configs/warehouse_uneven.json').read_text(encoding='utf-8')),True)
    add('reference_500',json.loads((ROOT/'configs/warehouse_reference.json').read_text(encoding='utf-8')),True)
    if not quick:
        for count in [10,40,100]:add(f'scaling_{count}',warehouse([count]),True)
    return cases

def validate_artifacts(directory, config):
    def ensure(condition):
        if not condition:raise ValueError('Artifact validation failed in '+str(directory))
    def rows(name):
        with (directory/name).open(newline='',encoding='utf-8') as f:return list(csv.DictReader(f))
    packets=rows('packets.csv');metrics={r['metric']:float(r['value']) for r in rows('metrics.csv')}
    ensure(len(packets)==metrics['transmitted_packets'])
    ensure(sum(p['status']=='delivered' for p in packets)==metrics['received_packets'])
    ensure(metrics['received_packets']+metrics['lost_packets']==len(packets))
    ensure(metrics['stationary']==1)
    ensure(all(n['initial_x']==n['final_x'] and n['initial_y']==n['final_y'] for n in rows('nodes.csv')))
    links={frozenset([l['from'],l['to']]) for l in config['links']}
    nodes={n['id']:n for n in config['nodes']}
    for p in packets:
        path=p['path'].split('>')
        ensure(path[0]==p['source'] and len(path)==len(set(path)))
        ensure(all(frozenset([a,b]) in links for a,b in zip(path,path[1:])))
        ensure(all(nodes[n]['role']!='endpoint' for n in path[1:-1]))
        if p['status']=='delivered':ensure(path[-1]==p['destination'])
        missed=p['status']!='delivered' or float(p['latency_ms'])>=config['deadlinesMs'][int(p['priority'])]
        ensure(int(p['deadline_miss'])==int(missed))
    delivered=[p for p in packets if p['status']=='delivered']
    if delivered:
        average=statistics.mean(float(p['finished_ms'])-float(p['created_ms']) for p in delivered)
        ensure(abs(average-metrics['average_end_to_end_delay_ms'])<1e-7)
    ensure(sum(int(p['deadline_miss']) for p in packets)==round(metrics['deadline_miss_percent']*len(packets)/100))
    classes=rows('per_priority.csv')
    queues=rows('queue_stats.csv')
    for q in queues:
        ensure(int(q['attempts'])==sum(int(q[k]) for k in ['admitted','overflow_drops','reservation_drops','policed_drops']))
        ensure(int(q['admitted'])==sum(int(q[k]) for k in ['service_starts','rerouted','queued_at_cutoff']))
        ensure(float(q['mean_wait_before_service_ms'])>=0 and float(q['max_wait_before_service_ms'])>=0)
    for status,column in [('queue_overflow','overflow_drops'),('reserved_capacity','reservation_drops'),('priority_policed','policed_drops')]:
        count=sum(p['status']==status for p in packets)
        ensure(metrics[status+'_packets']==count==sum(int(q[column]) for q in queues))
    for row in classes:
        values=[p for p in packets if p['priority']==row['priority']]
        ensure(int(row['sent'])==len(values))
        ensure(int(row['delivered'])==sum(p['status']=='delivered' for p in values))
        ensure(int(row['lost'])==sum(p['status']!='delivered' for p in values))
        if values:ensure(abs(float(row['deadline_miss_percent'])-100*sum(int(p['deadline_miss']) for p in values)/len(values))<1e-7)
    return metrics, classes

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--quick',action='store_true',help='One seed per case; omits extra scaling cases')
    parser.add_argument('--preset',choices=['bootstrap','default'],default='bootstrap')
    parser.add_argument('--suite',choices=['warehouse','protected-qos','capacity-admission','legacy-ealr'],default='warehouse',
                        help='Powered warehouse communication study (default) or preserved EALR study')
    parser.add_argument('--output-root',type=Path,default=ROOT/'results')
    parser.add_argument('--baseline',type=Path,help='Require identical inputs and packet traces for every run in a previously validated batch')
    args=parser.parse_args()
    cases={'warehouse':warehouse_scenarios,'protected-qos':qos_scenarios,'capacity-admission':capacity_study.scenarios,'legacy-ealr':legacy_scenarios}[args.suite](args.quick)
    policies=capacity_study.POLICIES if args.suite=='capacity-admission' else QOS_POLICIES if args.suite=='protected-qos' else POLICIES if args.suite=='warehouse' else [(a,a,None) for a in ['dijkstra','greedy','ealr']]
    if args.baseline:
        previous=json.loads((args.baseline/'RUN_STATUS.json').read_text())
        if previous['status']!='PASS' or previous['runs']!=sum(len(seeds)*len(policies) for _,_,seeds in cases):
            raise ValueError('Baseline must be a complete validated matrix with the same run count')
    build,env=verify(args.preset)
    executable=build/('ealr-fabric.exe' if sys.platform=='win32' else 'ealr-fabric')
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8]
    output=args.output_root.resolve()/('experiments-'+stamp);output.mkdir(parents=True,exist_ok=False)
    (output/'inputs').mkdir();source=manifest()
    provenance=dict(schema_version=1,utc=datetime.now(timezone.utc).isoformat(),source=source,git_commit=git_commit(),
                    executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),model='stationary abstract full-duplex links; not ns-3 or NR',
                    command=sys.argv,python=sys.version,suite=args.suite,
                    seed_policy='identical inputs and phase seeds across forwarding/scheduling policies')
    if args.baseline:provenance['baseline']=dict(path=str(args.baseline.resolve()),
        source_sha256=json.loads((args.baseline/'provenance.json').read_text())['source']['source_sha256'])
    (output/'provenance.json').write_text(json.dumps(provenance,indent=2),encoding='utf-8')
    run_rows=[];class_rows=[];scope_rows=[];replay_id=None
    for case,base,seeds in cases:
        for seed in seeds:
            offered=None
            for policy,algorithm,scheduler in policies:
                c=copy.deepcopy(base);c['routing']['algorithm']=algorithm;c['simulation']['seed']=seed
                if scheduler is not None:c['simulation']['scheduler']=scheduler
                if args.suite=='protected-qos':configure_policy(c,scheduler)
                if args.suite=='capacity-admission':capacity_study.configure_policy(c,policy)
                run_id=f'{case}-s{seed}-{policy}';input_path=output/'inputs'/f'{run_id}.json'
                if case==cases[0][0] and seed==cases[0][2][0]:replay_id=run_id
                input_path.write_text(json.dumps(c,indent=2)+'\n',encoding='utf-8')
                run=output/run_id
                execute([executable,'--config',input_path,'--output',run],env,output/'execution.log')
                metrics,classes=validate_artifacts(run,c)
                if args.baseline:
                    old=args.baseline/run_id
                    if json.loads((old/'config.json').read_text())!=c:raise ValueError('Baseline input differs: '+run_id)
                    if (old/'packets.csv').read_bytes()!=(run/'packets.csv').read_bytes():raise ValueError('Baseline packet trace differs: '+run_id)
                if args.suite=='protected-qos' and case=='communications' and metrics['deadline_miss_percent']!=0:
                    raise ValueError('Nominal communication deadline acceptance failed: '+run_id)
                with (run/'packets.csv').open(newline='',encoding='utf-8') as f:
                    current=[tuple(p[k] for k in ['flow','source','priority','bytes','created_ms']) for p in csv.DictReader(f)]
                if offered is not None and current!=offered:raise ValueError('Offered traffic differs across algorithms: '+run_id)
                offered=current
                (run/'provenance.json').write_text(json.dumps(dict(run_id=run_id,source_sha256=source['source_sha256'],
                     config_sha256=hashlib.sha256(input_path.read_bytes()).hexdigest(),seed=seed,algorithm=algorithm,policy=policy,
                     executable_sha256=provenance['executable_sha256']),indent=2),encoding='utf-8')
                metadata=dict(case=case,seed=seed,algorithm=algorithm,policy=policy,scheduler=c['simulation'].get('scheduler','strictPriority'))
                run_rows.append(dict(**metadata,run_id=run_id,**metrics))
                class_rows.extend(dict(**metadata,**r) for r in classes)
                scope_rows.extend(dict(**metadata,**r) for r in scope_metrics(run,c))
    for name,rows in [('comparison.csv',run_rows),('priority_comparison.csv',class_rows),('scope_priority_comparison.csv',scope_rows)]:
        fields=list(dict.fromkeys(k for r in rows for k in r))
        with (output/name).open('w',newline='',encoding='utf-8') as f:
            writer=csv.DictWriter(f,fields);writer.writeheader();writer.writerows(rows)
    aggregates=[]
    for case,_,_ in cases:
        for policy,algorithm,_ in policies:
            samples=[r for r in run_rows if r['case']==case and r['policy']==policy]
            for metric in ['average_end_to_end_delay_ms','packet_delivery_ratio_percent','deadline_miss_percent','throughput_mbps']:
                values=[r[metric] for r in samples]
                aggregates.append(dict(case=case,algorithm=algorithm,policy=policy,metric=metric,seeds=len(values),mean=statistics.mean(values),
                                       sample_stddev=statistics.stdev(values) if len(values)>1 else ''))
    with (output/'aggregate.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,aggregates[0].keys());writer.writeheader();writer.writerows(aggregates)
    if args.suite=='capacity-admission':capacity_study.write_tables(output,cases,class_rows,run_rows)
    # Replay baseline: byte-for-byte packet trace, independent of wall clock.
    replay=output/'replay';execute([executable,'--config',output/'inputs'/f'{replay_id}.json','--output',replay],env,output/'execution.log')
    if (replay/'packets.csv').read_bytes()!=(output/replay_id/'packets.csv').read_bytes():raise ValueError('Replay mismatch')
    plotting='unavailable (matplotlib not installed)'
    if importlib.util.find_spec('matplotlib'):
        execute([sys.executable,ROOT/'scripts/plot_fabric.py',output],env,output/'execution.log');plotting='PASS'
    (output/'RUN_STATUS.json').write_text(json.dumps(dict(status='PASS',suite=args.suite,runs=len(run_rows),artifact_checks='PASS',replay='PASS',plots=plotting,
        baseline_comparison='PASS' if args.baseline else 'not requested'),indent=2),encoding='utf-8')
    print(f'PASS: {len(run_rows)} experiments; packet/position/metric checks and replay verified. Plots: {plotting}.\nOUTPUT: {output}')

if __name__=='__main__':main()
