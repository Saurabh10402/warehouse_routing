# Install the stationary Wi-Fi / NR simulator

This workflow targets Ubuntu 24.04, including Ubuntu inside Windows WSL2.
Native Windows radio builds and macOS are not validated. The older wired
installer is a separate workflow. The repository must include
`scripts/setup_wireless.py`; unpublished local changes will not appear in an
older GitHub clone.

## 1. Prepare Ubuntu

On Windows, install Ubuntu 24.04 using `wsl --install -d Ubuntu-24.04` in an
administrator PowerShell window. Restart if requested, open Ubuntu and complete
its user setup. Run the following commands in the Ubuntu terminal, not PowerShell:

```sh
sudo apt update
sudo apt install -y git python3 cmake ninja-build g++ libeigen3-dev libsqlite3-dev
git clone https://github.com/Saurabh10402/warehouse_routing.git
cd warehouse_routing
```

Use the Linux home directory for compilation. Leave several GB free and keep
the computer awake. Internet is needed for package installation and Git clones.
The existing validated environment uses GCC 13.3 and Eigen 3.4.0 from Ubuntu.
Package updates can change the compiler/libraries; the fresh run's logs and
receipts are the authority for that installation.

## 2. Build and validate

Choose a destination that does not exist:

```sh
python3 scripts/setup_wireless.py "$HOME/warehouse-wireless" --jobs 2
```

The installer checks prerequisites, clones exact ns-3.46 and NR 4.1.1 commits,
applies the reviewed test-only Wi-Fi corrections, configures RelWithDebInfo with
assertions/logging and builds examples/tests. It then runs upstream QUICK tests
and the small warehouse control with original/replay accounting audits. It does
not run the 500-device experiment automatically. First compilation can take much
longer than a small simulation; progress is recorded in the printed log files.

Read `~/warehouse-wireless/setup-status.json`. `VALIDATED` means all these
commands completed successfully. The small run's result directory appears in
`setup-logs/warehouse-smoke.log`; inspect its `status.json` for delivery and
deadline outcomes separately. Successful accounting is not a deadline guarantee.
`FAILED` or `INTERRUPTED` is not a successful installation. Preserve logs; the
installer refuses existing destinations and does not erase a failed attempt.

`--configure-only` stops before compilation and records `CONFIGURED_NOT_BUILT`.
It is an installation preflight, not proof that tests or simulations work.
`--ns3-origin` and `--nr-origin` can select local Git mirrors for offline
dependency copying; exact commit checks still apply.

## 3. Run and view

From the repository directory:

```sh
python3 scripts/run_access_authorization.py configs/access_500_device_probe.json --probe-500 --outcome-measurement --ns3-source "$HOME/warehouse-wireless/ns-3" --build "$HOME/warehouse-wireless/build"
```

This performs an original run and replay. On the existing PC these together took
roughly one hour per seed. Do not start competing builds or simulations. Use the
printed result directory in the export command:

```sh
python3 scripts/visualize_access.py results/YOUR_RESULT output/my-wireless-run.html
```

Open the exported HTML in a browser. It is self-contained and can be copied to
another PC. See [viewer and configuration instructions](wireless-quickstart.md).

## Validation status

On 15 November 2026, a separate dependency tree created from local pinned Git
mirrors completed all 1,609 build steps in Ubuntu/WSL. The patched upstream QUICK
suite finished with 729 passes, two intentional long-example skips, zero failures
and zero crashes. The small ECMP warehouse control delivered all 36 tasks within
their configured deadlines, with byte-identical replay and independent outcome,
routing, coverage and radio-setting re-audits.

Result: `results/access-auth-20261115T111359Z-8054dd3d` (local generated artifact).
The configure-only installer and subsequent build/test/smoke stages were exercised
in this isolated tree. This is not a clean OS test or an internet-clone test, and
the full installer has not yet been exercised as one uninterrupted invocation.
The existing environment's separate three-seed 500-device results remain valid;
this installation check did not repeat that large study.
