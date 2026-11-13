"""Audit the fixed small control's cross-zone Wi-Fi fabric paths and replay."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
from audit_access_backhaul import audit


def check(folder, mode):
    original=(folder/'original.log').read_bytes()
    if original!=(folder/'replay.log').read_bytes():raise ValueError('Trace replay mismatch')
    if (folder/'original.csv').read_bytes()!=(folder/'replay.csv').read_bytes():raise ValueError('Device replay mismatch')
    if any((folder/(run+'.exit')).read_text().strip()!='0' for run in ('original','replay')):
        raise ValueError('Native delivery/attachment control failed; preserve logs')
    text=original.decode(); result=audit(text)
    paths=defaultdict(list)
    for line in text.splitlines():
        p=line.split()
        if p and p[0]=='WIRE' and p[5]=='9008':
            paths[tuple(p[3:7])].append((int(p[7]),p[1],p[2]))
    expected=['zone-A-ap-0','zone-A-switch-0']
    if mode=='spine':expected.append('spine-0')
    expected+=['zone-B-switch-0','zone-B-ap-0']
    if len(paths)!=10:raise ValueError('Expected ten cross-zone datagrams')
    for hops in paths.values():
        hops.sort()
        if [(a,b) for _,a,b in hops]!=list(zip(expected,expected[1:])):
            raise ValueError('Unexpected measured cross-zone fabric path')
    if result['flows']!=9 or result['received']!=90:raise ValueError('Delivery mismatch')
    result.update(crossZoneFabricPath=expected,crossZoneDatagrams=len(paths),
                  replay='byte-identical traces and devices',sha256=hashlib.sha256(original).hexdigest(),
                  limitation='Static topology comparison, not live failure recovery or radio/end-to-end timing audit')
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('batch',type=Path)
    args=p.parse_args()
    print(json.dumps({mode:check(args.batch/mode,mode) for mode in ('direct','spine')},indent=2))
