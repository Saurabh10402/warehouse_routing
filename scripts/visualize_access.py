"""Export independently audited wireless outcomes to a standalone offline HTML file."""
import argparse,csv,hashlib,json
from pathlib import Path
from audit_access_outcomes import check_outcomes
from audit_access_ecmp import check_ecmp
from audit_access_coverage import check_coverage
from run_access_authorization import check_radio

def load_run(result):
    result=Path(result)
    config=json.loads((result/'input.json').read_text(encoding='utf-8'));case=result/'case'
    audit=check_outcomes(case)
    if config['fabric']['routing']=='ecmp':audit['ecmp']=check_ecmp(case)
    audit['coverage']=check_coverage(case,audit);audit['radioRandomness']=check_radio(config,(case/'original.log').read_text())
    if audit!=json.loads((result/'audit.json').read_text()):raise ValueError('Saved audit differs from independent checks')
    events={};links=[]
    for line in (case/'original.log').read_text().splitlines():
        p=line.split()
        if p and p[0]=='TASK_EVENT':events.setdefault(int(p[1]),[]).append({'event':p[2],'timeMs':int(p[3])/1e6})
        if p and p[0]=='LINK':links.append(p[1:3])
    tasks=[]
    for i,(task,measured) in enumerate(zip(config['tasks'],audit['tasks'])):
        if task['id']!=measured['id']:raise ValueError('Task order mismatch')
        tasks.append({**task,**measured,'events':events.get(i,[])})
    data={'result':result.name,'config':config,'audit':{k:v for k,v in audit.items() if k!='tasks'},'tasks':tasks,'devices':list(csv.DictReader((case/'original.csv').open())),'links':links,'traceSha256':hashlib.sha256((case/'original.log').read_bytes()).hexdigest()}
    return data

def export(result,output,compare=()):
    output=Path(output)
    if output.exists():raise ValueError('Refusing to overwrite viewer')
    data=load_run(result);runs=[data]+[load_run(p) for p in compare]
    normalized=lambda c:{k:v for k,v in c.items() if k!='radioRandomness'}
    if any(normalized(d['config'])!=normalized(data['config']) for d in runs):raise ValueError('Comparison inputs must match except radio seed/run')
    identities=[(d['audit']['radioRandomness']['seed'],d['audit']['radioRandomness']['run']) for d in runs]
    if len(set(identities))!=len(identities):raise ValueError('Duplicate seed/run in comparison')
    data['comparison']=[{'result':d['result'],'seed':d['audit']['radioRandomness']['seed'],'run':d['audit']['radioRandomness']['run'],'offered':d['audit']['offeredTasks'],**d['audit']['outcomes'],'traceSha256':d['traceSha256']} for d in runs]
    template=Path(__file__).with_name('access_viewer.html').read_text(encoding='utf-8')
    encoded=json.dumps(data,ensure_ascii=True).replace('<','\\u003c').replace('>','\\u003e').replace('&','\\u0026')
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(template.replace('__ACCESS_DATA__',encoded),encoding='utf-8')
    return {'output':str(output),'tasks':len(data['tasks']),'robots':data['audit']['coverage']['robots'],'totalDevices':data['audit']['coverage']['actualNodesIncludingExternalServer'],'comparisonRuns':len(runs),'independentAudit':'PASS'}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('result',type=Path);p.add_argument('output',type=Path);p.add_argument('--compare',type=Path,action='append',default=[]);a=p.parse_args();print(json.dumps(export(a.result,a.output,a.compare),indent=2))
