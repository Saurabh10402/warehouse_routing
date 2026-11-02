# Warehouse routing simulation — start here

## Show the working demonstrations now

1. Extract the entire delivery ZIP into a normal folder. Do not open files inside the ZIP preview.
2. Open **OPEN_WAREHOUSE.html** in that folder with Chrome, Edge, Firefox, Safari or Brave.
3. Choose the baseline or spine-failure demonstration.

The saved viewers need no installation, server or Internet. Keep the folder structure intact. These pages display verified simulation results; clicking controls does not run a new simulation. Do not open the source template under `scripts/`.

## A three-minute presentation

**Topology:** eight zones, 482 stationary robots, eight zone switches, eight local edge servers and two shared spines: 500 devices and 506 wired links. Device counts are editable. This is a wired IP-routing simulation; it does not model wireless or private 5G radio behavior.

**Baseline:** select `A-same-zone` to show robot → zone switch → robot. Select `A-cross-zone` to show robot → source switch → spine → destination switch → robot. Select `A-edge-service` to show the local server path. Use Next hop to explain the recorded route. The displayed times come from native packet observations; stepping is not time-scaled animation.

**Failure:** open the spine-failure viewer, choose `A-cross-zone`, then compare:

| Packet | Generated | Observation |
|---|---|---|
| #6 | 20 ms | Route through shared-spine-1 |
| #7 | 40 ms | Spine 1 is down; route uses shared-spine-0 |
| #9 | 80 ms | Spine 1 has recovered; original route restored |

The failure occurs at 30 ms and recovery at 70 ms. Six cross-zone flows rerouted. All 120 packets arrived, and all 40 same-zone peer packets stayed off the spines. Exact replay and independent trace audits passed.

**State the limits:** 24 representative flows, not 482 robots simultaneously sending. Failure knowledge and route updates are immediate and centralized. Packets are spaced away from failure boundaries. These observations do not guarantee hardware convergence time, capacity or deployment deadlines.

## Change device counts and run a new simulation

The additional **traffic from all 482 robots** viewer shows a separate, validated workload: every robot sends five packets, for 2,410 packets total. All delivered in this test. One flow per robot rotates among local-peer, cross-zone and service traffic; this is not a saturated-load or deployment reliability guarantee. To reproduce it after setup:

```sh
python scripts/run_warehouse_layout.py --layout configs/warehouse_layout_all_robots.json
```

That JSON's optional `traffic` object controls mode, scope, packet interval, payload size, priority and number of service candidates. See docs/warehouse-demo.md for the exact fields and bounds. The original configs/warehouse_layout.json remains unchanged and still generates 24 representative flows.

Edit **configs/warehouse_layout.json**. Each zone specifies its ID, robots, switches and edge servers. Adding/removing zone entries changes the zone count. `sharedSpines` controls the common backbone. The reference robot counts are 59, 61, 58, 62, 60, 63, 59 and 60; they remain fixed between runs for reproducibility.

First install the prerequisites following **docs/first-run.md** for your operating system. Native execution requires Python 3.12+, a compatible C++ toolchain, CMake/Ninja, Boost.JSON and the project's pinned wired ns-3.46.1 installation. Toolchains and build directories are deliberately excluded from this ZIP. First-time setup requires downloads and a substantial build; it is separate from opening the viewers.

With the configured Python environment active, run from the extracted project folder:

```sh
python scripts/setup_ns3.py --jobs 2
python scripts/run_ns3_smoke.py
python scripts/run_warehouse_layout.py
python scripts/run_warehouse_layout.py --spine-outage
```

Use `python3` where that is your interpreter name. On Windows, install ordinary Python and follow the portable-toolchain instructions in docs/reproducibility.md before ns-3 setup. No Codex installation is required. Setup scripts preserve their logs and existing results.

Each demo prints a new result directory. To create its viewer:

```sh
python scripts/visualize_warehouse.py --batch results/YOUR_PRINTED_BATCH --output my-new-viewer.html
```

Replace `YOUR_PRINTED_BATCH` with the directory printed by the run. Use a new output filename; the exporter intentionally refuses overwrites. Open that new HTML. The delivered menu continues showing the preserved reference demonstrations, not your newly generated run.

The baseline native process took about 107 seconds and the outage process about 240 seconds on the Windows validation host; replay repeats each execution. Compilation and auditing add time. These are observations, not target-platform runtime promises. macOS M4 and Ubuntu ARM64 build/run instructions are supplied but have not been executed on those platforms.

## Check and recover the delivery

With Python installed, before editing any packaged files:

```sh
python scripts/verify_delivery.py
```

Expected: PASS for all file hashes and local menu/viewer links. Intentional edits cause hash mismatches; keep the original ZIP as your recovery copy. The package includes source checkpoints, raw baseline/outage/replay results and a Git bundle under recovery_packages. Historical provenance may contain the original Windows paths as evidence; these are not executable dependencies for the viewers or current scripts.

If the menu cannot find a viewer, extract the complete ZIP again into a new folder. If a simulation fails, preserve the printed batch, RUN_STATUS.json and logs; do not label partial output successful. For setup troubleshooting and verification boundaries, read docs/first-run.md and docs/warehouse-demo.md.

On Windows, extract the ZIP into a short folder such as `C:\Warehouse` to keep the full regression-data paths below Windows path-length limits. Keep the extracted folder structure intact.
