import sys,unittest,tempfile,shutil,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from audit_access_ecmp import check_ecmp
from run_access_authorization import capabilities

class EcmpAudit(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=Path(self.temp.name)
        for name in ('input.json','original.log'):
            shutil.copyfile(ROOT/'tests/fixtures/access-ecmp'/name,self.folder/name)
        self.trace=(self.folder/'original.log').read_text()
    def test_real_wire_choices(self):
        audit=check_ecmp(self.folder)
        self.assertEqual(audit['wireHopsChecked'],210)
        self.assertEqual(audit['spinesByPhase'],{'before':['spine-0','spine-1'],'during':['spine-1'],'after':['spine-0','spine-1']})
        self.assertGreater(audit['repeatedFlowGroups'],0)
        self.assertGreater(audit['restoredFlowMappings'],0)
    def test_wrong_equal_cost_choice(self):
        lines=self.trace.splitlines()
        for i,line in enumerate(lines):
            p=line.split()
            if p[0]=='WIRE' and p[1]=='zone-A-switch-0' and p[2].startswith('spine-') and int(p[7])<2000000000:
                p[2]='spine-1' if p[2]=='spine-0' else 'spine-0';lines[i]=' '.join(p);break
        else:self.fail('No switch choice in fixture')
        (self.folder/'original.log').write_text('\n'.join(lines))
        with self.assertRaisesRegex(ValueError,'flow hash'):check_ecmp(self.folder)
    def test_unknown_packet_not_skipped(self):
        (self.folder/'original.log').write_text(self.trace+'\nWIRE zone-A-switch-0 spine-0 192.168.0.2 192.0.2.1 20000 42 1500000000\n')
        with self.assertRaisesRegex(ValueError,'Unmapped'):check_ecmp(self.folder)
    def test_uninitialized_source_rejected(self):
        lines=self.trace.splitlines()
        for i,line in enumerate(lines):
            parts=line.split()
            if parts[0]=='WIRE':
                parts[3]='102.102.102.102';lines[i]=' '.join(parts);break
        (self.folder/'original.log').write_text('\n'.join(lines))
        with self.assertRaisesRegex(ValueError,'Unassigned ECMP packet source'):check_ecmp(self.folder)
    def test_wrong_owner(self):
        (self.folder/'original.log').write_text(self.trace.replace('ECMP_DEST zone-A-wifi-robot-0 192.168.0.2 zone-A-ap-0','ECMP_DEST zone-A-wifi-robot-0 192.168.0.2 core-pgw'))
        with self.assertRaisesRegex(ValueError,'owner'):check_ecmp(self.folder)
    def test_capability_scope(self):
        c=json.loads((self.folder/'input.json').read_text())
        self.assertEqual(capabilities(c)['totalDevices'],19)
        with self.assertRaisesRegex(ValueError,'Boundary-loss'):capabilities(c,boundary=True)

if __name__=='__main__':unittest.main()
