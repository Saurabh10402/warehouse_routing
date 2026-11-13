"""Audit transition losses without treating missing data as successful delivery."""
import json
from collections import defaultdict
from pathlib import Path
from access_study import time_ns


def check_boundary(folder):
    folder=Path(folder)
    config=json.loads((folder/'input.json').read_text())
    data=(folder/'original.log').read_bytes()
    if data!=(folder/'replay.log').read_bytes():raise ValueError('Trace replay differs')
    if (folder/'original.csv').read_bytes()!=(folder/'replay.csv').read_bytes():raise ValueError('Device replay differs')
    exits=[int((folder/(r+'.exit')).read_text()) for r in ('original','replay')]
    if exits[0]!=exits[1] or exits[0] not in (0,2):raise ValueError('Unexpected native exit')
    events=defaultdict(dict);results={};wires=[];drops=[];changes=[];rebuilds=[];identities={}
    for line in data.decode().splitlines():
        p=line.split()
        if not p:continue
        if p[0]=='TASK_EVENT':
            i=int(p[1])
            if p[2] in events[i]:raise ValueError('Duplicate task event')
            events[i][p[2]]=int(p[3])
        elif p[0]=='TASK_RESULT':
            if int(p[1]) in results:raise ValueError('Duplicate task result')
            results[int(p[1])]=p[2:]
        elif p[0]=='WIRE':wires.append(p)
        elif p[0]=='IP_DROP':drops.append(p)
        elif p[0]=='SPINE_EVENT':changes.append(p)
        elif p[0]=='ROUTE_REBUILD':rebuilds.append(p)
        elif p[0]=='IDENTITY':identities[p[1]]=p[2]
    n=len(config['tasks'])
    if set(results)!=set(range(n)) or set(events)!=set(range(n)):raise ValueError('Missing/extra task accounting')
    outage=config['outage'];down=time_ns(outage['downMs']);up=time_ns(outage['upMs'])
    if [(p[1],p[2],int(p[3])) for p in changes]!=[(outage['spine'],'down',down),(outage['spine'],'up',up)]:raise ValueError('Outage schedule mismatch')
    links=[line.split()[1:3] for line in data.decode().splitlines() if line.startswith('LINK ')]
    ports=2*sum(outage['spine'] in pair for pair in links)
    if not ports or any(int(p[4])!=ports for p in changes):raise ValueError('Incomplete interface event')
    if [(p[1],int(p[2])) for p in rebuilds]!=[('none',0),(outage['spine'],down),('none',up)]:raise ValueError('Missing route rebuild')
    if rebuilds[0][3]!=rebuilds[2][3]:raise ValueError('Routes not restored')
    server=identities['main-server-model']
    if any(p[2]=='main-server-model' and (p[5]!='8000' or p[4]!=server or not p[1].startswith('spine-')) for p in wires):raise ValueError('Invalid server request boundary')
    if any(p[1]=='main-server-model' and p[3]!=server for p in wires):raise ValueError('Server identity changed')
    counts={'delivered':0,'authorizationTimeout':0,'payloadUnreceivedByStop':0,'observedPayloadDrops':0,'deadlineMet':0,'deadlineMissed':0}
    summary=[]
    for i,task in enumerate(config['tasks']):
        e=events[i];state,*values=results[i];tx,rx,full,payload,met=map(int,values)
        protected=task['type'] in config['authorization']['requiredTaskTypes']
        if abs(e['start']-time_ns(task.get('startMs',1200+10*i)))>1:raise ValueError('Wrong task start')
        if tx not in (0,1) or rx not in (0,1) or rx>tx:raise ValueError('Invalid payload counts')
        task_drops=[p for p in drops if int(p[5])==20000+i]
        auth_drops=[p for p in drops if (int(p[4])==10000+i and p[5]=='8000') or int(p[5])==10000+i]
        if protected:
            if e.get('auth-request')!=e['start']:raise ValueError('Missing authorization request')
            if 'server-request' in e and e['server-request']<=e['auth-request']:raise ValueError('Server request order')
            if 'server-allow' in e and not e['server-request']<=e['server-allow']:raise ValueError('Server approval order')
            if tx:
                if not e['server-allow']<=e['approved']<=e['data-tx'] or e['approved']>=e['start']+time_ns(config['authorization']['timeoutMs']):raise ValueError('Unauthorized payload')
            elif state!='timeout' or e.get('timeout')!=e['start']+time_ns(config['authorization']['timeoutMs']) or 'approved' in e:raise ValueError('Incorrect blocked task')
        elif any(k in e for k in ('auth-request','server-request','approved','timeout')):raise ValueError('Normal task requested approval')
        if any(int(p[5])==20000+i and 'main-server-model' in p[1:3] for p in wires):raise ValueError('Payload reached server')
        if rx:
            if state!='delivered' or full!=e['data-rx']-e['start'] or payload!=e['data-rx']-e['data-tx']:raise ValueError('Delivery accounting mismatch')
            if met!=int(full<=time_ns(task['deadlineMs'])) or task_drops:raise ValueError('Incorrect delivery/deadline/drop')
            counts['delivered']+=1
        else:
            if full!=-1 or payload!=-1 or met!=0 or 'data-rx' in e:raise ValueError('Unreceived task hidden from deadline misses')
            if tx:
                if state!='sent' or 'data-tx' not in e:raise ValueError('Incorrect unreceived payload state')
                counts['payloadUnreceivedByStop']+=1
                if task_drops:counts['observedPayloadDrops']+=1
            else:
                if not protected or 'data-tx' in e or any(int(p[5])==20000+i for p in wires):raise ValueError('Blocked task leaked payload')
                if not auth_drops:raise ValueError('Timeout lacks observed authorization loss')
                counts['authorizationTimeout']+=1
        counts['deadlineMet' if met else 'deadlineMissed']+=1
        summary.append({'id':task['id'],'state':state,'payloadSent':tx,'payloadReceived':rx,'protected':protected,
                        'fullTaskMs':full/1e6 if rx else None,'deadlineMet':bool(met),
                        'observedPayloadDrops':len(task_drops),'observedAuthorizationDrops':len(auth_drops)})
    if exits[0]!=(0 if counts['delivered']==n else 2):raise ValueError('Native delivery gate inconsistent')
    inflight=[]
    for p in drops:
        if p[8]!='interface-down' or not down<=int(p[7])<up:raise ValueError('Unexpected packet drop')
        matches=[w for w in wires if w[2]==p[1] and w[3:5]==p[2:4] and w[5]==p[5] and w[6]==p[6] and int(w[7])<down<int(p[7])]
        if not matches:raise ValueError('No pre-outage transmission for observed loss')
        inflight.append({'node':p[1],'sourcePort':int(p[4]),'destinationPort':int(p[5]),'transmitNs':int(matches[-1][7]),'dropNs':int(p[7])})
    if not counts['observedPayloadDrops']:raise ValueError('No observed payload boundary loss exercised')
    if not inflight or not counts['authorizationTimeout']:raise ValueError('No actual authorization boundary loss exercised')
    restoration=[i for i,row in enumerate(summary) if row['payloadReceived'] and row['protected'] and events[i].get('auth-request',up)>=0 and events[i].get('auth-request',up)<up<events[i].get('server-request',0)]
    if not restoration:raise ValueError('No authorization request exercised across restoration')
    for lo,hi in [(0,down),(down,up),(up,float('inf'))]:
        if not any(row['payloadReceived'] and lo<=events[i]['start']<hi for i,row in enumerate(summary)):raise ValueError('Missing healthy phase delivery')
    if counts['deadlineMet']+counts['deadlineMissed']!=n:raise ValueError('Offered denominator mismatch')
    return {'offeredTasks':n,'replay':'byte-identical','nativeExit':exits[0],'outcomes':counts,'tasks':summary,
            'restorationCrossingTasks':restoration,'inFlightInterfaceDrops':inflight,'scope':'transition accounting control; PASS does not mean all-delivered; immediate centralized routing'}
