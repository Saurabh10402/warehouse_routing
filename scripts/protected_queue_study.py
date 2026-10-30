"""Prespecified queue-policy evaluation; preserves the four-policy warehouse baseline."""
import copy
import json
from warehouse_study import ROOT, POLICIES as BASELINE_POLICIES, scenarios as baseline_scenarios

POLICIES = BASELINE_POLICIES + [('shortest-protected', 'dijkstra', 'protectedPriority'),
                               ('ecmp-protected', 'ecmp', 'protectedPriority')]
DEFAULT_POLICY = dict(reservedEmergencyPackets=8, emergencyBurstBytes=512,
                      emergencyRateFraction=0.2, lowerClassWeights=[4,2,1])

def scenarios(quick=False):
    baseline={name:(config,seeds) for name,config,seeds in baseline_scenarios(quick)}
    names=['communications','uplink_contention','destination_bottleneck','spine_failure']
    cases=[(name,*baseline[name]) for name in names]
    seeds=[1] if quick else [1,2,3]
    c=copy.deepcopy(baseline['uplink_contention'][0])
    for w in c['traffic']:
        if w['priority']==0:w.update(intervalMs=0.5,phaseJitterMs=0.1)
    cases.append(('emergency_overload',c,seeds))
    c=copy.deepcopy(baseline['destination_bottleneck'][0])
    for source in range(8):
        for burst in range(4):
            c['traffic'].append(dict(id=f'burst-{source}-{burst}',source=f'zone0-ep{source}',
                destinations=['zone1-ep0'],priority=0,bytes=96,startMs=50,stopMs=50.1,
                intervalMs=10,phaseJitterMs=0))
    cases.append(('synchronized_emergency_burst',c,seeds))
    # One-factor ablations: traffic and topology unchanged from destination_bottleneck.
    for name,settings in [('no_reserved_buffer',dict(reservedEmergencyPackets=0)),
                          ('equal_lower_class_weights',dict(lowerClassWeights=[1,1,1]))]:
        c=copy.deepcopy(baseline['destination_bottleneck'][0])
        c['simulation']['protectedQueues']=dict(DEFAULT_POLICY,**settings)
        cases.append((name,c,seeds))
    return cases

def configure_policy(config, scheduler):
    if scheduler=='protectedPriority':
        config['simulation'].setdefault('protectedQueues',copy.deepcopy(DEFAULT_POLICY))
    else:
        config['simulation'].pop('protectedQueues',None)

if __name__=='__main__':
    c=copy.deepcopy(scenarios()[2][1]);c['simulation']['scheduler']='protectedPriority'
    c['routing']['algorithm']='ecmp';configure_policy(c,'protectedPriority')
    (ROOT/'configs/warehouse_protected.json').write_text(json.dumps(c,indent=2)+'\n',encoding='utf-8')
