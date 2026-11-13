"""Independently audit ECMP choices against physical wire traces and topology."""
import ipaddress,json
from collections import defaultdict,deque
from pathlib import Path
from access_study import time_ns
from wireless_layout import expand


def flow_hash(node,source,destination,protocol=17):
    data=node.encode()+b'\0'+ipaddress.ip_address(source).packed+ipaddress.ip_address(destination).packed+bytes([protocol])
    h=2166136261
    for byte in data:h=((h^byte)*16777619)&0xffffffff
    h=((h^(h>>16))*0x7feb352d)&0xffffffff
    h=((h^(h>>15))*0x846ca68b)&0xffffffff
    return h^(h>>16)


def check_ecmp(folder):
    folder=Path(folder);config=json.loads((folder/'input.json').read_text())
    if config['fabric']['routing']!='ecmp':raise ValueError('Expected ECMP configuration')
    devices={d['id']:d for d in expand(config['inventory'])['devices']}
    graph=defaultdict(set);owners={};names={};wires=[]
    for line in (folder/'original.log').read_text().splitlines():
        p=line.split()
        if not p:continue
        if p[0]=='LINK':graph[p[1]].add(p[2]);graph[p[2]].add(p[1])
        elif p[0]=='ECMP_DEST':
            if p[2] in owners or p[1] in names:raise ValueError('Duplicate destination identity')
            expected=p[1]
            if p[1] in devices and devices[p[1]]['role']=='robot':
                d=devices[p[1]];expected='core-pgw' if d['access']=='nr' else d['attachment']
            if p[3]!=expected:raise ValueError('Wrong destination owner')
            owners[p[2]]=p[3];names[p[1]]=p[2]
        elif p[0]=='WIRE':wires.append(p)
    if not owners:raise ValueError('Missing ECMP destination map')
    outage=config.get('outage');observations=0;multiple=0;skipped=0
    groups=defaultdict(list);used=defaultdict(set);phase_flows={}
    for p in wires:
        node,next_node,source,destination=p[1:5];now=int(p[7])
        if source not in owners:raise ValueError('Unassigned ECMP packet source address: '+source)
        if destination not in owners:skipped+=1;continue
        phase='static';failed=None
        if outage:
            down=time_ns(outage['downMs']);up=time_ns(outage['upMs'])
            phase='before' if now<down else 'during' if now<up else 'after'
            if phase=='during':failed=outage['spine']
        if node==failed or next_node==failed:raise ValueError('Packet used failed spine')
        owner=owners[destination]
        if node==owner:raise ValueError('Unexpected fabric hop from destination owner')
        allowed={v for v in graph if v!=failed and (v in (node,owner) or (not v.startswith('core-') and v!='main-server-model'))}
        # Reverse distances are computed independently of native per-first-hop searches.
        distance={owner:0};queue=deque([owner])
        while queue:
            current=queue.popleft()
            for other in graph[current]&allowed:
                if other not in distance:distance[other]=distance[current]+1;queue.append(other)
        if node not in distance:raise ValueError('Disconnected observed source')
        choices=sorted(v for v in graph[node]&allowed if distance.get(v,-2)==distance[node]-1)
        if not choices:raise ValueError('No shortest next hop')
        chosen=choices[flow_hash(node,source,destination)%len(choices)]
        if next_node!=chosen:raise ValueError('Wire next hop differs from equal-cost flow hash')
        observations+=1
        if len(choices)>1:multiple+=1
        key=(node,source,destination)
        groups[(phase,key,tuple(choices))].append(next_node)
        phase_flows[(phase,key)]=(next_node,tuple(choices))
        if next_node.startswith('spine-'):used[phase].add(next_node)
    if skipped:raise ValueError('Unmapped fabric packet destination')
    if not observations or not multiple:raise ValueError('No actual multipath choice exercised')
    repeated=sum(len(hops)>1 and len(key[2])>1 for key,hops in groups.items())
    if not repeated or any(len(set(hops))!=1 for hops in groups.values()):raise ValueError('Missing/inconsistent repeated flow')
    if len(set().union(*used.values()))<2:raise ValueError('Both spines not exercised')
    restored=0
    if outage:
        for (phase,key),value in phase_flows.items():
            if phase=='before' and ('after',key) in phase_flows:
                if value!=phase_flows[('after',key)]:raise ValueError('Flow mapping changed after restoration')
                restored+=1
        if not restored:raise ValueError('No same-flow restoration evidence')
        if len(used['before'])<2 or len(used['after'])<2:raise ValueError('Missing pre/post outage path diversity')
    return {'hash':'node + source/destination IPv4 + IP protocol; FNV1a32 with avalanche; sorted shortest-hop candidates',
            'wireHopsChecked':observations,'unmappedWireHops':skipped,'equalCostSelections':multiple,
            'repeatedFlowGroups':repeated,'restoredFlowMappings':restored,
            'spinesByPhase':{key:sorted(value) for key,value in used.items()},
            'scope':'Layer-3 per-flow ECMP; outer GTP tunnels remain grouped; not per-task load balancing'}
