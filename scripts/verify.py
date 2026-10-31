#!/usr/bin/env python3
"""Configure, build and test in a clean host-specific directory. No downloads."""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def environment():
    env = os.environ.copy()
    portable = [ROOT / '.tools/mingw64/bin', ROOT / '.tools/python/cmake/data/bin', ROOT / '.tools/python/bin'] if os.name=='nt' else []
    env['PATH'] = os.pathsep.join(str(p) for p in portable if p.is_dir()) + os.pathsep + env.get('PATH', '')
    return env

def execute(command, env, log):
    command = list(map(str, command))
    command[0] = shutil.which(command[0], path=env['PATH']) or command[0]
    print('+ ' + ' '.join(map(str, command)), flush=True)
    with log.open('a', encoding='utf-8') as output:
        output.write('+ ' + ' '.join(command) + '\n');output.flush()
        # Keep build progress and diagnostics visible, and retain partial logs if interrupted.
        with subprocess.Popen(command,cwd=ROOT,env=env,stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace') as process:
            for line in process.stdout:
                print(line,end='',flush=True);output.write(line);output.flush()
            code=process.wait()
    if code:raise SystemExit(code)

def verify(preset='bootstrap', build_dir=None, ns3_prefix=None, require_ns3=False):
    require_ns3 = require_ns3 or ns3_prefix is not None
    if require_ns3 and preset!='default':
        raise SystemExit('ns-3 verification requires --preset default (bootstrap explicitly disables ns-3)')
    if ns3_prefix is not None and not Path(ns3_prefix).is_dir():
        raise SystemExit('ns-3 prefix does not exist: '+str(ns3_prefix)+'. Run scripts/setup_ns3.py first.')
    env = environment()
    for tool in ('cmake', 'ninja', 'ctest'):
        if not shutil.which(tool, path=env['PATH']):
            raise SystemExit('Missing prerequisite: ' + tool + '. See docs/reproducibility.md')
    build = Path(build_dir) if build_dir else ROOT / 'build' / ('verified-ns3' if require_ns3 else 'verified-' + preset)
    build = build.resolve()
    build.mkdir(parents=True, exist_ok=True)
    log = build / 'verification.log'
    command = ['cmake', '--preset', preset, '-B', build, '-DPython3_EXECUTABLE=' + sys.executable,
               '-DEALR_REQUIRE_NS3='+('ON' if require_ns3 else 'OFF'),'-DEALR_NS3_PREFIX=']
    if preset=='default':
        prefix=Path(ns3_prefix).resolve() if ns3_prefix else ROOT/'.tools/ns3-wired-install'
        if not ns3_prefix and not prefix.is_dir():prefix=ROOT/'.tools/ns3-install'
        if prefix.is_dir():
            command.append('-DCMAKE_PREFIX_PATH='+str(prefix))
            command.append('-DEALR_NS3_PREFIX='+str(prefix))
            env['PATH']=str(prefix/'lib')+os.pathsep+str(prefix/'bin')+os.pathsep+env['PATH']
            for variable in ['LD_LIBRARY_PATH','DYLD_LIBRARY_PATH']:
                env[variable]=str(prefix/'lib')+os.pathsep+env.get(variable,'')
    boost = Path(os.environ['EALR_BOOST_SOURCE']).resolve() if os.environ.get('EALR_BOOST_SOURCE') else ROOT / '.tools/boost_1_85_0'
    if os.environ.get('EALR_BOOST_SOURCE') and not (boost/'boost/json/src.hpp').is_file():
        raise SystemExit('EALR_BOOST_SOURCE must contain boost/json/src.hpp: '+str(boost))
    if boost.is_dir():
        command.append('-DEALR_BOOST_SOURCE=' + str(boost))
    execute(command, env, log)
    execute(['cmake', '--build', build, '-j', '4'], env, log)
    execute(['ctest', '--test-dir', build, '--output-on-failure'], env, log)
    return build, env

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--preset', choices=['bootstrap', 'default'])
    parser.add_argument('--build-dir')
    parser.add_argument('--ns3-prefix',help='Require real ns-3 from this installed prefix; implies default preset')
    parser.add_argument('--require-ns3',action='store_true',help='Fail rather than silently verify only the core')
    args = parser.parse_args()
    verify(args.preset or ('default' if args.ns3_prefix or args.require_ns3 else 'bootstrap'), args.build_dir, args.ns3_prefix, args.require_ns3)
