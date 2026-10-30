#!/usr/bin/env python3
"""Create a source/checkpoint/evidence handoff, excluding build dependencies and caches."""
import argparse
import hashlib
import json
import subprocess
import uuid
import zipfile
from datetime import datetime,timezone
from pathlib import Path
from checkpoint import ROOT,source_files

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--experiment',type=Path,required=True);args=parser.parse_args()
    experiment=args.experiment.resolve()
    if not experiment.is_relative_to((ROOT/'results').resolve()):raise SystemExit('Experiment must be inside this repository results/')
    if json.loads((experiment/'RUN_STATUS.json').read_text())['status']!='PASS':raise SystemExit('Only validated batches may be packaged')
    destination=ROOT/'recovery_packages';destination.mkdir(exist_ok=True)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8]
    bundle=destination/f'source-{stamp}.bundle'
    subprocess.run(['git','bundle','create',str(bundle),'--all'],cwd=ROOT,check=True)
    archive=destination/f'wipro-recovery-{stamp}.zip'
    files=source_files()+[p for p in (ROOT/'checkpoints').rglob('*') if p.is_file()]+[p for p in experiment.rglob('*') if p.is_file()]+[bundle]
    hashes={}
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED) as z:
        for path in files:
            name=path.relative_to(ROOT).as_posix();z.write(path,name);hashes[name]=hashlib.sha256(path.read_bytes()).hexdigest()
        z.writestr('PACKAGE_MANIFEST.json',json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(),files=hashes),indent=2))
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:raise SystemExit('Archive CRC verification failed')
        for name,digest in hashes.items():
            if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise SystemExit('Archive hash verification failed: '+name)
    print('Verified recovery package: '+str(archive))
    print('SHA256: '+hashlib.sha256(archive.read_bytes()).hexdigest())
