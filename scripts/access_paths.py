"""Compile design-only expected paths; these are not measured packet traces."""
import argparse
from collections import deque
import json
from pathlib import Path
from access_study import validate
from wireless_layout import expand


def plan(c):
    summary=validate(c)
    devices={d['id']:d for d in expand(c['inventory'])['devices']}
    graph={}
    links=[]
    def link(a,b,kind='wired-fabric'):
        graph.setdefault(a,[]).append(b); graph.setdefault(b,[]).append(a)
        links.append({'from':a,'to':b,'kind':kind})
    spines=[d for d in devices if devices[d]['role']=='spine']
    gateways={}
    for zone in c['inventory']['zones']:
        zid=zone['id']; prefix='zone-'+zid+'-'
        # The new baseline has exactly one routing gateway per zone.
        if zone['switches']!=1: raise ValueError('Path contract requires one zone gateway')
        gateway=prefix+'switch-0'; gateways[zid]=gateway
        for spine in spines: link(gateway,spine)
        for d in devices.values():
            if d['zone']==zid and d['role'] in ('ap','gnb'): link(d['id'],gateway)
    for edge in c['fabric']['directZoneLinks']:
        link(gateways[edge['from']],gateways[edge['to']],'direct-zone-wired')
    for core in ('core-sgw','core-pgw'):
        for spine in spines: link(core,spine)
    # Internal support-core link; explicit, not an AP/gNB bypass.
    link('core-sgw','core-pgw','epc-s5')
    for spine in spines: link(spine,'external-boundary','external-interface')

    def route(a,b):
        previous={a:None}; queue=deque([a])
        while queue and b not in previous:
            current=queue.popleft()
            for nxt in sorted(graph[current]):
                # Core and external-service nodes are not transit shortcuts.
                if nxt not in (a,b) and (nxt.startswith('core-') or nxt=='external-boundary'): continue
                if nxt not in previous: previous[nxt]=current;queue.append(nxt)
        if b not in previous: raise ValueError('Disconnected expected path')
        result=[]; current=b
        while current is not None: result.append(current);current=previous[current]
        return result[::-1]

    def outbound(robot):
        d=devices[robot]
        if d['access']=='wifi': return [robot,d['attachment']]
        return [robot]+route(d['attachment'],'core-sgw')+['core-pgw']
    def inbound(robot): return outbound(robot)[::-1]

    tasks=[]
    protected=set(c['authorization']['requiredTaskTypes'])
    for task in c['tasks']:
        source,destination=task['source'],task['destination']
        left,right=outbound(source),inbound(destination)
        payload=left+route(left[-1],right[0])[1:]+right[1:]
        authorization=None
        if task['type'] in protected:
            request=left+route(left[-1],'external-boundary')[1:]+['main-server-model']
            authorization={'request':request,'response':request[::-1],
                           'payloadGate':'matching-allow-before-send; deny/timeout blocks'}
        tasks.append({'id':task['id'],'authorization':authorization,'payload':payload})
    return {'status':'EXPECTED DESIGN PATHS ONLY; no radio simulation',
            'localDeviceCount':summary['totalDevices'],
            'externalNodesExcludedFromLocalCount':['external-boundary','main-server-model'],
            'routing':'deterministic shortest-hop illustration; not ECMP evaluation',
            'links':links,'tasks':tasks}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('config',type=Path)
    args=p.parse_args()
    print(json.dumps(plan(json.loads(args.config.read_text())),indent=2))
