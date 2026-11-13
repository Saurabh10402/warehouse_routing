"""Validate and resolve a stationary wireless inventory; does not run ns-3."""
import argparse
import json
import math
import re
from pathlib import Path


def expand(spec):
    def fields(obj, required):
        if not isinstance(obj, dict) or set(obj) != set(required.split()):
            raise ValueError('Expected exactly these fields: '+required)
    def count(value, name, minimum=0):
        if type(value) is not int or not minimum <= value <= 4096:
            raise ValueError(name+' must be an integer in '+str(minimum)+'..4096')
        return value
    def position(value):
        if (not isinstance(value, list) or len(value) != 3 or
            any(type(v) not in (int, float) or not math.isfinite(v) for v in value)
            or value[2] < 0):
            raise ValueError('Positions require three finite metre coordinates, z >= 0')
    fields(spec, 'schema kind mobility sharedSpines core zones')
    if spec['schema'] != 'warehouse-wireless-layout-v1' or spec['kind'] != 'inventory' or spec['mobility'] != 'stationary':
        raise ValueError('Expected stationary warehouse-wireless-layout-v1 inventory')
    count(spec['sharedSpines'], 'sharedSpines', 1)
    if spec['core'] != {'model': '5g-lena-epc', 'pgw': 1, 'sgw': 1, 'mme': 1}:
        raise ValueError('This contract requires one PGW, SGW and MME; not a standalone 5G core')
    for role in ('pgw', 'sgw', 'mme'):
        count(spec['core'][role], role, 1)
    if not isinstance(spec['zones'], list) or not spec['zones']:
        raise ValueError('zones must be a nonempty list')
    seen = set()
    total = spec['sharedSpines'] + 3
    for zone in spec['zones']:
        fields(zone, 'id switches edgeServers wifiAps nrGnbs robots originM grid')
        zid = zone['id']
        if not isinstance(zid, str) or not re.fullmatch('[A-Za-z][A-Za-z0-9_-]*', zid) or zid in seen:
            raise ValueError('Zone identifiers must be unique simple names')
        seen.add(zid)
        for key in ('switches', 'edgeServers', 'wifiAps', 'nrGnbs'):
            count(zone[key], key, 1 if key == 'switches' else 0)
        fields(zone['robots'], 'nr wifi')
        for access, attachment in [('nr', 'nrGnbs'), ('wifi', 'wifiAps')]:
            count(zone['robots'][access], access)
            if zone['robots'][access] and not zone[attachment]:
                raise ValueError(zid+': robots require corresponding radio infrastructure')
        position(zone['originM'])
        fields(zone['grid'], 'columns spacingM')
        count(zone['grid']['columns'], 'columns', 1)
        spacing = zone['grid']['spacingM']
        if type(spacing) not in (int, float) or not math.isfinite(spacing) or spacing <= 0:
            raise ValueError('spacingM must be finite and positive')
        total += sum(zone[k] for k in ('switches','edgeServers','wifiAps','nrGnbs')) + sum(zone['robots'].values())
    if total > 4096:
        raise ValueError('Inventory limit is 4096 devices; not a tested radio scale limit')
    devices = []
    def add(identifier, role, zone=None, **extra):
        devices.append(dict(id=identifier, role=role, zone=zone, **extra))
    for i in range(spec['sharedSpines']):
        add(f'spine-{i}', 'spine')
    for role in ('pgw', 'sgw', 'mme'):
        add('core-'+role, role)
    for z in spec['zones']:
        prefix = 'zone-'+z['id']+'-'
        origin = z['originM']
        for key, role in [('switches','switch'),('edgeServers','edge'),('wifiAps','ap'),('nrGnbs','gnb')]:
            for i in range(z[key]):
                extra = {}
                if role in ('ap','gnb'):
                    extra['positionM'] = [origin[0]+i*2, origin[1]-2, origin[2]+3]
                add(prefix+role+'-'+str(i), role, z['id'], **extra)
        ordinal = 0
        for access, role, key in [('nr','gnb','nrGnbs'),('wifi','ap','wifiAps')]:
            for i in range(z['robots'][access]):
                col = ordinal % z['grid']['columns']
                row = ordinal // z['grid']['columns']
                pos = [origin[0]+col*z['grid']['spacingM'], origin[1]+row*z['grid']['spacingM'], origin[2]]
                position(pos)
                add(prefix+access+'-robot-'+str(i), 'robot', z['id'], access=access,
                    attachment=prefix+role+'-'+str(i % z[key]), positionM=pos)
                ordinal += 1
    roles = {role: sum(d['role'] == role for d in devices) for role in sorted({d['role'] for d in devices})}
    return dict(schema='warehouse-wireless-inventory-v1', mobility='stationary',
                status='configuration-only; not connected to ns-3 runner',
                totalDevices=len(devices), counts=roles, devices=devices)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config', type=Path)
    parser.add_argument('--output', type=Path, help='Create resolved inventory; refuses overwrite')
    args = parser.parse_args()
    try:
        result = expand(json.loads(args.config.read_text(encoding='utf-8')))
        if args.output:
            with args.output.open('x', encoding='utf-8') as out:
                json.dump(result, out, indent=2, allow_nan=False)
                out.write('\n')
        print(json.dumps({k: result[k] for k in ('status','totalDevices','counts')}, indent=2))
    except (ValueError, OSError) as error:
        parser.exit(2, str(error)+'\n')


if __name__ == '__main__':
    main()
