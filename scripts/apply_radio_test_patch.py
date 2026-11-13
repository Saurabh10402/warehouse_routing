#!/usr/bin/env python3
"""Apply only the reviewed ns-3.46 Wi-Fi test identity corrections."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

REVISION = 'ea50b72ab79b1ec5912c66d5b384effa829f18bc'
PATCH = Path(__file__).resolve().parent / 'patches/ns3-3.46-test-packet-identity.patch'


def apply(source):
    source = source.resolve()
    def git(*args, check=True):
        return subprocess.run(['git', '-C', str(source), *args], check=check,
                              capture_output=True, text=True)
    if git('rev-parse', 'HEAD').stdout.strip() != REVISION:
        raise SystemExit('Refusing: expected the pinned ns-3.46 revision.')
    # Compare the complete tracked diff with this patch in a temporary index-free
    # check. Existing unrelated edits must not be silently accepted or overwritten.
    names = git('diff', 'HEAD', '--name-only').stdout.splitlines()
    targets = [line[6:] for line in PATCH.read_text().splitlines() if line.startswith('+++ b/')]
    if set(names) - set(targets):
        raise SystemExit('Refusing: unrelated tracked dependency changes exist.')
    patch_args = ['--ignore-space-change', '--ignore-whitespace', str(PATCH)]
    if names:
        # Require exact patched bytes, not merely a reverse-applicable hunk.
        import tempfile
        with tempfile.TemporaryDirectory(prefix='radio-patch-check-') as tmp:
            expected = Path(tmp)
            for name in targets:
                path = expected/name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(subprocess.check_output(['git','-C',str(source),'show',f'HEAD:{name}']))
            subprocess.run(['git','apply','--unsafe-paths',*patch_args],cwd=expected,check=True)
            if any((source/name).read_bytes() != (expected/name).read_bytes() for name in targets):
                raise SystemExit('Refusing: existing edits differ from the approved test patch.')
        print('Already applied; exact test source verified.')
    else:
        git('apply', '--check', *patch_args)
        git('apply', *patch_args)
        print('Applied test-only corrections; rebuild and validation required.')
    print(json.dumps({'ns3_revision': REVISION,
                      'patch_sha256': hashlib.sha256(PATCH.read_bytes()).hexdigest(),
                      'test_files': targets}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    apply(parser.parse_args().source)
