# First run and installation handoff

Use a fresh source/recovery extraction, not a copied `build` or `.tools` directory from a
different computer. Open a terminal in the folder containing `CMakeLists.txt` and `scripts`.
The target is the MacBook Air M4 or Ubuntu 24.04 ARM64 in UTM. Commands below for those
platforms are prepared but **not executed on this Windows host**. This is not yet a fully
validated Mac/Ubuntu or NR product. See CURRENT_STATUS for the actual verification boundary.

## Ubuntu 24.04 in UTM

Use an ARM64 Ubuntu installation on the M4. In its terminal:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build libboost-json-dev python3 python3-venv
python3 -m venv .venv
. .venv/bin/activate
python -m pip install matplotlib==3.10.3
python scripts/first_run.py
```

The package-manager step needs internet and administrator permission. It supplies the C++
compiler, CMake, Ninja, Boost.JSON and Python; the project build does not install system files.
The ns-3 prerequisite families follow its [release installation guide](https://www.nsnam.org/docs/release/3.46/installation/singlehtml/index.html).

## macOS on the M4

Use a native Apple Silicon terminal. Install Xcode Command Line Tools (complete the installer
before continuing) and Homebrew from its [official site](https://brew.sh/) if not already present:

```sh
xcode-select --install
brew install cmake ninja boost python@3.12
python3.12 -m venv .venv
. .venv/bin/activate
python -m pip install matplotlib==3.10.3
python scripts/first_run.py
```

An “already installed” response to `xcode-select --install` is harmless.
Homebrew provides the versioned interpreter through its [Python 3.12 formula](https://formulae.brew.sh/formula/python@3.12).
Do not mix Intel Homebrew/compiler binaries with ARM64 libraries. Record `uname -m`, `python --version`,
`c++ --version` and `cmake --version` if setup fails. There are no Windows executable paths
or Codex-specific Python paths in these commands.

## What a successful first run means

The command configures a fresh host-specific build, compiles, runs all available core tests,
then exercises a small fixed-position warehouse example. Expected final message:

```text
PASS: first-run warehouse fixture, 180/180 delivered, zero deadline misses. OUTPUT: ...
```

The new `results/first-run-.../` folder contains `RUN_STATUS.json`, provenance, logs, and
`example/packets.csv`, class metrics and queue statistics. This confirms the independent
abstract-link simulator. It does not certify the warehouse or validate radio performance.
Repeated runs create separate directories and preserve old data.

After that succeeds, run the full warehouse comparison:

```sh
python scripts/run_all_experiments.py
```

This is the full 132-run / three-seed matrix. `--quick` is only a smoke check. Optional study
suites are `--suite protected-qos` and `--suite capacity-admission`; their known adverse results
are documented. The first-run fixture's zero misses are not a guarantee for these stress cases.

## Actual ns-3 packet-runtime check

Python 3.12+ is needed for safe source-archive extraction. Keep the environment above active:

```sh
python scripts/setup_ns3.py --jobs 4
python scripts/run_ns3_smoke.py
```

Setup pins ns-3.46.1 by SHA256, builds wired prerequisites and upstream UNIT QUICK tests,
and installs only beneath `.tools/ns3-wired-install`. This is a substantial first build.
With limited VM memory, use `--jobs 2`. Rerunning reuses the local source/build; do not delete
previous result directories. `--offline` requires the pinned archive already in `.tools`.
Only two documentation symlink aliases are omitted during Windows-compatible extraction.
Upstream suites run separately, as required by this ns-3 release. Some suites print nothing
until they finish: `threaded-simulator` took about 275 seconds on the Windows validation host.
The setup log names the current suite; a quiet interval alone is not a failure.

The second command requires real ns-3; it cannot silently fall back to the independent core.
It runs project tests, then a two-hop routed UDP fixture and deterministic replay. Expected:
three packets delivered, zero losses, **4.416 ms** mean delay, fixed positions. Tests also
check one-hop delay and actual receive-error loss after FlowMonitor's timeout.
The delay includes IPv4/UDP/PPP bytes; FlowMonitor byte counts are IP-layer counts.

`results/ns3-smoke-.../` preserves the measurements, dependency/library hashes and replay.
This is a small wired runtime fixture, **not schema-v2 warehouse forwarding or NR**. The old
NetAnim/Wi-Fi build failure is retained separately and is not fixed by excluding those modules.
NR must use a verified compatible ns-3 release; the [upstream compatibility table](https://cttc-lena.gitlab.io/nr/html/)
is authoritative. Do not mix the pinned wired build with an arbitrary NR release.

## Native wired warehouse check

After the small wired runtime check, run the supported schema-v2 warehouse acceptance cases:

```sh
python scripts/run_ns3_warehouse.py
```

This runs 12 native wired controls and 36 policy comparisons (three scenarios, three ECMP
hash seeds, four policies), auditing packet paths, headers, queues, deadlines, positions and
every replay. It uses declared supported inputs with identical offered traffic across policies. See the
[native wired warehouse guide](ns3-wired-warehouse.md) for limits and expected outputs.
Each native case/replay has a 60-second timeout; logs and process receipts live under
`processes/` in its result batch. A TIMEOUT/FAILED/RUNNING status is not successful validation.
Native failures, protected queues, NR and target-host execution remain pending.
See [the native policy declaration](ns3-policy-study.md) for interpretation and limits.

## If a command fails

| Symptom | Action |
|---|---|
| Missing cmake/ninja/c++ | Complete the OS prerequisites above and reopen the terminal if PATH changed. |
| Boost.JSON package not found | Install `libboost-json-dev` on Ubuntu or `boost` through native Homebrew; check architecture consistency. |
| Python below 3.12 | Use Python 3.12+ for `setup_ns3.py`; activate its virtual environment. |
| CMake cache names another host/compiler | Use a fresh extraction, or `scripts/verify.py --build-dir build/new-host`; preserve old evidence. |
| Checksum mismatch | Setup stops and retains the file. Do not disable checksum checks; record the URL/file/hash. |
| Compiler killed / out of memory | Reduce dependency setup jobs to 2. Preserve the failed build log. |
| ns-3 prefix absent | Complete `setup_ns3.py`; explicit `--ns3-prefix` never falls back to core-only verification. |
| Test or analytical delay mismatch | Stop interpreting results; preserve the exact log and generated batch. |
| No plots | Install the pinned matplotlib package in the same active Python environment. |

Project verification logs: `build/verified-bootstrap/verification.log` or
`build/verified-ns3/verification.log`. Dependency setup logs: `.tools/ns3-setup-*.log`.
Batch logs: the printed output directory's `execution.log`. Missing `RUN_STATUS.json`
means the batch did not complete, even when some output files exist.

## Windows validation host

With Python 3.12+ available, `python scripts/setup_portable.py --plots` installs the local
compiler/Boost prerequisites. Then use the same `first_run.py`, `setup_ns3.py` and
`run_ns3_smoke.py` commands. The scripts manage compiler/DLL paths in child environments;
no global PATH or registry edits are required. This host's success does not count as an
executed Mac/Ubuntu test.

For a separately installed compiler and an extracted official Boost tree, the verification
scripts also accept `EALR_BOOST_SOURCE` in the environment. It must point to a tree containing
`boost/json/src.hpp`; an invalid value fails explicitly. This supports offline source-package
testing without bundling the compiler or copying a machine-specific build cache.
