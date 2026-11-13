"""Validate the agreed access-routing design; does not execute authorization."""
import argparse
import json
import math
from pathlib import Path
from wireless_layout import expand


def time_ns(milliseconds):
    """Match native ControlMs: nearest nanosecond, positive half-ties upward."""
    return int(math.floor(milliseconds*1e6+0.5))


def validate(c):
    def fields(obj, names):
        if not isinstance(obj, dict) or set(obj) != set(names.split()):
            raise ValueError('Expected fields: '+names)
    def positive(value):
        if type(value) not in (int,float) or not math.isfinite(value) or value <= 0:
            raise ValueError('Timing must be finite and positive')
    fields(c,'schema implementationStatus inventory fabric cellularSupport authorization externalService tasks'+(' outage' if 'outage' in c else '')+(' radioRandomness' if 'radioRandomness' in c else ''))
    if 'radioRandomness' in c:
        fields(c['radioRandomness'],'seed run')
        if any(type(value) is not int or not 1<=value<=2147483647 for value in c['radioRandomness'].values()):
            raise ValueError('Radio seed/run must be integers between 1 and 2147483647')
    if c['schema']!='warehouse-access-study-v1' or c['implementationStatus']!='design-only':
        raise ValueError('Unsupported design contract')
    inventory=expand(c['inventory'])
    if any(z['edgeServers'] for z in c['inventory']['zones']):
        raise ValueError('Zone edge servers are outside the agreed scope')
    if c['inventory']['sharedSpines']<2:
        raise ValueError('Redundant spines required')
    zones={z['id'] for z in c['inventory']['zones']}
    f=c['fabric']
    fields(f,'accessToSpines directZoneLinks routing failureStudy')
    if f['accessToSpines']!='wired-dual-homed' or f['routing'] not in ('shortest-path','ecmp') or f['failureStudy'] not in ('none','single-spine-outage'):
        raise ValueError('Unsupported fabric design')
    if 'outage' in c:
        o=c['outage'];fields(o,'spine downMs upMs')
        if f['failureStudy']!='single-spine-outage' or o['spine'] not in {f'spine-{i}' for i in range(c['inventory']['sharedSpines'])}:
            raise ValueError('Outage requires an existing spine and single-spine-outage mode')
        positive(o['downMs']);positive(o['upMs'])
        if not 1000<=o['downMs']<o['upMs']<=60000:raise ValueError('Outage must satisfy 1000 <= downMs < upMs <= 60000')
    if not isinstance(f['directZoneLinks'],list): raise ValueError('Expected zone link list')
    seen=set()
    for link in f['directZoneLinks']:
        fields(link,'from to')
        a,b=link['from'],link['to']
        if a not in zones or b not in zones or a==b or frozenset((a,b)) in seen:
            raise ValueError('Unknown, duplicate or self-linked zone')
        seen.add(frozenset((a,b)))
    if c['cellularSupport']!={'model':'5g-lena-epc','placementStatus':'shared-local-core-via-fabric','bypassSpinesForAuthorization':False}:
        raise ValueError('Expected shared local EPC with explicit fabric backhaul')
    a=c['authorization']
    fields(a,'authority requiredTaskTypes afterApproval onDeny onTimeout timeoutMs cacheApprovals')
    if (a['authority']!='main-server' or a['afterApproval']!='local-routing' or
        a['onDeny']!='block' or a['onTimeout']!='block' or a['cacheApprovals'] is not False):
        raise ValueError('Main-server approval per protected task, fail-closed, then local data required')
    types=a['requiredTaskTypes']
    if not isinstance(types,list) or any(not isinstance(t,str) or not t.strip() for t in types) or len(set(types))!=len(types):
        raise ValueError('Expected unique nonempty task type names')
    positive(a['timeoutMs'])
    service=c['externalService']
    fields(service,'scope oneWayDelayMs processingDelayMs response')
    if service['scope']!='abstract-upper-network-and-main-server' or service['response'] not in ('allow','deny','timeout'):
        raise ValueError('Unsupported mock service')
    positive(service['oneWayDelayMs']); positive(service['processingDelayMs'])
    robots={d['id'] for d in inventory['devices'] if d['role']=='robot'}
    if not isinstance(c['tasks'],list) or not c['tasks']: raise ValueError('Tasks required')
    task_ids=set(); plan=[]
    for t in c['tasks']:
        fields(t,'id type source destination deadlineMs'+(' startMs' if 'startMs' in t else ''))
        if 'startMs' in t:
            positive(t['startMs'])
            if not 1000<=t['startMs']<=60000:raise ValueError('Task startMs must be between 1000 and 60000')
        if not isinstance(t['id'],str) or not t['id'] or t['id'] in task_ids:
            raise ValueError('Task IDs must be unique')
        task_ids.add(t['id'])
        if not isinstance(t['type'],str) or not t['type'].strip(): raise ValueError('Task type required')
        if t['source'] not in robots or t['destination'] not in robots or t['source']==t['destination']:
            raise ValueError('Task must reference two distinct robots')
        positive(t['deadlineMs'])
        plan.append({'id':t['id'],'requiresMainServerAuthorization':t['type'] in types})
    return {'status':'VALID DESIGN ONLY; simulator not connected','totalDevices':inventory['totalDevices'],
            'robotCount':inventory['counts'].get('robot',0),'tasks':plan}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config',type=Path)
    args=parser.parse_args()
    try:
        print(json.dumps(validate(json.loads(args.config.read_text(encoding='utf-8'))),indent=2))
    except (ValueError, OSError) as error:
        parser.exit(2,str(error)+'\n')
