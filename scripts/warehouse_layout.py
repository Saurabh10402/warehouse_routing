"""Expand editable device counts into a stationary wired schema-v2 configuration."""
import argparse
import json
import math
import re
from pathlib import Path


def expand(spec):
    def keys(value, allowed):
        if not isinstance(value, dict) or set(value)-set(allowed):
            raise ValueError('Unknown fields or invalid object: '+str(value))
    def count(value, name, minimum=0):
        if type(value) is not int or not minimum <= value <= 4096:
            raise ValueError(name+' must be an integer between '+str(minimum)+' and 4096')
        return value
    keys(spec, ['sharedSpines', 'zones', 'traffic'])
    traffic=spec.get('traffic',{})
    keys(traffic,['mode','scope','intervalMs','bytes','priority','serviceCandidates'])
    mode=traffic.get('mode','representative');scope=traffic.get('scope','mixed')
    if mode not in ['representative','allRobots']:raise ValueError('traffic.mode must be representative or allRobots')
    if scope not in ['mixed','same-zone','cross-zone','edge-service']:raise ValueError('Invalid traffic.scope')
    interval=traffic.get('intervalMs',20)
    if type(interval) not in (int,float) or not math.isfinite(interval) or not 0.001<=interval<=100:
        raise ValueError('traffic.intervalMs must be finite and between 0.001 and 100')
    size=count(traffic.get('bytes',256),'traffic.bytes',1)
    if size>1472:raise ValueError('traffic.bytes must not exceed the unfragmented UDP limit 1472')
    priority=count(traffic.get('priority',1),'traffic.priority')
    if priority>3:raise ValueError('traffic.priority must be 0..3')
    candidates=count(traffic.get('serviceCandidates',16 if mode=='representative' else 1),'serviceCandidates',1)
    if candidates>16:raise ValueError('serviceCandidates must not exceed 16')
    spines = count(spec['sharedSpines'], 'sharedSpines', 1)
    zones = spec['zones']
    if not isinstance(zones, list) or not zones:
        raise ValueError('zones must be a nonempty array')
    seen = set()
    for z in zones:
        keys(z, ['id', 'robots', 'switches', 'edgeServers'])
        if not isinstance(z['id'], str) or not re.fullmatch(r'[A-Za-z][A-Za-z0-9_-]*', z['id']) or z['id'] in seen:
            raise ValueError('Zone IDs must be unique simple identifiers')
        seen.add(z['id'])
        for field in ['robots', 'switches', 'edgeServers']:
            count(z[field], field, 1 if field == 'switches' else 0)
    total = spines + sum(z[k] for z in zones for k in ['robots', 'switches', 'edgeServers'])
    if total > 4096:
        raise ValueError('Native backend supports at most 4096 devices')
    c = dict(schemaVersion=2, name='Configurable stationary wired warehouse',
             topology=dict(kind='graph'), simulation=dict(durationMs=100, drainMs=100, seed=1, scheduler='strictPriority'),
             routing=dict(algorithm='ecmp'), deadlinesMs=[10,20,40,100],
             zones=[z['id'] for z in zones], nodes=[], links=[], traffic=[], events=[])
    def node(id, role, zone, x, y):
        c['nodes'].append(dict(id=id, role=role, zone=zone, access='wired', x=x, y=y))
    def link(a,b,capacity):
        c['links'].append(dict(id='link'+str(len(c['links'])), **{'from':a,'to':b}, capacityMbps=capacity, delayMs=0.1, queuePackets=100))
    for s in range(spines):
        node('shared-spine-'+str(s), 'spine', '', s*30, 200)
    robots = {}; edges = {}
    for zi,z in enumerate(zones):
        prefix='zone-'+z['id']+'-'; leaves=[prefix+'switch-'+str(i) for i in range(z['switches'])]
        for i,leaf in enumerate(leaves):
            node(leaf,'leaf',z['id'],zi*100+i*10,100)
            for s in range(spines): link(leaf,'shared-spine-'+str(s),1000)
        robots[z['id']] = [prefix+'robot-'+str(i) for i in range(z['robots'])]
        edges[z['id']] = [prefix+'edge-'+str(i) for i in range(z['edgeServers'])]
        for role, names in [('endpoint',robots[z['id']]),('edge',edges[z['id']])]:
            for i,id in enumerate(names):
                node(id,role,z['id'],zi*100+i%10,(i//10)*3 if role=='endpoint' else 80)
                link(id,leaves[i%len(leaves)],100 if role=='endpoint' else 1000)
    # Preserve the original representative workload exactly when traffic is omitted.
    for zi,z in enumerate(zones):
        peers=robots[z['id']]
        if not peers: continue
        other=next((robots[zones[(zi+j)%len(zones)]['id']] for j in range(1,len(zones)) if robots[zones[(zi+j)%len(zones)]['id']]),[])
        services=edges[z['id']]+[id for zone in zones if zone['id']!=z['id'] for id in edges[zone['id']]]
        for i,source in enumerate(peers if mode=='allRobots' else peers[:1]):
            targets=[]
            if len(peers)>1: targets.append(('same-zone',[peers[(i+1)%len(peers)]]))
            if other: targets.append(('cross-zone',[other[i%len(other)]]))
            if services: targets.append(('edge-service',services[:candidates]))
            if scope!='mixed':targets=[t for t in targets if t[0]==scope]
            if mode=='allRobots':
                if not targets:raise ValueError('No eligible '+scope+' destination for '+source)
                targets=[targets[i%len(targets)] if scope=='mixed' else targets[0]]
            for kind,dest in targets:
                name=z['id']+'-'+kind if mode=='representative' else source+'-'+kind
                c['traffic'].append(dict(id=name,source=source,destinations=dest,priority=priority,bytes=size,startMs=0,stopMs=100,intervalMs=interval))
    if len(c['traffic'])>4096 or len(c['traffic'])*math.ceil(100/interval)>1000000:
        raise ValueError('Workload exceeds native flow/packet limits')
    return c


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--layout',type=Path,default=Path(__file__).resolve().parents[1]/'configs/warehouse_layout.json')
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    try:
        c=expand(json.loads(a.layout.read_text(encoding='utf-8')))
        with a.output.open('x',encoding='utf-8') as f: json.dump(c,f,indent=2); f.write('\n')
    except (ValueError,KeyError,OSError,TypeError) as e: p.exit(2,str(e)+'\n')
    print(f"Generated {len(c['zones'])} zones, {len(c['nodes'])} devices, {len(c['links'])} links, {len(c['traffic'])} flows: {a.output}")
