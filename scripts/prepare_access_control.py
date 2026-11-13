"""Prepare a construction/peer probe, NOT execution of configured protected tasks."""
import argparse
import json
from pathlib import Path
from access_paths import plan
from wireless_layout import expand

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args()
    config=json.loads(args.config.read_text(encoding='utf-8'))
    expected=plan(config)
    inventory=expand(config['inventory'])
    args.output.mkdir(exist_ok=False)
    (args.output/'input.json').write_text(json.dumps(config,indent=2),encoding='utf-8')
    (args.output/'expected-design.json').write_text(json.dumps(expected,indent=2),encoding='utf-8')
    with (args.output/'nodes.txt').open('w',encoding='utf-8') as out:
        for d in inventory['devices']:
            if d['role'] in ('pgw','sgw','mme'):continue
            out.write(' '.join([d['id'],d['role'],d.get('access','-'),d.get('attachment','-'),*map(str,d.get('positionM',[0,0,0]))])+'\n')
    with (args.output/'links.txt').open('w',encoding='utf-8') as out:
        for edge in config['fabric']['directZoneLinks']:out.write(edge['from']+' '+edge['to']+'\n')
    print('Prepared peer probe only; authorization tasks are NOT executed.')
