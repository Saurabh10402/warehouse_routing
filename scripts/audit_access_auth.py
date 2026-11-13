"""Check native task authorization ordering, blocking, timings and server boundary."""
import argparse
from collections import defaultdict
import json
from pathlib import Path
from access_study import time_ns


def check(folder):
    config=json.loads((folder/'input.json').read_text())
    data=(folder/'original.log').read_bytes()
    if data!=(folder/'replay.log').read_bytes():raise ValueError('Trace replay differs')
    if (folder/'original.csv').read_bytes()!=(folder/'replay.csv').read_bytes():raise ValueError('Device replay differs')
    if any((folder/(r+'.exit')).read_text().strip()!='0' for r in ('original','replay')):raise ValueError('Native control failed')
    events=defaultdict(dict);results={};wire=[];identities={};spine_events=[];rebuilds=[]
    for line in data.decode().splitlines():
        p=line.split()
        if p and p[0]=='TASK_EVENT':
            i=int(p[1]);name=p[2]
            if name in events[i]:raise ValueError('Unexpected duplicate event')
            events[i][name]=int(p[3])
        elif p and p[0]=='TASK_RESULT':results[int(p[1])]=p[2:]
        elif p and p[0]=='WIRE':wire.append(p)
        elif p and p[0]=='SPINE_EVENT':spine_events.append(p[1:])
        elif p and p[0]=='ROUTE_REBUILD':rebuilds.append(p[1:])
        elif p and p[0]=='IDENTITY':
            if p[1] in identities:raise ValueError('Duplicate fabric identity')
            identities[p[1]]=p[2]
    if len(results)!=len(config['tasks']):raise ValueError('Missing task result')
    summary=[];mode=config['externalService']['response']
    for i,task in enumerate(config['tasks']):
        protected=task['type'] in config['authorization']['requiredTaskTypes']
        e=events[i];state,tx,rx,full,payload,met=results[i]
        expected_start=time_ns(task.get('startMs',1200+i*10))
        if abs(e['start']-expected_start)>1:raise ValueError('Incorrect task start time')
        tx,rx,full,payload,met=map(int,(tx,rx,full,payload,met))
        if protected:
            if not e['start']<=e['auth-request']<e['server-request']:raise ValueError('Invalid request order')
            if mode=='allow':
                order=['server-request','server-allow','approved','data-tx','data-rx']
                if any(e[a]>e[b] for a,b in zip(order,order[1:])):raise ValueError('Approval ordering violation')
                if e['approved']>=e['start']+time_ns(config['authorization']['timeoutMs']):raise ValueError('Late approval released data')
            else:
                expected='denied' if mode=='deny' else 'timeout'
                if state!=expected or tx or rx or 'data-tx' in e or 'data-rx' in e:raise ValueError('Blocked task leaked data')
                if any(int(p[5])==20000+i for p in wire):raise ValueError('Blocked task on fabric')
                if mode=='timeout' and e['timeout']-e['start']!=time_ns(config['authorization']['timeoutMs']):raise ValueError('Incorrect timeout')
                if mode=='deny' and not e['server-request']<=e['server-deny']<=e['denied']:raise ValueError('Invalid denial ordering')
        elif any(k.startswith('auth-') or k.startswith('server-') or k=='approved' for k in e):
            raise ValueError('Normal task incorrectly requested authorization')
        if not protected or mode=='allow':
            if state!='delivered' or tx!=1 or rx!=1:raise ValueError('Expected task not delivered')
            if full!=e['data-rx']-e['start'] or payload!=e['data-rx']-e['data-tx']:raise ValueError('Timing accounting mismatch')
            if met!=int(full<=time_ns(task['deadlineMs'])):raise ValueError('Deadline accounting mismatch')
        # No payload is permitted onto a server link, whether protected or normal.
        if any(int(p[5])==20000+i and 'main-server-model' in p[1:3] for p in wire):raise ValueError('Payload routed through server')
        summary.append({'id':task['id'],'protected':protected,'state':state,'payloadSent':tx,'payloadReceived':rx,
                        'authorizationMs':(e['approved']-e['start'])/1e6 if 'approved' in e else None,
                        'fullTaskMs':full/1e6 if rx else None,'payloadMs':payload/1e6 if rx else None,
                        'deadlineMet':bool(met)})
    server_requests=[p for p in wire if p[2]=='main-server-model' and p[5]=='8000']
    if len(server_requests)!=sum(t['type'] in config['authorization']['requiredTaskTypes'] for t in config['tasks']):raise ValueError('Missing main-server wire requests')
    if any(not p[1].startswith('spine-') for p in server_requests):raise ValueError('Server request bypassed spine')
    if identities:
        if len(set(identities.values()))!=len(identities):raise ValueError('Reused fabric identity')
        server=identities['main-server-model']
        if any(p[4]!=server for p in server_requests):raise ValueError('Request used physical server address')
        replies=[p for p in wire if p[1]=='main-server-model']
        if any(p[3]!=server for p in replies):raise ValueError('Reply changed server identity')
        if len(replies)!=(0 if mode=='timeout' else len(server_requests)):raise ValueError('Missing server reply')
    outage_audit=None
    if 'outage' in config:
        o=config['outage'];down=time_ns(o['downMs']);up=time_ns(o['upMs']);failed=o['spine']
        expected=[(failed,'down',down),(failed,'up',up)]
        if [(p[0],p[1],int(p[2])) for p in spine_events]!=expected:raise ValueError('Missing or mistimed outage events')
        link_lines=[line.split()[1:3] for line in data.decode().splitlines() if line.startswith('LINK ')]
        port_count=2*sum(failed in pair for pair in link_lines)
        if not port_count or any(int(p[3])!=port_count for p in spine_events):raise ValueError('Incomplete interface shutdown/restoration')
        if [(p[0],int(p[1])) for p in rebuilds]!=[('none',0),(failed,down),('none',up)]:raise ValueError('Missing route rebuild')
        if int(rebuilds[0][2])!=int(rebuilds[2][2]) or int(rebuilds[1][2])>=int(rebuilds[0][2]):raise ValueError('Unexpected route restoration counts')
        # This phase control has no task at an event boundary. Reject any actual
        # transmission on the failed spine during the outage, not just planned paths.
        if any(down<=int(p[7])<up and failed in p[1:3] for p in wire):raise ValueError('Packet used failed spine')
        phases=[]
        for name,lo,hi in [('before',0,down),('during',down,up),('after',up,float('inf'))]:
            packets=[p for p in wire if lo<=int(p[7])<hi]
            requests=[p for p in packets if p[2]=='main-server-model' and p[5]=='8000']
            tunnels=[p for p in packets if p[5]=='2152']
            used=sorted({node for p in requests+tunnels for node in p[1:3] if node.startswith('spine-')})
            expected_spine=next(f'spine-{i}' for i in range(config['inventory']['sharedSpines']) if name!='during' or f'spine-{i}'!=failed)
            if config['fabric']['routing']=='ecmp':
                allowed={f'spine-{i}' for i in range(config['inventory']['sharedSpines']) if name!='during' or f'spine-{i}'!=failed}
                if not used or not set(used)<=allowed or not requests or not tunnels:raise ValueError('Missing ECMP phase route evidence: '+name)
            elif used!=[expected_spine] or not requests or not tunnels:raise ValueError('Missing phase-specific server/NR route evidence: '+name)
            phases.append({'phase':name,'spines':used,'serverRequests':len(requests),'nrTunnelTransmissions':len(tunnels)})
        outage_audit={'spine':failed,'downMs':o['downMs'],'upMs':o['upMs'],'interfacesChanged':port_count,'phases':phases,
                      'model':'administrative interface outage with immediate centralized route rebuild; no boundary-loss guarantee'}
    elif spine_events:raise ValueError('Unexpected outage events')
    return {'mode':mode,'offeredTasks':len(summary),'tasks':summary,'replay':'byte-identical',
            'stableFabricIdentities':identities,'outage':outage_audit,
            'scope':'behavioral external server; no cryptographic authentication or upper-network protocol model'}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('batch',type=Path)
    args=p.parse_args()
    print(json.dumps([check(args.batch/m) for m in ('allow','deny','timeout')],indent=2))
