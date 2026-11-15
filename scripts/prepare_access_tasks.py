"""Prepare native authorization controls from the validated access-study JSON."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from access_study import validate

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('config',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--probe-500',action='store_true')
    args=p.parse_args();c=json.loads(args.config.read_text(encoding='utf-8'));validate(c)
    limit=2000 if args.probe_500 else 1000
    if c['authorization']['timeoutMs']>60000 or len(c['tasks'])>limit:
        raise SystemExit(f'Native control limit: timeout <=60000 ms and <={limit} tasks')
    subprocess.run([sys.executable,str(Path(__file__).with_name('prepare_access_control.py')),str(args.config),str(args.output)],check=True)
    service=c['externalService'];required=set(c['authorization']['requiredTaskTypes'])
    outage=c.get('outage',{'spine':'none','downMs':0,'upMs':0})
    with (args.output/'tasks.txt').open('w',encoding='utf-8') as out:
        out.write(f"{service['oneWayDelayMs']} {service['processingDelayMs']} {c['authorization']['timeoutMs']} {service['response']} {outage['spine']} {outage['downMs']} {outage['upMs']} {c['fabric']['routing']}\n")
        for i,t in enumerate(c['tasks']):
            out.write(f"{t['source']} {t['destination']} {int(t['type'] in required)} {t['deadlineMs']} {t.get('startMs',1200+i*10)}\n")
    print('Task table added; task indices map to input.json order. No simulation executed.')
