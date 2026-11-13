import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'scripts'))
from wireless_layout import expand


class WirelessLayoutTests(unittest.TestCase):
    def setUp(self):
        self.spec = json.loads((ROOT/'configs/warehouse_wireless_layout.json').read_text())

    def test_inventory_and_attachments(self):
        result = expand(self.spec)
        self.assertEqual(result['totalDevices'], 500)
        self.assertEqual(result['counts']['robot'], 463)
        devices = {d['id']: d for d in result['devices']}
        self.assertEqual(len(devices), 500)
        for robot in (d for d in devices.values() if d['role'] == 'robot'):
            target = devices[robot['attachment']]
            self.assertEqual(target['zone'], robot['zone'])
            self.assertEqual(target['role'], {'nr':'gnb','wifi':'ap'}[robot['access']])
        self.assertEqual(result, expand(self.spec))

    def test_missing_radio_rejected(self):
        self.spec['zones'][0]['nrGnbs'] = 0
        with self.assertRaises(ValueError): expand(self.spec)

    def test_invalid_values_rejected(self):
        for key, value in [('switches',True),('edgeServers',-1),('originM',[0,float('nan'),1])]:
            trial = copy.deepcopy(self.spec)
            trial['zones'][0][key] = value
            with self.assertRaises(ValueError): expand(trial)
        self.spec['mobility'] = 'random'
        with self.assertRaises(ValueError): expand(self.spec)

    def test_duplicates_and_unknown_fields(self):
        self.spec['zones'][1]['id'] = 'A'
        with self.assertRaises(ValueError): expand(self.spec)
        self.spec['zones'][1]['id'] = 'B'
        self.spec['zones'][0]['robts'] = 10
        with self.assertRaises(ValueError): expand(self.spec)

    def test_counts_are_configurable(self):
        self.spec['zones'] = self.spec['zones'][:1]
        z = self.spec['zones'][0]
        z['robots'] = {'nr':3,'wifi':0}
        z['wifiAps'] = 0
        z['nrGnbs'] = 2
        result = expand(self.spec)
        self.assertEqual(result['totalDevices'], 12)
        self.assertEqual(result['counts']['robot'], 3)


if __name__ == '__main__':
    unittest.main()
