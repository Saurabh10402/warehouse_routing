"""Bundle source, both demonstrations and raw evidence; verify a fresh extraction."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid
import zipfile
from datetime import datetime,timezone
from checkpoint import ROOT,source_files,manifest
from verify_delivery import verify

BATCHES=['layout-20261101T203251Z-838ca5b1','layout-20261101T203641Z-425a1ee4',
         'layout-20261102T065135Z-da39e8c6','viewer-checkpoint015','delivery-viewers-018',
         'layout-20261102T121400Z-1b3f3906','layout-20261102T120343Z-c17f3ded',
         'ns3-warehouse-20261102T121634Z-4c776a9c','experiments-20261102T120006Z-a802bc3c',
         'route-entry-regression-20261102T120420Z-681299aa']


def main():
    for name in BATCHES:
        if json.loads((ROOT/'results'/name/'RUN_STATUS.json').read_text())['status']!='PASS':raise ValueError('Incomplete batch: '+name)
    state=json.loads((ROOT/'checkpoints/PROJECT_STATE.json').read_text())
    saved=json.loads((ROOT/'checkpoints'/state['source_state']).read_text())
    if saved['files']!=manifest()['files']:raise ValueError('Current checkpoint source mismatch')
    destination=ROOT/'recovery_packages';destination.mkdir(exist_ok=True)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8]
    bundle=destination/('delivery-source-'+stamp+'.bundle')
    subprocess.run(['git','bundle','create',str(bundle),'--all'],check=True,cwd=ROOT)
    files=source_files()+[p for p in (ROOT/'checkpoints').rglob('*') if p.is_file()]
    for name in BATCHES:files += [p for p in (ROOT/'results'/name).rglob('*') if p.is_file()]
    files.append(bundle)
    hashes={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    archive=destination/('warehouse-delivery-'+stamp+'.zip')
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED) as z:
        for p in files:z.write(p,p.relative_to(ROOT).as_posix())
        z.writestr('DELIVERY_MANIFEST.json',json.dumps(dict(checkpoint=state['checkpoint'],files=hashes),indent=2))
    extracted=ROOT/'.recovery'/('d '+stamp[-8:]);extracted.mkdir(parents=True)
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:raise ValueError('Archive CRC failure')
        for info in z.infolist():
            if not (extracted/info.filename).resolve().is_relative_to(extracted.resolve()):raise ValueError('Unsafe archive path')
        z.extractall(extracted)
    checks=verify(extracted)
    subprocess.run([sys.executable,str(extracted/'scripts/verify_delivery.py')],cwd=extracted,check=True)
    subprocess.run([sys.executable,str(extracted/'scripts/checkpoint.py'),state['checkpoint'],'--verify'],cwd=extracted,check=True)
    restored=extracted/bundle.relative_to(ROOT)
    subprocess.run(['git','bundle','verify',str(restored)],cwd=ROOT,check=True,capture_output=True)
    head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    if head+' HEAD' not in subprocess.check_output(['git','bundle','list-heads',str(restored)],text=True):raise ValueError('Bundle HEAD mismatch')
    receipt=dict(archive=str(archive),sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),bytes=archive.stat().st_size,
                 extraction=str(extracted),checks=checks,checkpoint=state['checkpoint'],head=head,git_bundle='PASS',
                 scope='Extraction, hashes and entry/viewer links verified; no new target-platform build claim')
    (destination/('receipt-'+state['checkpoint']+'-delivery.json')).write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
