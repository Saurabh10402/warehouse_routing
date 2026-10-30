"""Primary powered-infrastructure study; legacy EALR experiments remain separate."""
import copy
import csv
import json
import statistics
from pathlib import Path
from generate_examples import base, node, link

ROOT = Path(__file__).resolve().parents[1]
POLICIES = [('shortest-fifo', 'dijkstra', 'fifo'),
            ('shortest-priority', 'dijkstra', 'strictPriority'),
            ('ecmp-fifo', 'ecmp', 'fifo'),
            ('ecmp-priority', 'ecmp', 'strictPriority')]


def communications(counts, all_scopes=True):
    """Explicit wired baseline: same-zone peers, cross-zone peers, and edge services.

    Wired isolation avoids confusing fabric behavior with unmodeled radio behavior.
    Coordinates and counts are inputs, not routing assumptions.
    """
    if len(counts) < 2 or any(n < 2 for n in counts):
        raise ValueError('Communication example needs at least two zones and two peers per zone')
    c = base('Powered warehouse fabric; illustrative wired communication baseline')
    c['topology'] = dict(kind='leafSpine', minimumSpinesPerLeaf=2, minimumEdges=2)
    c['routing'] = dict(algorithm='ecmp')
    for s in range(2): node(c, f'spine{s}', 'spine', s*20, 200)
    for e in range(2):
        node(c, f'edge{e}', 'edge', e*20, 300)
        for s in range(2): link(c, f'spine{s}', f'edge{e}', 1000, 0.05)
    for z, count in enumerate(counts):
        zone = f'zone{z}'; leaf = f'leaf{z}'; c['zones'].append(zone)
        node(c, leaf, 'leaf', z*20, 100, zone)
        for s in range(2): link(c, leaf, f'spine{s}', 1000, 0.05)
        for e in range(count):
            endpoint = f'{zone}-ep{e}'
            node(c, endpoint, 'endpoint', z*20+e%10, e//10, zone)
            link(c, endpoint, leaf, 100, 0.1)
            next_zone = (z+1) % len(counts)
            destinations = [(f'{zone}-ep{(e+1)%count}',),
                            (f'zone{next_zone}-ep{e%counts[next_zone]}',), ('edge0', 'edge1')]
            for scope, dest in enumerate(destinations):
                if not all_scopes and scope != e%3: continue
                c['traffic'].append(dict(id=f'flow-{endpoint}-{scope}', source=endpoint,
                    destinations=list(dest), priority=e%4, bytes=[96,256,512,1200][e%4],
                    startMs=0, stopMs=1000, intervalMs=200, phaseJitterMs=20))
    return c


def scenarios(quick=False):
    cases = []
    def add(name, config): cases.append((name, config, [1] if quick else [1,2,3]))
    baseline = json.loads((ROOT/'configs/warehouse_communications.json').read_text(encoding='utf-8'))
    add('communications', baseline)
    for name, scopes in [('same_zone_only', {0}), ('cross_zone_only', {1})]:
        c = copy.deepcopy(baseline)
        c['traffic'] = [w for w in c['traffic'] if int(w['id'].rsplit('-',1)[1]) in scopes]
        add(name, c)
    busy = communications([8,8])
    busy['simulation'].update(durationMs=200, drainMs=200)
    for w in busy['traffic']:
        w.update(stopMs=200, intervalMs=[10,10,5,4][w['priority']], phaseJitterMs=1)
    for l in busy['links']:
        if l['from'].startswith('leaf') and l['to'].startswith('spine'):
            l.update(capacityMbps=10, queuePackets=32)
    add('uplink_contention', busy)
    c = copy.deepcopy(busy)
    for l in c['links']:
        if l['from']=='zone1-ep0': l.update(capacityMbps=1, queuePackets=32)
    for w in c['traffic']:
        if w['source'].startswith('zone0') and w['id'].endswith('-1'):
            w['destinations']=['zone1-ep0']
    add('destination_bottleneck', c)
    for name, events in [
        ('spine_failure', [dict(type='node', target='spine0', timeMs=300, available=False),
                           dict(type='node', target='spine0', timeMs=700, available=True)]),
        ('zone_switch_failure', [dict(type='node', target='leaf0', timeMs=300, available=False),
                                dict(type='node', target='leaf0', timeMs=700, available=True)]),
        ('edge_failure', [dict(type='node', target='edge0', timeMs=300, available=False)]),
        ('all_spines_down', [dict(type='node', target=f'spine{s}', timeMs=300, available=False) for s in range(2)])]:
        c = copy.deepcopy(baseline); c['events'] = events; add(name, c)
    if not quick:
        add('uneven_communications', communications([10,35,75,20], all_scopes=False))
        add('reference_500_communications', communications([50]*10, all_scopes=False))
    return cases


def scope_metrics(directory, config):
    """Class/scope denominators include losses, even when no destination was reachable."""
    nodes = {n['id']:n for n in config['nodes']}
    flows = {w['id']:w for w in config['traffic']}
    with (directory/'packets.csv').open(newline='', encoding='utf-8') as f:
        packets = list(csv.DictReader(f))
    groups = {}
    for p in packets:
        if p['flow'] not in flows:
            scope = 'safety_event'
        else:
            # Use intended primary destination so failover/loss does not change group.
            w = flows[p['flow']]; source = nodes[w['source']]; target = nodes[w['destinations'][0]]
            scope = ('edge_service' if target['role']=='edge' else
                     'same_zone' if source['zone']==target['zone'] else 'cross_zone')
        groups.setdefault((scope, int(p['priority'])), []).append(p)
    rows = []
    for (scope, priority), values in sorted(groups.items()):
        delays = sorted(float(p['latency_ms']) for p in values if p['status']=='delivered')
        sent = len(values); delivered = len(delays)
        misses = sent-delivered+sum(d>=config['deadlinesMs'][priority] for d in delays)
        rows.append(dict(scope=scope, priority=priority, sent=sent, delivered=delivered,
            lost=sent-delivered, mean_ms=statistics.mean(delays) if delays else '',
            delivery_percent=100*delivered/sent, deadline_miss_percent=100*misses/sent,
            queue_overflow=sum(p['status']=='queue_overflow' for p in values),
            reserved_capacity=sum(p['status']=='reserved_capacity' for p in values),
            priority_policed=sum(p['status']=='priority_policed' for p in values)))
    return rows


if __name__ == '__main__':
    (ROOT/'configs/warehouse_communications.json').write_text(
        json.dumps(communications([4,8]), indent=2)+'\n', encoding='utf-8')
