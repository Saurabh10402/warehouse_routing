"""Bounded matched radio-seed comparison; every case retains native replay/audits."""
import argparse,copy,csv,json,statistics,subprocess
from datetime import datetime,timezone
from pathlib import Path
import uuid
from run_access_authorization import run,capabilities,ROOT
from process_control import write_json

PAIRS=((1,1),(2,1),(3,1),(1,2))


def make_plan(base,scale=False):
    cases=[]
    for seed,number in PAIRS:
        for policy in ('shortest-path','ecmp'):
            c=copy.deepcopy(base)
            c['radioRandomness']={'seed':seed,'run':number};c['fabric']['routing']=policy
            capabilities(c,scale=scale)
            cases.append((f'seed-{seed}-run-{number}-{policy}',c))
    return cases


def summarize(audit):
    tasks=audit['tasks'];times=[t['fullTaskMs'] for t in tasks if t['payloadReceived']]
    met=sum(t['deadlineMet'] for t in tasks)
    return {'offered':len(tasks),'delivered':sum(t['payloadReceived'] for t in tasks),
            'blocked':sum(t['state'] in ('denied','timeout') for t in tasks),
            'deadlineMet':met,'deadlineMissed':len(tasks)-met,
            'medianTaskMs':statistics.median(times) if times else None,'maxTaskMs':max(times) if times else None}


def comparison(config,ns3,build,results,scale=False):
    base=json.loads(config.read_text(encoding='utf-8'));plan=make_plan(base,scale)
    batch=results.resolve()/('access-seeds-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(parents=True,exist_ok=False);(batch/'inputs').mkdir()
    for label,c in plan:write_json(batch/'inputs'/(label+'.json'),c)
    write_json(batch/'plan.json',{'schema':'access-seed-comparison-v1','radioPairs':[list(p) for p in PAIRS],
        'policies':['shortest-path','ecmp'],'cases':[label for label,_ in plan],
        'scope':'three seeds at run1 plus seed1/run2 substream check; fixed ECMP hash; small stationary control'})
    status={'status':'RUNNING','planned':len(plan),'completed':0,'failed':0};rows=[]
    write_json(batch/'status.json',status);print('Comparison: '+str(batch),flush=True)
    try:
        for label,c in plan:
            row={'case':label,'seed':c['radioRandomness']['seed'],'run':c['radioRandomness']['run'],
                 'policy':c['fabric']['routing'],'offered':len(c['tasks'])}
            try:
                result=run(batch/'inputs'/(label+'.json'),ns3,build,batch/'cases'/label,scale=scale)
                audit=json.loads((result/'audit.json').read_text())
                row.update(status='PASS',result=str(result.relative_to(batch)),**summarize(audit))
            except (ValueError,RuntimeError,OSError,subprocess.SubprocessError) as error:
                row.update(status='FAILED',reason=str(error));status['failed']+=1
            rows.append(row);status['completed']+=1
            write_json(batch/'summary.json',rows);write_json(batch/'status.json',status)
        status['status']='PASS' if status['failed']==0 else 'FAILED'
        fields=['case','seed','run','policy','status','offered','delivered','blocked','deadlineMet','deadlineMissed','medianTaskMs','maxTaskMs','result','reason']
        with (batch/'summary.csv').open('x',newline='',encoding='utf-8') as out:
            writer=csv.DictWriter(out,fieldnames=fields);writer.writeheader();writer.writerows(rows)
    except BaseException as error:
        status.update(status='INTERRUPTED' if isinstance(error,KeyboardInterrupt) else 'FAILED',reason=str(error));raise
    finally:write_json(batch/'status.json',status)
    print(json.dumps(status,indent=2),flush=True)
    if status['status']!='PASS':raise RuntimeError('One or more cases failed; inspect '+str(batch))
    return batch


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('config',type=Path);p.add_argument('--ns3-source',type=Path,required=True)
    p.add_argument('--build',type=Path,required=True);p.add_argument('--results',type=Path,default=ROOT/'results')
    p.add_argument('--scale-control',action='store_true',help='Use explicit bounded intermediate-scale controls')
    args=p.parse_args()
    try:comparison(args.config,args.ns3_source,args.build,args.results,args.scale_control)
    except (ValueError,RuntimeError,OSError,subprocess.SubprocessError) as error:p.exit(1,str(error)+'\n')
