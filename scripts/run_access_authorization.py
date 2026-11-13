"""Build/run/replay/audit the small authorization control on Linux/WSL."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import signal
import subprocess
import sys
import time
from datetime import datetime, timezone
import uuid
from access_study import validate
from access_paths import plan
from audit_access_auth import check
from process_control import write_json

ROOT=Path(__file__).resolve().parents[1]
NS3='ea50b72ab79b1ec5912c66d5b384effa829f18bc'
NR='f29ebd33450c49af934ea5dde8606f855c22c2a6'


def capabilities(config, boundary=False, scale=False):
    summary=validate(config);plan(config)
    if boundary and config['fabric']['routing']=='ecmp':
        raise ValueError('Boundary-loss audit is validated only for shortest-path; use the phased ECMP control')
    if config['fabric']['failureStudy']=='single-spine-outage' and 'outage' not in config:
        raise ValueError('Live outage requires an explicit outage schedule')
    limit=300 if scale else 64
    if summary['totalDevices']>limit:
        raise ValueError(f'This control is limited to {limit} local devices; use the explicit bounded scale control only for intermediate validation')
    if not any(z['robots']['nr'] for z in config['inventory']['zones']):
        raise ValueError('This native control requires NR UEs')
    if len(config['tasks'])>1000 or config['authorization']['timeoutMs']>60000:
        raise ValueError('At most1000 tasks and60000ms timeout supported')
    if max(config['externalService'][k] for k in ('oneWayDelayMs','processingDelayMs'))>60000:
        raise ValueError('External delays above60000ms outside bounded control scope')
    return summary


def native_run_limit(local_devices, scale=False):
    """Bound wall time separately from simulated task deadlines."""
    return (300 if local_devices<=128 else (600 if local_devices<=200 else 1200)) if scale else 180


def check_radio(config, trace):
    expected=config.get('radioRandomness',{'seed':1,'run':1})
    records=[line.split()[1:] for line in trace.splitlines() if line.startswith('RADIO_RNG ')]
    if records!=[[str(expected['seed']),str(expected['run'])]]:
        raise ValueError('Native radio seed/run differs from configured values')
    return expected


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
    return h.hexdigest()


def command(args, log, cwd, seconds, allowed=(0,)):
    """Preserve raw deterministic output; terminate the Linux process group on timeout."""
    with log.open('x') as out:
        process=subprocess.Popen(list(map(str,args)),cwd=cwd,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            code=process.wait(timeout=seconds)
        except BaseException:
            os.killpg(process.pid,signal.SIGKILL);process.wait(timeout=10)
            raise
    if code not in allowed:raise RuntimeError(f'Command exited {code}; see {log}')
    return code


def timed_command(args, log, cwd, seconds, timings, label, timing_path, allowed=(0,)):
    """Retain elapsed time even when native execution fails or times out."""
    started=time.monotonic()
    try:
        return command(args,log,cwd,seconds,allowed=allowed)
    finally:
        timings[label]=time.monotonic()-started
        write_json(timing_path,{'nativeSeconds':timings,'perRunLimitSeconds':seconds})


def run(config_path, ns3, build, results, boundary=False, scale=False, measure=False):
    if boundary and measure:raise ValueError("Choose boundary control or outcome measurement, not both")
    config=json.loads(config_path.read_text(encoding='utf-8'));summary=capabilities(config,boundary,scale)
    radio=config.get('radioRandomness',{'seed':1,'run':1})
    run_limit=native_run_limit(summary['totalDevices'],scale)
    if boundary and ('outage' not in config or config['externalService']['response']!='allow'):
        raise ValueError('Boundary audit requires scheduled outage and allow response')
    if platform.system()!='Linux':raise ValueError('Run this command in Linux/WSL, not native Windows')
    ns3=ns3.resolve();build=build.resolve()
    for path,revision in ((ns3,NS3),(ns3/'contrib/nr',NR)):
        actual=subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()
        if actual!=revision:raise ValueError('Unexpected dependency revision: '+str(path))
    if not (build/'CMakeCache.txt').is_file():raise ValueError('Existing configured radio build required')
    batch=results.resolve()/('access-auth-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8])
    batch.mkdir(parents=True,exist_ok=False)
    status={'status':'RUNNING','scope':'authorization control; immediate centralized spine recovery when scheduled; no scale claim','scaleControl':scale,'radioRandomness':radio,'localDevices':summary['totalDevices'],'auditMode':'outcome-measurement' if measure else ('boundary-loss' if boundary else 'delivery-control')}
    write_json(batch/'status.json',status)
    print('Results: '+str(batch),flush=True)
    try:
        # Reject unexpected dependency edits; verify known test-only patch.
        command([sys.executable,ROOT/'scripts/apply_radio_test_patch.py',ns3],batch/'dependency-check.log',ROOT,60)
        if subprocess.check_output(['git','-C',str(ns3/'contrib/nr'),'diff','HEAD','--name-only'],text=True).strip():
            raise ValueError('Unexpected tracked NR dependency changes')
        sources={'access-task-control.cc':'warehouse-access-tasks.cc','access-task-fabric.h':'access-task-fabric.h','access-authorization.h':'access-authorization.h','access-ecmp.h':'access-ecmp.h'}
        (batch/'source').mkdir()
        for name,target in sources.items():
            original=ROOT/'src'/name;destination=ns3/'scratch'/target
            shutil.copyfile(original,batch/'source'/name)
            if not destination.exists() or destination.read_bytes()!=original.read_bytes():
                if destination.exists():
                    (batch/'previous-scratch').mkdir(exist_ok=True)
                    shutil.copyfile(destination,batch/'previous-scratch'/target)
                shutil.copyfile(original,destination)
        write_json(batch/'source-hashes.json',{name:sha(ROOT/'src'/name) for name in sources})
        write_json(batch/'input.json',config)
        command(['cmake','--build',build,'--target','scratch_warehouse-access-tasks','-j','2'],batch/'build.log',ROOT,900)
        exe=ns3/'build/scratch/ns3.46-warehouse-access-tasks-relwithdebinfo'
        if not exe.is_file():raise ValueError('Expected RelWithDebInfo executable missing')
        # Hash the actual executable and transitive ns-3 libraries before execution.
        linked=subprocess.check_output(['ldd',str(exe)],text=True)
        libs=[Path(line.split('=>',1)[1].strip().split()[0]) for line in linked.splitlines() if 'libns3' in line and '=>' in line]
        write_json(batch/'provenance.json',{'ns3':NS3,'nr':NR,'platform':platform.platform(),'radioRandomness':radio,
            'executable':str(exe),'executableSha256':sha(exe),'libraries':{str(p):sha(p) for p in libs},
            'cmakeCacheSha256':sha(build/'CMakeCache.txt')})
        command([sys.executable,ROOT/'scripts/prepare_access_tasks.py',batch/'input.json',batch/'case'],batch/'prepare.log',ROOT,60)
        case=batch/'case'
        timings={}
        for label in ('original','replay'):
            code=timed_command([exe,'--inventory='+str(case/'nodes.txt'),'--links='+str(case/'links.txt'),
                     '--tasks='+str(case/'tasks.txt'),'--radioSeed='+str(radio['seed']),'--radioRun='+str(radio['run']),'--output='+str(case/(label+'.csv'))],case/(label+'.log'),case,run_limit,timings,label,batch/'execution-timing.json',allowed=(0,2) if boundary or measure else (0,))
            (case/(label+'.exit')).write_text(str(code)+'\n')
        if measure:
            from audit_access_outcomes import check_outcomes
            audit=check_outcomes(case)
        elif boundary:
            from audit_access_boundary import check_boundary
            audit=check_boundary(case)
        else:audit=check(case)
        if config['fabric']['routing']=='ecmp':
            from audit_access_ecmp import check_ecmp
            audit['ecmp']=check_ecmp(case)
        from audit_access_coverage import check_coverage
        audit['coverage']=check_coverage(case,audit)
        audit['radioRandomness']=check_radio(config,(case/'original.log').read_text())
        write_json(batch/'audit.json',audit)
        status.update(status='PASS',offeredTasks=audit['offeredTasks'],replay=audit['replay'])
        if measure:status.update(status='MEASURED',validation='PASS',outcomes=audit['outcomes'],nativeExit=audit['nativeExit'],deliveryControlPassed=audit['deliveryControlPassed'],allTasksDelivered=audit['allTasksDelivered'],allDeadlinesMet=audit['allDeadlinesMet'],meaning='Accounting/replay validated; inspect outcomes separately from delivery and deadline success')
        if boundary:status.update(outcomes=audit['outcomes'],meaning='Loss/accounting control passed; not all tasks delivered')
    except BaseException as error:
        status.update(status='INTERRUPTED' if isinstance(error,KeyboardInterrupt) else 'FAILED',reason=str(error))
        raise
    finally:
        write_json(batch/'status.json',status)
    print(json.dumps(status,indent=2));return batch


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('config',type=Path);p.add_argument('--ns3-source',type=Path,required=True)
    p.add_argument('--build',type=Path,required=True);p.add_argument('--results',type=Path,default=ROOT/'results')
    modes=p.add_mutually_exclusive_group()
    modes.add_argument('--outcome-measurement',action='store_true',help='Audit outcomes and replay even on native delivery exit2; reports MEASURED, not delivery PASS')
    modes.add_argument('--boundary-control',action='store_true',help='Audit observed transition loss/timeouts; does not require all tasks to deliver')
    p.add_argument('--scale-control',action='store_true',help='Explicit intermediate control: at most300 local devices;300s through128,600s through200,1200s above; not500-device validation')
    args=p.parse_args()
    try:run(args.config,args.ns3_source,args.build,args.results,args.boundary_control,args.scale_control,args.outcome_measurement)
    except (ValueError,RuntimeError,OSError,subprocess.SubprocessError) as error:p.exit(1,str(error)+'\n')
