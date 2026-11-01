#!/usr/bin/env python3
"""Create/verify an immutable source manifest; deliberately excludes build/results."""
import argparse
import hashlib
import json
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIRECTORIES = ('src', 'include', 'cmake', 'configs', 'tests', 'scripts', 'docs')


def git_commit():
    """Extracted recovery sources can run without Git; source manifests remain authoritative."""
    git = shutil.which('git')
    if git:
        top = subprocess.run([git, 'rev-parse', '--show-toplevel'], cwd=ROOT, text=True, capture_output=True)
        if top.returncode != 0 or Path(top.stdout.strip()).resolve() != ROOT.resolve():
            return 'unavailable (source archive)'
        result = subprocess.run([git, 'rev-parse', 'HEAD'], cwd=ROOT, text=True, capture_output=True)
        if result.returncode == 0:
            return result.stdout.strip()
    return 'unavailable (source archive)'

def source_files():
    files = [ROOT / name for name in ('CMakeLists.txt', 'CMakePresets.json', 'README.md', '.gitignore', 'AGENTS.md', 'OPEN_WAREHOUSE.html') if (ROOT/name).is_file()]
    for directory in DIRECTORIES:
        files.extend(p for p in (ROOT / directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    return sorted(files)

def manifest():
    hashes = {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_files()}
    digest = hashlib.sha256(json.dumps(hashes, sort_keys=True).encode()).hexdigest()
    return {'source_sha256': digest, 'files': hashes}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('identifier')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    path = ROOT / 'checkpoints' / 'manifests' / (args.identifier + '.json')
    current = manifest()
    if args.verify:
        saved = json.loads(path.read_text())
        if current['files'] != saved['files']:
            changed = sorted(k for k in current['files'].keys() | saved['files'].keys()
                             if current['files'].get(k) != saved['files'].get(k))
            raise SystemExit('Source differs: ' + ', '.join(changed))
        print('PASS: source matches ' + args.identifier)
    else:
        current.update(identifier=args.identifier, utc=datetime.now(timezone.utc).isoformat())
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('x', encoding='utf-8') as output:
            json.dump(current, output, indent=2)
        print(current['source_sha256'])
