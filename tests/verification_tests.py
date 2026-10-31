"""Failure paths must not silently claim that ns-3 or a modified dependency was verified."""
import hashlib
import io
import os
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import setup_ns3
import verify
import checkpoint


class VerificationTests(unittest.TestCase):
    def test_upstream_suites_run_separately_and_require_matching_pass_reports(self):
        with tempfile.TemporaryDirectory() as tmp:
            log=Path(tmp)/'units.log'
            def execute(command, env, destination):
                with destination.open('a',encoding='utf-8') as output:
                    output.write('+ '+' '.join(map(str,command))+'\n')
                    if '--print-test-name-list' in command:
                        output.write('suite-one\nsuite with spaces\n')
                    else:
                        name=next(arg.removeprefix('--test-name=') for arg in command if arg.startswith('--test-name='))
                        output.write('PASS '+name+' 0.001 s\n')
            with patch.object(setup_ns3,'execute',side_effect=execute) as run:
                self.assertEqual(setup_ns3.run_unit_suites('runner',{},log),2)
                self.assertEqual(run.call_count,3)
            # A successful process exit without suite PASS records is insufficient.
            def silent(command,env,destination):
                destination.write_text('suite-one\n' if '--print-test-name-list' in command else '')
            with patch.object(setup_ns3,'execute',side_effect=silent):
                with self.assertRaisesRegex(SystemExit,'reports do not match'):
                    setup_ns3.run_unit_suites('runner',{},Path(tmp)/'silent.log')

    def test_source_archive_does_not_inherit_an_enclosing_repository_commit(self):
        with patch.object(checkpoint.shutil,'which',return_value='git'),patch.object(checkpoint.subprocess,'run') as run:
            run.return_value.returncode=0
            run.return_value.stdout=str(checkpoint.ROOT.parent)
            self.assertEqual(checkpoint.git_commit(),'unavailable (source archive)')
            self.assertEqual(run.call_count,1)

    def test_source_archive_can_record_provenance_without_git(self):
        with patch.object(checkpoint.shutil,'which',return_value=None),patch.object(checkpoint.subprocess,'run') as run:
            self.assertEqual(checkpoint.git_commit(),'unavailable (source archive)')
            run.assert_not_called()

    def test_failed_command_keeps_diagnostics_and_exit_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            log=Path(tmp)/'run.log'
            with self.assertRaises(SystemExit) as failure:
                verify.execute([sys.executable,'-c',"print('diagnostic before failure'); raise SystemExit(7)"],os.environ.copy(),log)
            self.assertEqual(failure.exception.code,7)
            self.assertIn('diagnostic before failure',log.read_text())

    def test_explicit_ns3_cannot_be_disabled_or_missing(self):
        with self.assertRaisesRegex(SystemExit,'bootstrap explicitly disables'):
            verify.verify('bootstrap',require_ns3=True)
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(SystemExit,'prefix does not exist'):
                verify.verify('default',ns3_prefix=Path(tmp)/'absent')

    def test_unix_never_prepends_windows_toolchain(self):
        with patch.object(verify.os,'name','posix'),patch.dict(os.environ,{'PATH':'/usr/bin'}):
            self.assertNotIn('mingw',verify.environment()['PATH'])

    @unittest.skipIf(sys.version_info<(3,12),'Source extraction tool requires Python 3.12+')
    def test_dependency_hash_and_source_change_are_rejected_without_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);archive=root/'source.tar.bz2'
            with tarfile.open(archive,'w:bz2') as tar:
                data=(setup_ns3.VERSION+'\n').encode()
                member=tarfile.TarInfo('ns-'+setup_ns3.VERSION+'/VERSION');member.size=len(data)
                tar.addfile(member,io.BytesIO(data))
            with self.assertRaisesRegex(ValueError,'checksum mismatch'):
                setup_ns3.source_tree(archive,root)
            with patch.object(setup_ns3,'SHA256',hashlib.sha256(archive.read_bytes()).hexdigest()):
                source=setup_ns3.source_tree(archive,root)
                self.assertEqual(setup_ns3.source_tree(archive,root),source)
                # Version text still strips to the same value, but exact archive identity differs.
                (source/'VERSION').write_text(setup_ns3.VERSION+'  \n')
                with self.assertRaisesRegex(ValueError,'source differs'):
                    setup_ns3.source_tree(archive,root)
                self.assertEqual((source/'VERSION').read_text(),setup_ns3.VERSION+'  \n')

    def test_dependency_path_traversal_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);archive=root/'source.tar.bz2'
            with tarfile.open(archive,'w:bz2') as tar:
                member=tarfile.TarInfo('ns-'+setup_ns3.VERSION+'/../../escape');member.size=1
                tar.addfile(member,io.BytesIO(b'x'))
            with patch.object(setup_ns3,'SHA256',hashlib.sha256(archive.read_bytes()).hexdigest()):
                with self.assertRaisesRegex(ValueError,'Unexpected archive path'):
                    setup_ns3.source_tree(archive,root)
            self.assertFalse((root/'escape').exists())


if __name__=='__main__':unittest.main()
