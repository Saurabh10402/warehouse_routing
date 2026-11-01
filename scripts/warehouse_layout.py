"""Expand editable device counts into a stationary wired schema-v2 configuration."""
import argparse
import json
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
    keys(spec, ['sharedSpines', 'zones'])
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
    # Representative traffic exercises all three scopes without implying every robot transmits.
    for zi,z in enumerate(zones):
        peers=robots[z['id']]
        if not peers: continue
        targets=[]
        if len(peers)>1: targets.append(('same-zone',[peers[1]]))
        other=next((robots[zones[(zi+j)%len(zones)]['id']] for j in range(1,len(zones)) if robots[zones[(zi+j)%len(zones)]['id']]),[])
        if other: targets.append(('cross-zone',[other[0]]))
        services=edges[z['id']]+[id for zone in zones if zone['id']!=z['id'] for id in edges[zone['id']]]
        if services: targets.append(('edge-service',services[:16]))
        for scope,dest in targets:
            c['traffic'].append(dict(id=z['id']+'-'+scope,source=peers[0],destinations=dest,priority=1,bytes=256,startMs=0,stopMs=100,intervalMs=20))
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
    print(f"Generated {len(c['zones'])} zones, {len(c['nodes'])} devices, {len(c['links'])} links, {len(c['traffic'])} representative flows: {a.output}")
