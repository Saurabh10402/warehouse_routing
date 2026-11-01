"""Export an independently audited native result as a standalone offline HTML viewer."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
from ns3_warehouse import validate_output


def export(batch, output):
    batch,output=Path(batch),Path(output)
    if json.loads((batch/'RUN_STATUS.json').read_text())['status']!='PASS':
        raise ValueError('Only completed PASS batches may be visualized')
    config=json.loads((batch/'config.json').read_text())
    report=validate_output(batch/'run',config)
    def rows(name):
        with (batch/'run'/name).open(newline='',encoding='utf-8') as f:return list(csv.DictReader(f))
    packets=rows('packets.csv')
    if not packets:raise ValueError('No recorded packets to display')
    data=dict(config=config,packets=packets,hops=[h for h in rows('hops.csv') if h['event']=='rx'],report=report,batch=batch.name,
              hashes={name:hashlib.sha256((batch/name).read_bytes()).hexdigest() for name in ['config.json','run/packets.csv','run/hops.csv']})
    template=Path(__file__).with_name('warehouse_viewer.html').read_text(encoding='utf-8')
    payload=json.dumps(data,separators=(',',':')).replace('<','\\u003c').replace('>','\\u003e').replace('&','\\u0026')
    with output.open('x',encoding='utf-8') as f:f.write(template.replace('/*WAREHOUSE_DATA*/',payload))
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--batch',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    try: print(json.dumps(export(a.batch,a.output),indent=2));print('Viewer: '+str(a.output.resolve()))
    except (ValueError,OSError,KeyError,AssertionError) as e:p.exit(2,str(e)+'\n')
