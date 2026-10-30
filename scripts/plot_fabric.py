#!/usr/bin/env python3
"""Separate mean/error-bar plots per scenario; never combine unlike experimental inputs."""
import csv
import statistics
import sys
from pathlib import Path
portable=Path(__file__).resolve().parents[1]/'.tools/plotting'
if portable.is_dir():sys.path.insert(0,str(portable))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

root=Path(sys.argv[1]);output=root/'graphs';output.mkdir(exist_ok=False)
with (root/'aggregate.csv').open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
for case in dict.fromkeys(r['case'] for r in rows):
    for metric,label in [('average_end_to_end_delay_ms','Mean delivered latency (ms)'),('packet_delivery_ratio_percent','Delivery (%)'),('deadline_miss_percent','Deadline misses incl. loss (%)'),('throughput_mbps','Application throughput (Mbps)')]:
        values=[r for r in rows if r['case']==case and r['metric']==metric]
        fig,ax=plt.subplots(figsize=(8,4),layout='constrained')
        ax.bar([r.get('policy',r['algorithm']) for r in values],[float(r['mean']) for r in values],
               yerr=[float(r['sample_stddev']) if r['sample_stddev'] else 0 for r in values],capsize=4,color=['#507ab0','#238879','#a7a7a7','#bc7d39'])
        ax.set(ylabel=label,title=f'{case}\nCore link abstraction; n={values[0]["seeds"]} seed(s)')
        fig.savefig(output/f'{case}-{metric}.png',dpi=150);plt.close(fig)

# Scope and priority stay visible: aggregate delivery can hide missed control deadlines.
scope_file=root/'scope_priority_comparison.csv'
if scope_file.exists():
    with scope_file.open(newline='',encoding='utf-8') as f:scope_rows=list(csv.DictReader(f))
    for case in dict.fromkeys(r['case'] for r in scope_rows):
        samples=[r for r in scope_rows if r['case']==case]
        scopes=list(dict.fromkeys(r['scope'] for r in samples))
        policies=list(dict.fromkeys(r['policy'] for r in samples))
        fig,axes=plt.subplots(len(scopes),1,figsize=(9,3*len(scopes)),squeeze=False,layout='constrained')
        width=0.8/len(policies)
        for ax,scope in zip(axes[:,0],scopes):
            for index,policy in enumerate(policies):
                means=[];errors=[]
                for priority in range(4):
                    values=[float(r['deadline_miss_percent']) for r in samples
                            if r['scope']==scope and r['policy']==policy and int(r['priority'])==priority]
                    means.append(statistics.mean(values) if values else float('nan'))
                    errors.append(statistics.stdev(values) if len(values)>1 else 0)
                ax.bar([p-0.4+width*(index+0.5) for p in range(4)],means,width,
                       yerr=errors,capsize=2,label=policy)
            ax.set(xticks=range(4),xticklabels=['P0','P1','P2','P3'],ylim=(0,105),
                   ylabel='Deadline misses incl. loss (%)',title=scope)
        axes[0,0].legend(fontsize=8,ncol=2)
        fig.suptitle(f'{case}: scope / priority; seed means and sample SD\nCore wired/link abstraction; absent classes have no bar')
        fig.savefig(output/f'{case}-scope-priority-deadlines.png',dpi=150);plt.close(fig)
