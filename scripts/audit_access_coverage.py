"""Check native device inventory and report per-phase robot traffic coverage."""
import csv,json,re
from pathlib import Path
from wireless_layout import expand
from access_study import time_ns


def check_coverage(folder,audit):
    folder=Path(folder);config=json.loads((folder/'input.json').read_text())
    inventory=expand(config['inventory']);devices={d['id']:d for d in inventory['devices']}
    expected={name:d for name,d in devices.items() if d['role'] not in ('pgw','sgw','mme')}
    with (folder/'original.csv').open(newline='') as stream:rows=list(csv.DictReader(stream))
    actual={r['id']:r for r in rows}
    if len(actual)!=len(rows) or set(actual)!=set(expected):raise ValueError('Native device inventory mismatch')
    if len({r['nodeId'] for r in rows})!=len(rows):raise ValueError('Duplicate native node ID')
    for name,device in expected.items():
        row=actual[name]
        if row['role']!=device['role']:raise ValueError('Native role mismatch')
        radio={'ap':'ns3::WifiNetDevice','gnb':'ns3::NrGnbNetDevice'}.get(device['role'],'none')
        if device['role']=='robot':
            radio='ns3::NrUeNetDevice' if device['access']=='nr' else 'ns3::WifiNetDevice'
            if row['attachment']!=device['attachment']:raise ValueError('Native radio attachment mismatch')
        if row['radioType']!=radio:raise ValueError('Robot or access node is not the configured radio type')
    counts=re.findall(r'inventory=(\d+) actualNodes=(\d+); tasks=(\d+)',(folder/'original.log').read_text())
    if counts!=[(str(inventory['totalDevices']),str(inventory['totalDevices']+1),str(len(config['tasks'])))]:raise ValueError('Native total node/task count mismatch')
    robots={name:d for name,d in devices.items() if d['role']=='robot'}
    outage=config.get('outage');phases={name:{'offered':set(),'sent':set(),'received':set(),'tasks':0} for name in (('before','during','after') if outage else ('static',))}
    if len(audit['tasks'])!=len(config['tasks']):raise ValueError('Missing task coverage result')
    for i,(task,result) in enumerate(zip(config['tasks'],audit['tasks'])):
        if task['id']!=result['id']:raise ValueError('Task coverage identity mismatch')
        start=time_ns(task.get('startMs',1200+10*i));phase='static'
        if outage:phase='before' if start<time_ns(outage['downMs']) else 'during' if start<time_ns(outage['upMs']) else 'after'
        record=phases[phase];record['tasks']+=1;record['offered'].add(task['source'])
        if result['payloadSent']:record['sent'].add(task['source'])
        if result['payloadReceived']:record['received'].add(task['source'])
    output={}
    for phase,record in phases.items():
        zones={}
        for zone in config['inventory']['zones']:
            z=zone['id'];zones[z]={access:sum(robots[r]['zone']==z and robots[r]['access']==access for r in record['offered']) for access in ('nr','wifi')}
        output[phase]={'tasks':record['tasks'],'offeredSources':len(record['offered']),'payloadSources':len(record['sent']),
                       'deliveredSources':len(record['received']),'missingOfferedSources':sorted(set(robots)-record['offered']),'zoneAccessOfferedSources':zones}
    return {'localDevices':inventory['totalDevices'],'actualNodesIncludingExternalServer':inventory['totalDevices']+1,
            'robots':len(robots),'zones':len(config['inventory']['zones']),'phases':output,
            'allRobotsOfferedInEveryPhase':all(not p['missingOfferedSources'] for p in output.values()),
            'scope':'Per-phase source coverage; blocked tasks remain offered, not counted as payload sources'}
