#!/usr/bin/env python3
"""Optional Windows-only local prerequisites; pinned official archives, no system changes."""
import argparse
import hashlib
import os
import subprocess
import sys
import urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
TOOLS=ROOT/'.tools'
GCC='https://github.com/brechtsanders/winlibs_mingw/releases/download/14.2.0posix-12.0.0-ucrt-r3/winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64ucrt-12.0.0-r3.zip'
BOOST='https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz'
def download(url,name,checksum):
    path=TOOLS/name
    if not path.exists():
        partial=path.with_suffix('.partial')
        print('Downloading '+url,flush=True);urllib.request.urlretrieve(url,partial)
        if hashlib.sha256(partial.read_bytes()).hexdigest()!=checksum:raise SystemExit('Checksum mismatch: '+name)
        partial.rename(path)
    if hashlib.sha256(path.read_bytes()).hexdigest()!=checksum:raise SystemExit('Checksum mismatch: '+name)
    return path

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--plots',action='store_true');args=parser.parse_args()
    if os.name!='nt':raise SystemExit('Use your OS package manager for CMake, Ninja, C++17 and Boost.JSON; see docs/reproducibility.md')
    TOOLS.mkdir(exist_ok=True)
    gcc=download(GCC,'gcc.zip','88868d745b807f083a117ff69348d8bc021ad7389aa503379dbed1866efcaeb9')
    boost=download(BOOST,'boost.tar.gz','be0d91732d5b0cc6fbb275c7939974457e79b54d6f07ce2e3dfdd68bef883b0b')
    if not (TOOLS/'mingw64/bin/g++.exe').exists():subprocess.run(['tar','-xf',str(gcc),'-C',str(TOOLS)],check=True)
    if not (TOOLS/'boost_1_85_0/boost/json/src.hpp').exists():subprocess.run(['tar','-xzf',str(boost),'-C',str(TOOLS),'boost_1_85_0/boost','boost_1_85_0/libs/json/src','boost_1_85_0/LICENSE_1_0.txt'],check=True)
    # WinLibs archive includes CMake and Ninja; no global PATH or registry modifications.
    for name in ['g++.exe','cmake.exe','ninja.exe','ctest.exe']:
        if not (TOOLS/'mingw64/bin'/name).is_file():raise SystemExit('Incomplete portable toolchain: '+name)
    if args.plots and not (TOOLS/'plotting/matplotlib').is_dir():
        subprocess.run([sys.executable,'-m','pip','install','--target',str(TOOLS/'plotting'),'matplotlib==3.10.3'],check=True)
    print('Prerequisites verified. Run: python scripts/run_all_experiments.py')
