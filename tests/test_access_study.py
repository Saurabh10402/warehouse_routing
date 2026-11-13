import copy
import json
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from access_study import validate

class AccessStudyTests(unittest.TestCase):
    def setUp(self):
        self.c=json.loads((ROOT/'configs/warehouse_access_small.json').read_text())

    def test_authorization_independent_of_zone(self):
        result=validate(self.c)
        self.assertEqual([t['requiresMainServerAuthorization'] for t in result['tasks']],[False,True,True])
        self.c['authorization']['requiredTaskTypes']=['telemetry']
        self.assertEqual([t['requiresMainServerAuthorization'] for t in validate(self.c)['tasks']],[True,False,False])

    def test_counts(self):
        self.assertEqual(validate(self.c)['totalDevices'],19)
        large=json.loads((ROOT/'configs/warehouse_access_500.json').read_text())
        self.assertEqual(validate(large)['totalDevices'],500)
        self.assertEqual(validate(large)['robotCount'],471)

    def test_invalid_authorization(self):
        for key,value in [('authority','layer-one'),('onTimeout','allow'),('afterApproval','server-relay'),('timeoutMs',float('nan')),('cacheApprovals',True)]:
            c=copy.deepcopy(self.c); c['authorization'][key]=value
            with self.assertRaises(ValueError): validate(c)

    def test_topology_and_endpoints(self):
        for change in ('duplicate-link','unknown-robot','edge-server'):
            c=copy.deepcopy(self.c)
            if change=='duplicate-link': c['fabric']['directZoneLinks'].append({'from':'B','to':'A'})
            elif change=='unknown-robot': c['tasks'][0]['source']='missing'
            else: c['inventory']['zones'][0]['edgeServers']=1
            with self.assertRaises(ValueError): validate(c)

if __name__=='__main__': unittest.main()
