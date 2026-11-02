"""Audited Dijkstra / three-seed ECMP comparison with identical all-robot arrivals."""
import argparse
import copy
import csv
import hashlib
import html
import json
import math
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


def write_json(path, data):
    path.write_text(json.dumps(data, indent=2)+'\n', encoding='utf-8')


def declarations(base):
    result=[]
    for algorithm, seed in [('dijkstra',1),('ecmp',1),('ecmp',2),('ecmp',3)]:
        config=copy.deepcopy(base)
        config['routing']['algorithm']=algorithm
        config['simulation']['seed']=seed
        result.append((f'{algorithm}-s{seed}',config))
    return result


def arrival_signature(packets):
    return [(p['id'],p['flow'],p['source'],p['destination'],p['priority'],
             p['payload_bytes'],p['created_ms'],p['deadline_ms']) for p in packets]


def metrics(packets, spines):
    delays=sorted(float(p['finished_ms'])-float(p['created_ms']) for p in packets if p['status']=='delivered')
    return dict(offered=len(packets),delivered=len(delays),
                deadline_misses=sum(p['deadline_met']=='0' for p in packets),
                mean_ms=sum(delays)/len(delays) if delays else None,
                p95_ms=delays[math.ceil(.95*len(delays))-1] if delays else None,
                max_ms=max(delays) if delays else None,
                spine_packets={s:sum(s in p['observed_path'].split('>') for p in packets) for s in spines},
                spine_flows={s:len({p['flow'] for p in packets if s in p['observed_path'].split('>')}) for s in spines})


def render_report(reports):
    def fmt(v): return 'n/a' if v is None else f'{v:.6f}'
    rows=[]
    for r in reports:
        m=r['all'];cross=r['cross-zone']
        rows.append('<tr><td>'+html.escape(r['case'])+'</td>'+''.join('<td>'+v+'</td>' for v in [
            f"{m['delivered']}/{m['offered']}",str(m['deadline_misses']),fmt(m['mean_ms']),
            fmt(m['p95_ms']),fmt(cross['mean_ms']),
            html.escape(' / '.join(f'{s}: {n}' for s,n in m['spine_packets'].items()))])+'</tr>')
    return '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Warehouse routing comparison</title><style>body{font:16px system-ui;color:#173047;background:#f3f6fa;max-width:1200px;margin:40px auto;padding:20px}table{border-collapse:collapse;background:white;width:100%}td,th{padding:14px;text-align:left;border-bottom:1px solid #dce3eb}.scroll{overflow:auto}h1{font-size:30px}</style>
<h1>Dijkstra and ECMP: measured routing comparison</h1>
<p>Identical stationary wired topology, traffic arrivals, payloads and strict-priority queues. One Dijkstra case and three ECMP hash seeds, each independently audited and replayed exactly.</p>
<div class="scroll"><table><thead><tr><th>Routing / seed</th><th>Delivered / offered</th><th>Deadline misses</th><th>Mean delay (ms)</th><th>P95 delay (ms)</th><th>Cross-zone mean (ms)</th><th>Observed packets per spine</th></tr></thead><tbody>'''+''.join(rows)+'''</tbody></table></div>
<p>Latency describes delivered packets only. P95 uses the nearest-rank definition. Spine counts measure observed packet paths, not bandwidth utilization; local traffic can bypass all spines. Full per-scope counts, delays and distinct-flow counts are retained in comparison.json.</p>
<p>ECMP spreads flows over equal-cost paths using a transport hash; it does not split each packet across paths. Dijkstra selects a shortest path. Better balance does not automatically mean lower delay.</p>
<p>This synchronized illustrative workload is a functional comparison, not a capacity study. Seeds vary only ECMP hashing, not random arrivals. No radio, hardware convergence or target-platform performance claim.</p></html>'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--layout',type=Path,default=ROOT/'configs/warehouse_layout_all_robots.json')
    parser.add_argument('--ns3-prefix',type=Path,default=ROOT/'.tools/ns3-wired-install')
    parser.add_argument('--case-timeout',type=positive_seconds,default=600)
    args=parser.parse_args();prefix=args.ns3_prefix.resolve()
    spec=json.loads(args.layout.read_text(encoding='utf-8'));base=expand(spec)
    declared=declarations(base)
    build,env=verify('default',ns3_prefix=prefix,require_ns3=True)
    exe=build/('ealr-ns3-fabric.exe' if sys.platform=='win32' else 'ealr-ns3-fabric')
    batch=ROOT/'results'/('routing-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(parents=True);(batch/'processes').mkdir();(batch/'inputs').mkdir()
    with BatchGuard(batch/'RUN_STATUS.json') as state:
        source=manifest()
        write_json(batch/'layout.json',spec)
        for name,config in declared:write_json(batch/'inputs'/(name+'.json'),config)
        inputs={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (batch/'inputs').glob('*.json')}
        libraries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (prefix/'lib').glob('*') if p.is_file() and (p.suffix in ['.dll','.dylib'] or '.so' in p.name)}
        if not libraries:raise RuntimeError('Missing native library provenance')
        write_json(batch/'provenance.json',dict(source=source,git_commit=git_commit(),platform=platform.platform(),
            python=sys.version,command=sys.argv,executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
            libraries=libraries,input_sha256=inputs,case_timeout_seconds=args.case_timeout))
        write_json(batch/'declaration.json',dict(cases=[n for n,c in declared],ecmp_hash_seeds=[1,2,3],
            controls='Only routing algorithm and ECMP hash seed vary; topology, scheduling and arrivals identical',
            metrics=['delivery','deadline misses','delivered mean/p95/max latency','per-scope outcomes','observed spine packet and flow counts'],
            scope='Deterministic light functional workload; no saturation, stochastic replication or radio claim'))
        reports=[];audits=[];arrivals=None
        spines=[n['id'] for n in base['nodes'] if n['role']=='spine']
        for name,config in declared:
            for suffix in ['run','replay']:
                label=name+'-'+suffix;state.active(label)
                run_native([exe,'--config',batch/'inputs'/(name+'.json'),'--output',batch/label],env,
                    batch/'processes'/(label+'.log'),batch/'processes'/(label+'.json'),args.case_timeout,ROOT)
                state.completed();audits.append(dict(case=label,report=validate_output(batch/label,config)))
            for artifact in ARTIFACTS:
                if (batch/(name+'-run')/artifact).read_bytes()!=(batch/(name+'-replay')/artifact).read_bytes():raise RuntimeError('Replay mismatch: '+name+'/'+artifact)
            with (batch/(name+'-run')/'packets.csv').open(newline='',encoding='utf-8') as f:packets=list(csv.DictReader(f))
            signature=arrival_signature(packets)
            if arrivals is not None and signature!=arrivals:raise RuntimeError('Routing cases changed arrivals')
            arrivals=signature
            reports.append(dict(case=name,all=metrics(packets,spines),**{scope:metrics([p for p in packets if p['flow'].endswith('-'+scope)],spines) for scope in ['same-zone','cross-zone','edge-service']}))
            print('Audited and replayed: '+name,flush=True)
        if source!=manifest():raise RuntimeError('Source changed during batch')
        if inputs!={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (batch/'inputs').glob('*.json')}:raise RuntimeError('Inputs changed during batch')
        write_json(batch/'audit.json',audits);write_json(batch/'comparison.json',reports)
        (batch/'comparison.html').write_text(render_report(reports),encoding='utf-8')
        state.passed(dict(suite='all-robot-routing-comparison',runs=len(reports),replay='PASS every case',
            artifact_checks='PASS',matched_arrivals='PASS',hash_seeds=[1,2,3],stochastic_evaluation=False))
    print('PASS: '+str(batch))


if __name__=='__main__':main()
