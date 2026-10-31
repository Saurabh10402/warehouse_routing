"""Declared deterministic wired ns-3 cases and independent raw-artifact checks."""
import copy
import csv
import heapq
import json
import math
import ipaddress
from collections import Counter, defaultdict, deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ('packets.csv', 'hops.csv', 'queues.csv', 'nodes.csv', 'summary.json')


def cases():
    base = json.loads((ROOT/'configs/warehouse_ns3_wired.json').read_text(encoding='utf-8'))
    pair = copy.deepcopy(base)
    pair.update(name='Native FIFO overload with one waiting slot', topology={'kind': 'graph'}, zones=['A'],
                nodes=[dict(id='a', role='endpoint', zone='A', access='wired', x=0, y=0),
                       dict(id='b', role='edge', access='wired', x=5, y=0)],
                links=[dict(id='ab', **{'from': 'a', 'to': 'b'}, capacityMbps=.008, delayMs=2, queuePackets=1)],
                traffic=[dict(id='flow', source='a', destinations=['b'], priority=0, bytes=100,
                              startMs=0, stopMs=3, intervalMs=1, phaseJitterMs=0)],
                simulation=dict(durationMs=3, drainMs=500, seed=1, scheduler='fifo'))
    cutoff = copy.deepcopy(pair); cutoff['simulation']['drainMs'] = 0
    disconnected = copy.deepcopy(pair); disconnected['links'] = []
    duplex = copy.deepcopy(pair)
    duplex['traffic'].append(dict(pair['traffic'][0], id='reverse', source='b', destinations=['a'], priority=3))
    diamond = copy.deepcopy(pair)
    diamond['simulation'].update(durationMs=100, drainMs=20)
    diamond['nodes'] += [dict(id=n, role='relay', access='wired', x=2, y=i) for i, n in enumerate(['slow', 'fast'])]
    diamond['links'] = [dict(id=a+b, **{'from': a, 'to': b}, capacityMbps=rate, delayMs=delay, queuePackets=8)
                        for a, b, rate, delay in [('a','slow',1,.1), ('slow','b',1,.1), ('a','fast',100,.6), ('fast','b',100,.6)]]
    diamond['traffic'] = [dict(pair['traffic'][0], id='small', bytes=1, startMs=10, stopMs=11, intervalMs=10),
                          dict(pair['traffic'][0], id='large', bytes=1400, startMs=50, stopMs=51, intervalMs=10, priority=3)]
    transit = copy.deepcopy(diamond); transit['nodes'][2].update(role='endpoint', zone='A')
    controls = [('communications', base), ('fifo-overload', pair), ('cutoff', cutoff),
                ('disconnected', disconnected), ('full-duplex', duplex), ('size-sensitive', diamond), ('endpoint-transit', transit)]
    priority = copy.deepcopy(pair)
    priority['simulation'].update(scheduler='strictPriority',durationMs=6,drainMs=1000)
    priority['links'][0]['queuePackets'] = 8
    priority['traffic'] = [dict(pair['traffic'][0],id='p'+str(i),priority=p,startMs=i,stopMs=i+1,intervalMs=10) for i,p in enumerate([3,3,0,0,1])]
    full = copy.deepcopy(priority); full['links'][0]['queuePackets'] = 1
    starved = copy.deepcopy(priority)
    starved['simulation'].update(durationMs=400,drainMs=0); starved['links'][0]['queuePackets'] = 16
    starved['traffic'] = starved['traffic'][:2]+[dict(pair['traffic'][0],id='flood',startMs=2,stopMs=400,intervalMs=100)]
    expensive = copy.deepcopy(diamond); expensive['routing']['algorithm']='ecmp'
    excluded = copy.deepcopy(transit); excluded['routing']['algorithm']='ecmp'
    controls += [('priority-order',priority),('priority-full',full),('priority-starvation',starved),
                 ('ecmp-size-sensitive',expensive),('ecmp-endpoint-transit',excluded)]
    return controls + policy_cases(base)


def policy_cases(base):
    """Predeclared three scenarios x seeds 1/2/3 x four forwarding/service policies."""
    fabric = copy.deepcopy(base)
    fabric['nodes'].append(dict(id='spine2',role='spine',access='wired',x=60,y=20))
    fabric['links'] += [dict(link,id=link['id']+'-second', **{'from': 'spine2' if link['from']=='spine' else link['from'],
                         'to': 'spine2' if link['to']=='spine' else link['to']}) for link in base['links'] if 'spine' in [link['from'],link['to']]]
    fabric['simulation'].update(durationMs=80,drainMs=1000)
    fabric['traffic'] = [dict(id=scope+'-p'+str(p), source=source,destinations=[target],priority=p,bytes=[64,128,256,1000][p],
                             startMs=p*.1+i*.01,stopMs=80,intervalMs=[4,6,8,10][p],phaseJitterMs=0)
                         for i,(scope,source,target) in enumerate([('same','A0','A1'),('cross','A0','B0'),('edge','C2','edge')]) for p in range(4)]
    runs = []
    for scenario in ['mixed','uplink-contention','destination-bottleneck']:
        for seed in [1,2,3]:
            for routing in ['dijkstra','ecmp']:
                for scheduler in ['fifo','strictPriority']:
                    config = copy.deepcopy(fabric)
                    config['simulation'].update(seed=seed,scheduler=scheduler); config['routing']['algorithm']=routing
                    config['name']='Native static policy comparison: '+scenario
                    for link in config['links']:
                        ends={link['from'],link['to']}
                        if (scenario=='uplink-contention' and ends & {'leafA','leafC'} and ends & {'spine','spine2'}) or (scenario=='destination-bottleneck' and 'edge' in ends):
                            link.update(capacityMbps=.2,queuePackets=4)
                    runs.append((f'matrix-{scenario}-s{seed}-{routing}-{scheduler}',config))
    return runs



def require(condition, message):
    if not condition:
        raise ValueError('ns-3 artifact validation: '+message)


def rows(path):
    with path.open(newline='', encoding='utf-8') as stream:
        return list(csv.DictReader(stream))


def minimum_delay(config, source, target, payload):
    nodes = {n['id']: n for n in config['nodes']}
    adjacency = defaultdict(list)
    for link in config['links']:
        weight = link['delayMs'] + (payload+30)*8/(link['capacityMbps']*1000)
        adjacency[link['from']].append((link['to'], weight))
        adjacency[link['to']].append((link['from'], weight))
    distances = {source: 0}; frontier = [(0, source)]
    while frontier:
        cost, node = heapq.heappop(frontier)
        if cost > distances[node]: continue
        if node == target: return cost
        if node != source and nodes[node]['role'] == 'endpoint': continue
        for next_node, weight in adjacency[node]:
            candidate = cost + weight
            if candidate < distances.get(next_node, math.inf):
                distances[next_node] = candidate
                heapq.heappush(frontier, (candidate, next_node))
    return math.inf


def ecmp_path(config, flow, flow_index):
    """Independent reverse-distance eligibility plus specified wire-tuple hash."""
    nodes = {n['id']:n for n in config['nodes']}
    indices = {n['id']:i for i,n in enumerate(config['nodes'])}
    source, target = flow['source'], flow['destinations'][0]
    adjacency = defaultdict(list)
    for link in config['links']:
        cost = link['delayMs']+(flow['bytes']+30)*8/(link['capacityMbps']*1000)
        for a,b in [(link['from'],link['to']),(link['to'],link['from'])]: adjacency[a].append((b,cost))
    distance = {n:minimum_delay(config,n,target,flow['bytes']) if nodes[n]['role']!='endpoint' or n in [source,target] else math.inf for n in nodes}
    if math.isinf(distance[source]): return []
    mask = (1<<64)-1; value = 14695981039346656037
    fields = [(0xac100001+indices[source],4),(0xac100001+indices[target],4),(17,1),(20000+flow_index,2),(10000+flow_index,2),(config['simulation']['seed'],4)]
    for field, width in fields:
        for byte in field.to_bytes(width,'big'): value = ((value^byte)*1099511628211)&mask
    path = [source]
    while path[-1] != target:
        node = path[-1]
        eligible = sorted((b for b,cost in adjacency[node] if distance[b]<distance[node] and abs(cost+distance[b]-distance[node])<=1e-12*max(1.,distance[node])), key=indices.get)
        require(bool(eligible), 'ECMP no eligible next hop')
        h = (value+indices[node]+0x9e3779b97f4a7c15)&mask
        h = ((h^(h>>30))*0xbf58476d1ce4e5b9)&mask
        h = ((h^(h>>27))*0x94d049bb133111eb)&mask
        h ^= h>>31
        path.append(eligible[h%len(eligible)])
        require(len(path)<=len(nodes), 'ECMP reference loop')
    return path


def validate_output(output, config):
    """Recompute counts, FIFO conservation, hop timing, paths and deadlines from traces."""
    output = Path(output)
    summary = json.loads((output/'summary.json').read_text(encoding='utf-8'))
    packets, hops, queues, positions = [rows(output/name) for name in ARTIFACTS[:4]]
    flows = {f['id']: f for f in config['traffic']}; nodes = {n['id']: n for n in config['nodes']}
    links = {link['id']: link for link in config['links']}
    require('ns-3 stationary wired' in summary['model'] and summary['scheduler'] == config['simulation']['scheduler'], 'wrong model label')
    require(summary['stationary'] is True and len(positions) == len(nodes), 'stationarity/node count')
    require({n['id'] for n in positions} == set(nodes), 'node IDs')
    for actual in positions:
        expected = nodes[actual['id']]
        require(actual['role'] == expected['role'] and actual['zone'] == expected.get('zone', ''), 'node metadata')
        require((float(actual['x']), float(actual['y']), float(actual['z'])) == (expected['x'], expected['y'], 0), 'node moved')
    offered = []
    for flow in config['traffic']:
        k = 0
        while flow.get('startMs', 0)+k*flow['intervalMs'] < flow['stopMs']:
            offered.append((flow, flow.get('startMs', 0)+k*flow['intervalMs'])); k += 1
    require(len(packets) == len(offered), 'offered packet denominator')
    flow_indices = {f['id']:i for i,f in enumerate(config['traffic'])}
    node_indices = {n['id']:i for i,n in enumerate(config['nodes'])}
    planned = {f['id']:ecmp_path(config, f, i) for i,f in enumerate(config['traffic'])} if config['routing']['algorithm']=='ecmp' else {}
    by_id = {}; statuses = Counter(); missed = 0; received_bytes = 0
    cutoff = config['simulation']['durationMs']+config['simulation'].get('drainMs', 100)
    require(summary['cutoff_ms'] == cutoff, 'measurement cutoff')
    for i, (packet, (flow, generated)) in enumerate(zip(packets, offered)):
        require(int(packet['id']) == i and packet['flow'] == flow['id'], 'packet identity/order')
        require(packet['source'] == flow['source'] and packet['destination'] == flow['destinations'][0], 'packet endpoints')
        require(int(packet['priority']) == flow['priority'] and int(packet['payload_bytes']) == flow['bytes'], 'packet class/size')
        created, finished = float(packet['created_ms']), float(packet['finished_ms'])
        require(math.isfinite(created) and math.isfinite(finished) and abs(created-generated) <= 1e-6 and created <= finished <= cutoff+1e-6, 'packet timestamps')
        status = packet['status']; require(status in {'delivered','no_route','queue_overflow','cutoff'}, 'unknown packet outcome')
        statuses[status] += 1; by_id[i] = packet
        deadline = config.get('deadlinesMs', [10,20,40,100])[flow['priority']]
        met = status == 'delivered' and finished-created <= deadline+1e-9
        require(float(packet['deadline_ms']) == deadline and int(packet['deadline_met']) == int(met), 'deadline accounting')
        missed += not met
        path = packet['observed_path'].split('>')
        require(path[0] == flow['source'] and len(path) == len(set(path)), 'observed source/path loop')
        require(all(nodes[n]['role'] != 'endpoint' for n in path[1:-1]), 'endpoint used as transit')
        if planned:
            require(packet['expected_path'] == '>'.join(planned[flow['id']]), 'ECMP transport hash/eligible next hop')
        minimum = minimum_delay(config, flow['source'], flow['destinations'][0], flow['bytes'])
        if status == 'no_route': require(math.isinf(minimum) and path == [flow['source']], 'incorrect no-route result')
        if status == 'delivered':
            require(path[-1] == flow['destinations'][0] and packet['observed_path'] == packet['expected_path'], 'delivered path')
            cost = 0
            for a, b in zip(path, path[1:]):
                candidates = [link for link in links.values() if {link['from'],link['to']} == {a,b}]
                require(len(candidates) == 1, 'path uses missing/ambiguous link')
                link = candidates[0]; cost += link['delayMs']+(flow['bytes']+30)*8/(link['capacityMbps']*1000)
            require(abs(cost-minimum) <= 1e-8, 'route is not minimum free-flow wire delay')
            require(finished-created >= cost-1e-5, 'delivery faster than physical minimum')
            received_bytes += flow['bytes']+28
    waiting = defaultdict(deque); counts = defaultdict(Counter); peaks = Counter(); paths = {i:[p['source']] for i,p in by_id.items()}
    in_flight = {}; last_time = -1.; drop_ids = set(); attempted = set(); tx_free = defaultdict(float); last_receive = {}
    for hop in hops:
        packet_id = int(hop['packet']); require(packet_id in by_id, 'unknown packet in hop trace')
        packet = by_id[packet_id]; link = links[hop['link']]
        key = (hop['link'],hop['from'],hop['to']); event = hop['event']; time = float(hop['time_ms'])
        require({hop['from'],hop['to']} == {link['from'],link['to']}, 'hop does not use declared link')
        require(math.isfinite(time) and last_time <= time <= cutoff+1e-6, 'hop chronology'); last_time = time
        require(float(packet['created_ms'])-1e-9 <= time <= float(packet['finished_ms'])+1e-9, 'hop outside packet lifetime')
        require(int(hop['wire_bytes']) == int(packet['payload_bytes'])+30, 'wrong PPP wire byte count')
        flow = flows[packet['flow']]; f = flow_indices[flow['id']]
        require(int(hop['source_port']) == 20000+f and int(hop['destination_port']) == 10000+f, 'UDP transport identity')
        require(hop['source_ipv4'] == str(ipaddress.IPv4Address(0xac100001+node_indices[flow['source']])) and
                hop['destination_ipv4'] == str(ipaddress.IPv4Address(0xac100001+node_indices[flow['destinations'][0]])), 'IPv4 transport identity')
        require(int(hop['ip_tos']) == [224,160,96,0][flow['priority']], 'wire DSCP priority mapping')
        counts[key][event] += 1
        if event == 'attempt':
            require(packet_id not in drop_ids and packet_id not in in_flight and paths[packet_id][-1] == hop['from'], 'attempt from wrong node/state')
            require((packet_id,key) not in attempted, 'duplicate per-hop attempt'); attempted.add((packet_id,key))
        elif event == 'enqueue':
            require((packet_id,key) in attempted and len(waiting[key]) < link.get('queuePackets',100), 'queue admission exceeds capacity')
            waiting[key].append(packet_id); peaks[key] = max(peaks[key],len(waiting[key]))
        elif event == 'tx':
            require(bool(waiting[key]), 'transmission from empty queue')
            chosen = waiting[key][0] if config['simulation']['scheduler']=='fifo' else min(waiting[key], key=lambda p:int(by_id[p]['priority']))
            require(chosen == packet_id and packet_id not in in_flight, 'queue service order or duplicate transmission')
            require(time >= tx_free[key]-2e-6, 'overlapping transmissions on one directional port')
            tx_free[key] = time+int(hop['wire_bytes'])*8/(link['capacityMbps']*1000)
            waiting[key].remove(packet_id); in_flight[packet_id] = (key,time)
        elif event == 'rx':
            require(packet_id in in_flight, 'receive without physical transmission')
            sent_key, sent_time = in_flight.pop(packet_id)
            require(sent_key == key, 'receive uses a different link/direction')
            physical = link['delayMs']+int(hop['wire_bytes'])*8/(link['capacityMbps']*1000)
            require(abs(time-sent_time-physical) <= 2e-6, 'physical hop delay/serialization')
            paths[packet_id].append(hop['to'])
            last_receive[packet_id] = time
        elif event == 'drop':
            require((packet_id,key) in attempted and len(waiting[key]) == link.get('queuePackets',100), 'drop without full queue')
            require(packet_id not in drop_ids and packet['status'] == 'queue_overflow', 'drop outcome mismatch'); drop_ids.add(packet_id)
        else: raise ValueError('Unknown native trace event: '+event)
    for packet_id, packet in by_id.items():
        require('>'.join(paths[packet_id]) == packet['observed_path'], 'path does not match raw receive events')
        require((packet_id in drop_ids) == (packet['status'] == 'queue_overflow'), 'drop accounting')
        queued = any(packet_id in queue for queue in waiting.values())
        if packet['status'] == 'cutoff': require(packet_id in in_flight or queued, 'censored packet has no outstanding native transmission/queue entry')
        if packet['status'] == 'delivered':
            require(packet_id not in in_flight and not queued and abs(last_receive.get(packet_id,-1)-float(packet['finished_ms'])) < 1e-9, 'receiver and final native hop disagree')
        if packet['status'] == 'no_route': require(not any(p == packet_id for p,key in attempted), 'unroutable packet reached a device queue')
    expected_keys = {(link['id'],a,b) for link in links.values() for a,b in [(link['from'],link['to']),(link['to'],link['from'])]}
    require(len(queues) == len(expected_keys) and {(q['link'],q['from'],q['to']) for q in queues} == expected_keys, 'directional queue inventory')
    for q in queues:
        key = (q['link'],q['from'],q['to']); count = counts[key]
        for field,event in [('attempts','attempt'),('admitted','enqueue'),('started','tx'),('dropped','drop')]:
            require(int(q[field]) == count[event], 'queue counter differs from raw events: '+field)
        require(count['attempt'] == count['enqueue']+count['drop'] and count['enqueue'] == count['tx']+len(waiting[key]), 'queue conservation')
        require(int(q['waiting_at_cutoff']) == len(waiting[key]) and int(q['peak_waiting']) == peaks[key], 'queue occupancy')
    require(summary['offered_packets'] == len(packets) and summary['delivered_packets'] == statuses['delivered'] and summary['deadline_missed_packets'] == missed, 'summary application counts')
    require(summary['packet_statuses'] == dict(statuses), 'summary outcome counts')
    require(summary['ip_transmitted_packets'] == len(packets)-statuses['no_route'] and summary['ip_received_packets'] == statuses['delivered'] and summary['ip_received_bytes'] == received_bytes, 'IP/application cross-check')
    require(summary['ip_unreceived_at_cutoff'] == len(packets)-statuses['no_route']-statuses['delivered'], 'IP cutoff accounting')
    return dict(offered=len(packets), delivered=statuses['delivered'], deadline_misses=missed,
                queue_drops=statuses['queue_overflow'], no_route=statuses['no_route'], cutoff=statuses['cutoff'])
