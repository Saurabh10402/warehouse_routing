# Build, reproduce, and recover

For the ordered first-time setup, expected outputs and troubleshooting, use
[the first-run guide](first-run.md). The recipes below retain the underlying developer commands.

## Core prerequisites

C++17 compiler, CMake >=3.22, Ninja, Boost.JSON and Python >=3.10 for the runner.
Matplotlib is optional and never required for C++ compilation or CTest. No ns-3 required.

Ubuntu 24.04 package setup (standard distribution packages; not executed on this Windows host):

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build libboost-json-dev python3 python3-matplotlib
python3 scripts/run_all_experiments.py
```

macOS (requires Xcode command-line tools/Homebrew; not executed here):

```sh
brew install cmake ninja boost python@3.12
python3.12 -m venv .venv
. .venv/bin/activate
python -m pip install -r scripts/requirements.txt
python scripts/run_all_experiments.py
```

On Windows with Python installed:

```powershell
python scripts/setup_portable.py --plots
python scripts/run_all_experiments.py
```

`setup_portable.py` verifies official archive checksums, extracts only inside `.tools`,
and uses portable GCC with bundled CMake/Ninja. It never edits the global PATH/registry.
It reuses existing verified archives and installed tools. Delete nothing to resume a failed run;
inspect a partial download or installation before retrying. It does not install ns-3 or WSL.

This session used the desktop's existing Python 3.12 executable (local convenience, not a project dependency):

```powershell
& "$env:USERPROFILE\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe" scripts/run_all_experiments.py
```

The runner prepends `.tools/mingw64/bin` in its own child environment and discovers local Boost.
If that desktop Python is unavailable later, install/use any compatible Python and run the same script.
Other platforms should use their compiler packages; portable Windows binaries do not work on a Mac.

Pinned archives used and verified:

| Dependency | SHA256 |
|---|---|
| WinLibs GCC 14.2.0 POSIX UCRT r3 zip | 88868d745b807f083a117ff69348d8bc021ad7389aa503379dbed1866efcaeb9 |
| Official Boost 1.85.0 tar.gz | be0d91732d5b0cc6fbb275c7939974457e79b54d6f07ce2e3dfdd68bef883b0b |

Sources: [WinLibs release](https://github.com/brechtsanders/winlibs_mingw/releases/tag/14.2.0posix-12.0.0-ucrt-r3),
[Boost release](https://archives.boost.io/release/1.85.0/source/).
Optional charts use matplotlib 3.10.3 in the portable setup. The first session also installed
standalone CMake 3.31.6/Ninja wheels; the compiler archive's bundled tools take precedence.

## Direct CMake commands

With installed dependency packages and no stale cache:

```sh
cmake --preset bootstrap
cmake --build --preset bootstrap
ctest --preset bootstrap
cmake --preset default
cmake --build --preset default
ctest --preset default
```

The recovered archive contains Mac-specific caches in `build/bootstrap` and `build/default`.
Keep them as historical evidence; do not reuse them on Windows. The verified commands use:

```sh
python scripts/verify.py --preset bootstrap
python scripts/verify.py --preset default
```

These configure `build/verified-bootstrap` and `build/verified-default`. A new explicit
`--build-dir` is available for a different host/compiler, avoiding destructive cache cleanup.
The underlying configure is `cmake --preset bootstrap -B BUILD_DIR` plus Python discovery
and `-DEALR_BOOST_SOURCE=PATH_TO_BOOST` when the portable source tree is present.
Compiler warnings remain enabled. Verification logs append under the selected build directory.

## Experiment outputs and interpretation

`python scripts/run_all_experiments.py` performs configure/build/tests, 132 primary comparison runs
in the current matrix, raw artifact validation, a replay, aggregate CSVs and optional plots.
The primary scenarios are documented in `docs/warehouse-study.md`: same/cross-zone and
edge-service traffic with four forwarding/scheduling policies and three seeds each.
`python scripts/run_all_experiments.py --suite legacy-ealr` retains the earlier 114-run suite.
`python scripts/run_all_experiments.py --suite protected-qos` runs 144 protected-queue comparisons.
To regression-check the four-policy baseline, add `--baseline results/experiments-20261030T140941Z-dd164497`
to the default command; this requires identical configuration and packet traces in all 132 runs.
Those legacy scenarios include baseline, 500 endpoints, uneven zones, 10/40/100 scaling, congestion,
priority mix/FIFO ablation, energy/congestion-weight ablations, an energy/latency tradeoff, link/spine/edge failures,
complete outage, and cloud-independent safety. Random-phase workloads use seeds 1,2,3.

Results are written to `results/experiments-UTC-UNIQUE/`. Never rename/overwrite a previous
run to disguise a changed input. Each run has an exact config snapshot and source/config/
executable hashes. The root provenance contains all source-file hashes. Metrics and paths
are checked against raw packet observations. `RUN_STATUS.json` is written only after success.
If it is absent, inspect `execution.log`; the batch is incomplete even if some runs succeeded.

Full experiments are intentionally distinct from unit tests. `--quick` is a single-seed smoke
matrix and should not be substituted for the full stochastic comparison in a report.
`comparison.csv`, `priority_comparison.csv`, `scope_priority_comparison.csv`, `aggregate.csv`,
and `graphs/` are the main comparisons.
Plots are grouped by identical scenario; bars are seed means with sample-standard-deviation
error bars. A single sample has no estimated variability. Raw per-priority observations remain
available even when an aggregate plot hides class differences.

## Optional ns-3

The wired prerequisite workflow is `python scripts/setup_ns3.py` (Python 3.12+), then
`python scripts/run_ns3_smoke.py`. It isolates NetAnim as optional, installs only under
`.tools/ns3-wired-install`, and checks routed UDP delivery/delay and loss. See the current
checkpoint for actual executed status. Schema-v2 warehouse/NR integration is separate.

`scripts/verify.py --ns3-prefix PREFIX` implies the default preset and requires the specified
package. It fails on missing targets instead of silently validating core-only code; an
explicit prefix cannot be satisfied by an unrelated globally installed ns-3 package.

The default preset emits an explicit warning when required modules are unavailable. A successful
core-only default build is not evidence that ns-3/NR was tested. See `docs/ns3-integration.md`
and the current checkpoint for actual external build attempts. ns-3 should be built/installed
with its matching release instructions and an appropriate compiler; then expose its prefix
via CMAKE_PREFIX_PATH and use a new build directory. NR versions must match ns-3 versions.

## Checkpoints and portable handoff

The original zip is preserved at `.recovery/original.zip`, SHA256
`6e0e84ba7f075737da9299ec3520bb875404556a9199a9b92468b938e6da1838`.
The clean extraction is `.recovery/original/`. Original source checkpoint commit: `d632f2c`.
The first validated stationary simulator milestone is `d2f34e9`.

Read checkpoint state before editing. A manifest hashes exact source bytes, excluding builds,
tools, results and checkpoint metadata so a status update does not invalidate source identity.
Verify using `python scripts/checkpoint.py CHECKPOINT_ID --verify`. A mismatch must be inspected,
not erased. Manifests are created exclusively and never overwrite an earlier checkpoint.

Git commits preserve source history. Generated results are ignored by Git; source commits alone
do not preserve experiment data. `scripts/package_recovery.py` produces a handoff zip containing
source, checkpoint evidence, selected experiment outputs and a Git bundle, without copying
large build/tool directories. Keep the original archive separately. Restore source into a clean
directory and run the same verification command on the destination platform.
