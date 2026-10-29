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
    portable = [ROOT / '.tools/mingw64/bin', ROOT / '.tools/python/cmake/data/bin', ROOT / '.tools/python/bin']
    env['PATH'] = os.pathsep.join(str(p) for p in portable if p.is_dir()) + os.pathsep + env.get('PATH', '')
    return env

def execute(command, env, log):
    command = list(map(str, command))
    command[0] = shutil.which(command[0], path=env['PATH']) or command[0]
    print('+ ' + ' '.join(map(str, command)), flush=True)
    result = subprocess.run(list(map(str, command)), cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(result.stdout, end='', flush=True)
    with log.open('a', encoding='utf-8') as output:
        output.write('+ ' + ' '.join(map(str, command)) + '\n' + result.stdout)
    if result.returncode:
        raise SystemExit(result.returncode)

def verify(preset='bootstrap', build_dir=None):
    env = environment()
    for tool in ('cmake', 'ninja', 'ctest'):
        if not shutil.which(tool, path=env['PATH']):
            raise SystemExit('Missing prerequisite: ' + tool + '. See docs/reproducibility.md')
    build = Path(build_dir) if build_dir else ROOT / 'build' / ('verified-' + preset)
    build = build.resolve()
    build.mkdir(parents=True, exist_ok=True)
    log = build / 'verification.log'
    command = ['cmake', '--preset', preset, '-B', build, '-DPython3_EXECUTABLE=' + sys.executable]
    boost = ROOT / '.tools/boost_1_85_0'
    if boost.is_dir():
        command.append('-DEALR_BOOST_SOURCE=' + str(boost))
    execute(command, env, log)
    execute(['cmake', '--build', build, '-j', '4'], env, log)
    execute(['ctest', '--test-dir', build, '--output-on-failure'], env, log)
    return build, env

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--preset', choices=['bootstrap', 'default'], default='bootstrap')
    parser.add_argument('--build-dir')
    args = parser.parse_args()
    verify(args.preset, args.build_dir)
