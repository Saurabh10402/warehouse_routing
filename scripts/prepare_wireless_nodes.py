"""Export a validated inventory for the native radio-construction control."""
import argparse
import json
from pathlib import Path
from wireless_layout import expand


def export(config, output):
    inventory = expand(json.loads(config.read_text(encoding='utf-8')))
    with output.open('x', encoding='utf-8') as stream:
        for device in inventory['devices']:
            if device['role'] in ('pgw', 'sgw', 'mme'):
                continue  # Created by the EPC helper, not duplicate placeholders.
            xyz = device.get('positionM', [0, 0, 0])
            row = [device['id'], device['role'], device.get('access', '-'),
                   device.get('attachment', '-'), *map(str, xyz)]
            stream.write(' '.join(row)+'\n')
    return inventory


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = export(args.config, args.output)
    print(f"Prepared {result['totalDevices']} modeled nodes; no simulation executed")
