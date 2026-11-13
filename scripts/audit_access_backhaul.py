"""Audit traced fabric transmissions in the small backhaul probe (not radio hops)."""
import argparse
from collections import defaultdict
import json
from pathlib import Path


def audit(text):
    packets=defaultdict(list);links=set();flows=[]
    for line in text.splitlines():
        parts=line.split()
        if parts and parts[0]=='LINK':links.add(frozenset(parts[1:3]))
        if parts and parts[0]=='WIRE':
            _,a,b,source,dest,port,ident,time=parts
            if port=='2152':packets[(source,dest,ident)].append((int(time),a,b))
        if parts and parts[0]=='FLOW':
            flows.append(int(parts[-1].split('=')[1].split('/')[0]))
    if not packets or not flows:raise ValueError('Missing tunnel traces or flow results')
    routes=set()
    for records in packets.values():
        ordered=sorted(records)
        path=[ordered[0][1]]
        for _,a,b in ordered:
            if a!=path[-1] or frozenset((a,b)) not in links:raise ValueError('Broken tunnel trace')
            path.append(b)
        # This probe requires every traced S1 datagram to traverse all three wires.
        if len(path)!=4:raise ValueError('Expected gNB-gateway-spine-SGW tunnel path')
        forward=path if path[-1]=='core-sgw' else path[::-1]
        if not ('-gnb-' in forward[0] and '-switch-' in forward[1] and forward[2].startswith('spine-') and forward[3]=='core-sgw'):
            raise ValueError('Unexpected tunnel path')
        routes.add(tuple(path))
    return {'scope':'fabric transmission audit only; not full application/radio path audit',
            'tracedS1Datagrams':len(packets),'s1Paths':[list(p) for p in sorted(routes)],
            'flows':len(flows),'offered':10*len(flows),'received':sum(flows),
            'allDelivered':all(n==10 for n in flows)}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path)
    args=p.parse_args();print(json.dumps(audit(args.log.read_text()),indent=2))
