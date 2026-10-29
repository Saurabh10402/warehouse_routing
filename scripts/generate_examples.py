#!/usr/bin/env python3
"""Deterministically expand example warehouse layouts into explicit schema-v2 graphs."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def base(name):
    return dict(schemaVersion=2, name=name,
                topology=dict(kind='graph', minimumSpinesPerLeaf=0, minimumEdges=0),
                simulation=dict(durationMs=1000, drainMs=200, seed=1, scheduler='strictPriority'),
                routing=dict(algorithm='dijkstra', alpha=1, beta=1, gamma=1, latencyScaleMs=1, maxStretch=1.5),
                deadlinesMs=[10,20,40,100],
                safety=dict(warningCelsius=70, stopCelsius=80, broadcastEmergencyStop=True,
                            processAtLocalEdge=True, requireManualResetAfterStop=True, processingMs=0.1),
                zones=[], nodes=[], links=[], traffic=[], events=[])

def node(c, identifier, role, x, y, zone='', access='wired', **extra):
    c['nodes'].append(dict(id=identifier,role=role,x=x,y=y,zone=zone,access=access,**extra))

def link(c, a, b, capacity=100, delay=0.1, queue=100):
    c['links'].append(dict(id=a+'--'+b, **{'from':a, 'to':b}, capacityMbps=capacity,delayMs=delay,queuePackets=queue))

def warehouse(counts, spines=2, edges=2):
    c=base('Reference configuration; physical layout not finalized')
    c['topology']=dict(kind='leafSpine',minimumSpinesPerLeaf=spines,minimumEdges=edges)
    for i in range(spines): node(c,f'spine{i}','spine',i*20,200)
    for i in range(edges):
        node(c,f'edge{i}','edge',i*20,300)
        for s in range(spines):link(c,f'spine{s}',f'edge{i}',25000,0.05)
    for z,count in enumerate(counts):
        zone=f'zone{z}';c['zones'].append(zone)
        leaf=f'leaf{z}';node(c,leaf,'leaf',z*20,100,zone)
        for s in range(spines):link(c,leaf,f'spine{s}',10000,0.05)
        for technology,capacity in [('wifi',200),('private5g',1000)]:
            access=f'{zone}-{technology}';node(c,access,'access',z*20,50,zone,technology)
            link(c,access,leaf,capacity,0.2)
        for e in range(count):
            endpoint=f'{zone}-ep{e}';technology=['wired','private5g','wifi','wifi'][e%4]
            node(c,endpoint,'endpoint',z*20+(e%10),e//10,zone,technology)
            link(c,endpoint,leaf if technology=='wired' else f'{zone}-{technology}',100,
                 {'wired':0.1,'wifi':1.0,'private5g':0.5}[technology])
            c['traffic'].append(dict(id=f'flow-{endpoint}',source=endpoint,destinations=[f'edge{i}' for i in range(edges)],
                                     priority=e%4,bytes=512,startMs=0,stopMs=1000,intervalMs=200,phaseJitterMs=100))
    return c

def small():
    c=base('Small deterministic diamond')
    c['zones']=['A'];c['simulation']['durationMs']=100;c['simulation']['drainMs']=100
    for args in [('source','endpoint',0,0),('fast','relay',1,0),('alternate','relay',1,1),('edge0','edge',2,0),('edge1','edge',2,1)]:
        node(c,*args,zone='A')
    for a,b in [('source','fast'),('fast','edge0'),('source','alternate'),('alternate','edge0'),('alternate','edge1')]:link(c,a,b,10,0.2)
    for p in range(4):c['traffic'].append(dict(id=f'p{p}',source='source',destinations=['edge0','edge1'],priority=p,bytes=500,startMs=p,stopMs=90,intervalMs=10))
    return c

if __name__=='__main__':
    for name,c in [('warehouse_reference',warehouse([50]*10)),('warehouse_uneven',warehouse([10,35,75,20])),('warehouse_small',small())]:
        (ROOT/'configs'/f'{name}.json').write_text(json.dumps(c,indent=2)+'\n',encoding='utf-8')
