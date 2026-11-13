"""Audit measured task outcomes; never equate accounting validity with delivery."""
import json
from collections import defaultdict
from pathlib import Path
from access_study import time_ns, validate


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_outcomes(folder):
    folder = Path(folder)
    config = json.loads((folder/'input.json').read_text(encoding='utf-8'))
    validate(config)
    raw = (folder/'original.log').read_bytes()
    require(raw == (folder/'replay.log').read_bytes(), 'Trace replay differs')
    require((folder/'original.csv').read_bytes() == (folder/'replay.csv').read_bytes(), 'Device replay differs')
    exits = [int((folder/(label+'.exit')).read_text()) for label in ('original', 'replay')]
    require(exits[0] == exits[1] and exits[0] in (0, 2), 'Unexpected native exit')
    events, results, identities = defaultdict(dict), {}, {}
    wires, drops, changes, rebuilds, links = [], [], [], [], []
    allowed_events = {'start', 'auth-request', 'server-request', 'server-allow', 'server-deny',
                      'approved', 'denied', 'timeout', 'ignored-response', 'data-tx', 'data-rx'}
    for line in raw.decode().splitlines():
        p = line.split()
        if not p:
            continue
        if p[0] == 'TASK_EVENT':
            i = int(p[1])
            require(p[2] in allowed_events and p[2] not in events[i], 'Unknown or duplicate task event')
            events[i][p[2]] = int(p[3])
        elif p[0] == 'TASK_RESULT':
            i = int(p[1])
            require(i not in results, 'Duplicate task result')
            results[i] = p[2:]
        elif p[0] == 'IDENTITY':
            require(p[1] not in identities, 'Duplicate identity')
            identities[p[1]] = p[2]
        elif p[0] == 'WIRE': wires.append(p)
        elif p[0] == 'IP_DROP': drops.append(p)
        elif p[0] == 'SPINE_EVENT': changes.append(p)
        elif p[0] == 'ROUTE_REBUILD': rebuilds.append(p)
        elif p[0] == 'LINK': links.append(p[1:3])
    n = len(config['tasks'])
    require(set(events) == set(results) == set(range(n)), 'Missing or extra task accounting')
    require('main-server-model' in identities and len(set(identities.values())) == len(identities), 'Invalid fabric identities')
    server = identities['main-server-model']
    for p in wires:
        if p[2] == 'main-server-model':
            require(p[5] == '8000' and p[4] == server and p[1].startswith('spine-'), 'Invalid server request boundary')
        if p[1] == 'main-server-model':
            require(p[3] == server and p[2].startswith('spine-'), 'Invalid server reply boundary')
            i = int(p[5])-10000
            require(0 <= i < n and ('server-allow' in events[i] or 'server-deny' in events[i]), 'Unmatched server reply')
    outage = config.get('outage')
    if outage:
        down, up, failed = time_ns(outage['downMs']), time_ns(outage['upMs']), outage['spine']
        require([(p[1], p[2], int(p[3])) for p in changes] == [(failed, 'down', down), (failed, 'up', up)], 'Outage schedule mismatch')
        ports = 2*sum(failed in pair for pair in links)
        require(ports > 0 and all(int(p[4]) == ports for p in changes), 'Incomplete interface event')
        require([(p[1], int(p[2])) for p in rebuilds] == [('none', 0), (failed, down), ('none', up)], 'Missing route rebuild')
        require(rebuilds[0][3] == rebuilds[2][3] and int(rebuilds[1][3]) < int(rebuilds[0][3]), 'Routes not restored')
        require(not any(down <= int(p[7]) < up and failed in p[1:3] for p in wires), 'Transmission used failed spine')
    else:
        require(not changes and [(p[1], int(p[2])) for p in rebuilds] == [('none', 0)], 'Unexpected outage events')
    mode = config['externalService']['response']
    timeout = time_ns(config['authorization']['timeoutMs'])
    processing = time_ns(config['externalService']['processingDelayMs'])
    last_ms = max([t.get('startMs', 1200+10*i) for i, t in enumerate(config['tasks'])] + [outage['upMs'] if outage else 0])
    stop = time_ns(last_ms+2000+config['authorization']['timeoutMs']+2*config['externalService']['oneWayDelayMs']+config['externalService']['processingDelayMs'])
    counts = dict(delivered=0, authorizationDenied=0, authorizationTimeout=0,
                  payloadUnreceivedByStop=0, payloadSendFailed=0, observedPayloadDrops=0,
                  deadlineMet=0, deadlineMissed=0)
    summary, contract_passed = [], True
    for i, task in enumerate(config['tasks']):
        e = events[i]
        state, *values = results[i]
        require(len(values) == 5, 'Malformed task result')
        tx, rx, full, payload, met = map(int, values)
        start = time_ns(task.get('startMs', 1200+10*i))
        protected = task['type'] in config['authorization']['requiredTaskTypes']
        require(e.get('start') == start and all(start <= t <= stop+1 for t in e.values()), 'Wrong task event time')
        require(tx in (0, 1) and rx in (0, 1) and met in (0, 1) and rx <= tx, 'Invalid task counters')
        require(not ('timeout' in e and ('approved' in e or 'denied' in e)), 'Contradictory authorization terminal events')
        require(not ('approved' in e and 'denied' in e), 'Conflicting authorization decisions')
        if protected:
            require(e.get('auth-request') == start, 'Missing authorization request')
            if 'server-request' in e:
                require(e['server-request'] > start, 'Invalid server request order')
                decision = 'server-'+mode
                if mode != 'timeout' and e['server-request']+processing <= stop:
                    require(decision in e, 'Missing scheduled server decision')
            for decision in ('server-allow', 'server-deny'):
                if decision in e:
                    require(decision == 'server-'+mode and 'server-request' in e and
                            e[decision]-e['server-request'] == processing, 'Invalid server decision')
            for client, decision, expected_mode in [('approved', 'server-allow', 'allow'), ('denied', 'server-deny', 'deny')]:
                if client in e:
                    require(mode == expected_mode and decision in e and e[decision] <= e[client] < start+timeout, 'Late or invalid client decision')
            if 'timeout' in e:
                require(e['timeout'] == start+timeout, 'Incorrect authorization timeout')
            if 'ignored-response' in e:
                decision = 'server-'+mode
                require('timeout' in e and decision in e and e['ignored-response'] >= max(e['timeout'], e[decision]), 'Invalid ignored late response')
            if 'data-tx' in e:
                require('approved' in e and e['data-tx'] == e['approved'], 'Unauthorized payload attempt')
            if 'approved' in e:
                require('data-tx' in e, 'Approved task never attempted payload')
        else:
            require(set(e) <= {'start', 'data-tx', 'data-rx'} and e.get('data-tx') == start, 'Normal task requested approval or did not send')
        task_wires = [p for p in wires if int(p[5]) == 20000+i]
        task_drops = [p for p in drops if int(p[5]) == 20000+i]
        require(not any('main-server-model' in p[1:3] for p in task_wires), 'Payload reached server')
        require(not task_wires or ('data-tx' in e and tx == 1 and all(int(p[7]) >= e['data-tx'] for p in task_wires)), 'Payload wire trace without send')
        if rx:
            require(not task_drops, 'Delivered task also has terminal payload drop')
            require(state == 'delivered' and 'data-rx' in e and 'data-tx' in e and e['data-rx'] >= e['data-tx'], 'Invalid delivery state')
            require(full == e['data-rx']-start and payload == e['data-rx']-e['data-tx'], 'Delivery timing mismatch')
            require(met == int(full <= time_ns(task['deadlineMs'])), 'Incorrect deadline result')
            counts['delivered'] += 1
        else:
            require(full == payload == -1 and met == 0 and 'data-rx' not in e, 'Unreceived task hidden from deadline misses')
            if 'data-tx' in e:
                require(state == 'sent', 'Unreceived payload state mismatch')
                counts['payloadUnreceivedByStop' if tx else 'payloadSendFailed'] += 1
                if task_drops: counts['observedPayloadDrops'] += 1
            else:
                require(protected and tx == 0 and not task_wires, 'Blocked task leaked payload')
                require(state in ('timeout', 'denied') and state in e, 'Missing blocked task outcome')
                counts['authorizationTimeout' if state == 'timeout' else 'authorizationDenied'] += 1
        counts['deadlineMet' if met else 'deadlineMissed'] += 1
        expected = (rx == 1) if not protected or mode == 'allow' else (tx == rx == 0 and state == ('denied' if mode == 'deny' else 'timeout'))
        contract_passed = contract_passed and expected
        summary.append(dict(id=task['id'], state=state, protected=protected, payloadSent=tx, payloadReceived=rx,
                            fullTaskMs=full/1e6 if rx else None, payloadMs=payload/1e6 if rx else None,
                            authorizationMs=(e['approved']-start)/1e6 if 'approved' in e else None,
                            deadlineMet=bool(met), ignoredLateResponse='ignored-response' in e,
                            observedPayloadDrops=len(task_drops)))
    require(exits[0] == (0 if contract_passed else 2), 'Native delivery gate inconsistent')
    require(sum(counts[k] for k in ('delivered', 'authorizationDenied', 'authorizationTimeout', 'payloadUnreceivedByStop', 'payloadSendFailed')) == n, 'Outcome denominator mismatch')
    require(counts['deadlineMet']+counts['deadlineMissed'] == n, 'Deadline denominator mismatch')
    return dict(offeredTasks=n, replay='byte-identical', nativeExit=exits[0], outcomes=counts, tasks=summary,
                deliveryControlPassed=contract_passed, allTasksDelivered=counts['delivered'] == n,
                allDeadlinesMet=counts['deadlineMet'] == n,
                scope='Outcome accounting and replay only; unreceived is not proof of a particular radio drop or cause')
