"""Declared single-switch zone isolation demonstration and acceptance report."""
import csv
from collections import Counter


def configure(config, zone):
    switches=[n['id'] for n in config['nodes'] if n['zone']==zone and n['role']=='leaf']
    if len(switches)!=1:
        raise ValueError('Zone outage requires a known zone with exactly one switch')
    target=switches[0]
    config['events']=[dict(type='node',target=target,timeMs=30,available=False),
                      dict(type='node',target=target,timeMs=70,available=True)]
    return target


def report(directory, config, zone):
    nodes={n['id']:n for n in config['nodes']}
    affected={f['id'] for f in config['traffic'] if nodes[f['source']]['zone']==zone or
              all(nodes[d]['zone']==zone for d in f['destinations'])}
    with (directory/'packets.csv').open(newline='',encoding='utf-8') as stream:
        packets=list(csv.DictReader(stream))
    groups={phase:{kind:[] for kind in ['affected','unaffected']} for phase in ['before','outage','recovered']}
    for p in packets:
        t=float(p['created_ms']);phase='before' if t<30 else 'outage' if t<70 else 'recovered'
        kind='affected' if p['flow'] in affected else 'unaffected'
        groups[phase][kind].append(p)
        expected='no_route' if phase=='outage' and kind=='affected' else 'delivered'
        if p['status']!=expected:raise ValueError('Unexpected zone-outage outcome: '+p['id']+' expected '+expected)
        if expected=='no_route' and p['deadline_met']!='0':raise ValueError('Lost packet omitted from deadline misses')
    for phase,kinds in groups.items():
        if not all(kinds.values()):raise ValueError('Demonstration needs affected and unaffected samples in every phase')
    for flow in {p['flow'] for p in packets}:
        if not all(any(p['flow']==flow for rows in groups[phase].values() for p in rows) for phase in groups):
            raise ValueError('Missing before/outage/recovery samples for '+flow)
    return dict(zone=zone,down_ms=30,up_ms=70,affected_flows=sorted(affected),
        phases={phase:{kind:dict(offered=len(rows),delivered=sum(p['status']=='delivered' for p in rows),
            deadline_misses=sum(p['deadline_met']=='0' for p in rows),statuses=dict(Counter(p['status'] for p in rows)))
            for kind,rows in kinds.items()} for phase,kinds in groups.items()},
        scope='Single-switch zone isolation; immediate centralized updates; spaced samples, not hardware convergence. Lost packets remain in offered/deadline denominators.')
