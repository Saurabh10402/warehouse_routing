#!/usr/bin/env python3
"""Separate mean/error-bar plots per scenario; never combine unlike experimental inputs."""
import csv
import sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

root=Path(sys.argv[1]);output=root/'graphs';output.mkdir(exist_ok=False)
with (root/'aggregate.csv').open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
for case in dict.fromkeys(r['case'] for r in rows):
    for metric,label in [('average_end_to_end_delay_ms','Mean delivered latency (ms)'),('packet_delivery_ratio_percent','Delivery (%)'),('deadline_miss_percent','Deadline misses incl. loss (%)'),('throughput_mbps','Application throughput (Mbps)')]:
        values=[r for r in rows if r['case']==case and r['metric']==metric]
        fig,ax=plt.subplots(figsize=(6,4),layout='constrained')
        ax.bar([r['algorithm'] for r in values],[float(r['mean']) for r in values],
               yerr=[float(r['sample_stddev']) if r['sample_stddev'] else 0 for r in values],capsize=4,color=['#507ab0','#a7a7a7','#238879'])
        ax.set(ylabel=label,title=f'{case}\nCore link abstraction; n={values[0]["seeds"]} seed(s)')
        fig.savefig(output/f'{case}-{metric}.png',dpi=150);plt.close(fig)
