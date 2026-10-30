"""Declared capacity/admission sensitivity; independent of the saved baseline matrices."""
import copy
import csv
import statistics
from protected_queue_study import DEFAULT_POLICY, scenarios as qos_scenarios

SETTINGS = {
    'strict': None,
    'reserved-unpoliced': dict(emergencyPolicingEnabled=False),
    'rate10': dict(emergencyRateFraction=0.1),
    'rate20': {},
    'rate40': dict(emergencyRateFraction=0.4),
    'rate60': dict(emergencyRateFraction=0.6),
    'burst128': dict(emergencyBurstBytes=128),
    'burst2048': dict(emergencyBurstBytes=2048),
}
POLICIES = [(name, 'ecmp', 'strictPriority' if settings is None else 'protectedPriority')
            for name, settings in SETTINGS.items()]
CAPACITIES = [0.5, 1, 2, 5]


def scenarios(quick=False):
    previous = {name: config for name, config, _ in qos_scenarios()}
    workloads = {'mixed': previous['destination_bottleneck'],
                 'burst': previous['synchronized_emergency_burst']}
    flood = copy.deepcopy(workloads['mixed'])
    for flow in flood['traffic']:
        if flow['priority'] == 0:
            flow.update(intervalMs=0.5, phaseJitterMs=0.1)
    workloads['flood'] = flood
    cases = []
    for workload, base in workloads.items():
        for capacity in CAPACITIES:
            config = copy.deepcopy(base)
            for link in config['links']:
                if 'zone1-ep0' in (link['from'], link['to']):
                    link['capacityMbps'] = capacity
            cases.append((f'{workload}-c{capacity:g}', config, [4] if quick else [4, 5, 6]))
    return cases


def configure_policy(config, policy):
    settings = SETTINGS[policy]
    if settings is None:
        config['simulation'].pop('protectedQueues', None)
    else:
        config['simulation']['protectedQueues'] = dict(copy.deepcopy(DEFAULT_POLICY), **settings)


def endpoint_envelopes(config):
    """Source-time P0 envelope sigma + rho*t, before network queuing/jitter/loss.

    One payload per periodic flow bounds its initial phase; one-shot traffic adds
    its payload to sigma only. Ingress counts intended primary destinations.
    This is NOT a downstream arrival envelope or a guarantee of admission.
    """
    endpoints = {n['id'] for n in config['nodes'] if n['role'] == 'endpoint'}
    rows = []
    for endpoint in sorted(endpoints):
        attachment = [l for l in config['links'] if endpoint in (l['from'], l['to'])]
        if len(attachment) != 1:
            raise ValueError('Envelope study requires singly attached endpoints')
        for direction in ['outgoing', 'incoming']:
            flows = [w for w in config['traffic'] if w['priority'] == 0 and
                     (w['source'] == endpoint if direction == 'outgoing' else w['destinations'][0] == endpoint)]
            periodic = [w for w in flows if w['stopMs'] - w['startMs'] > w['intervalMs']]
            rows.append(dict(endpoint=endpoint, direction=direction,
                capacity_mbps=attachment[0]['capacityMbps'],
                p0_periodic_mbps=sum(w['bytes'] * 8 / w['intervalMs'] / 1000 for w in periodic),
                p0_source_sigma_bytes=sum(w['bytes'] for w in flows)))
    return rows


def write_tables(output, cases, class_rows, run_rows):
    """Keep all policy/capacity outcomes, including failures and lower classes."""
    operating = []
    envelopes = []
    for case, config, seeds in cases:
        workload, capacity = case.rsplit('-c', 1)
        for policy, _, scheduler in POLICIES:
            c = copy.deepcopy(config)
            c['simulation']['scheduler'] = scheduler
            configure_policy(c, policy)
            q = c['simulation'].get('protectedQueues', {})
            enabled = scheduler == 'protectedPriority' and q.get('emergencyPolicingEnabled', True)
            for envelope in endpoint_envelopes(c):
                budget = envelope['capacity_mbps'] * q['emergencyRateFraction'] if enabled else ''
                envelopes.append(dict(case=case, policy=policy, **envelope,
                    policer_mbps=budget, policer_burst_bytes=q['emergencyBurstBytes'] if enabled else '',
                    periodic_exceeds_budget=envelope['p0_periodic_mbps'] > budget if enabled else ''))
            samples = [r for r in run_rows if r['case'] == case and r['policy'] == policy]
            row = dict(case=case, workload=workload, capacity_mbps=float(capacity), policy=policy,
                       seeds=','.join(map(str, seeds)))
            for priority in range(4):
                classes = [r for r in class_rows if r['case'] == case and r['policy'] == policy and int(r['priority']) == priority]
                misses = [float(r['deadline_miss_percent']) for r in classes]
                delivery = [100 * int(r['delivered']) / int(r['sent']) for r in classes]
                row.update({f'p{priority}_miss_mean_percent': statistics.mean(misses),
                            f'p{priority}_miss_worst_seed_percent': max(misses),
                            f'p{priority}_miss_sample_sd': statistics.stdev(misses) if len(misses)>1 else '',
                            f'p{priority}_delivery_mean_percent': statistics.mean(delivery)})
            row['p0_zero_misses_all_tested_seeds'] = row['p0_miss_worst_seed_percent'] == 0
            for loss in ['priority_policed', 'reserved_capacity', 'queue_overflow']:
                row[loss+'_mean_packets'] = statistics.mean(r[loss+'_packets'] for r in samples)
            operating.append(row)
    for name, rows in [('operating_limits.csv', operating), ('endpoint_envelopes.csv', envelopes)]:
        with (output/name).open('w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, rows[0].keys()); writer.writeheader(); writer.writerows(rows)
