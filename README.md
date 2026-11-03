# Warehouse routing simulation — installation and usage

A configurable stationary warehouse network with native wired ns-3 routing, Dijkstra/ECMP comparison, priority queues, failure/recovery demonstrations and an offline packet-path viewer. The supplied layout has **8 zones and 500 devices: 482 robots, 8 zone switches, 8 local edge servers and 2 shared spines**.

## Complete project report

[Download the illustrated 24-page PDF report](https://github.com/Saurabh10402/warehouse_routing/releases/download/checkpoint-022/warehouse-project-report-20261103.pdf). It covers architecture, features and development history, measured tables and graphs, the visualization walkthrough, installation, limitations and next steps. Prepared 3 November 2026 using evidence through checkpoint024; the demonstration ZIP remains the unchanged checkpoint022 snapshot.

The report text is preserved in [docs/project-report-source.md](docs/project-report-source.md). The optional `scripts/build_project_report.py` requires ReportLab and the original result/screenshot folders referenced in the script; these are reporting dependencies, not requirements for viewing the PDF or running the simulator.

## 1. View the working demonstrations (no installation required)

1. Open [the delivery release](https://github.com/Saurabh10402/warehouse_routing/releases/tag/checkpoint-022).
2. Under **Assets**, download **warehouse-demo-checkpoint-022.zip**. Do not select GitHub's automatic “Source code” ZIP: it omits generated results.
3. Extract the **entire** archive. On Windows use a short folder such as `C:\Warehouse` (right-click → Extract All). On macOS/Linux use a short folder in your home directory.
4. Find the extracted folder containing `OPEN_WAREHOUSE.html`, `START_HERE.md` and `results`.
5. Double-click **OPEN_WAREHOUSE.html**, or open it through your browser's File → Open menu. Keep the folder structure intact.
6. Select a demonstration:

| Demonstration | Recorded behavior |
|---|---|
| Working topology viewer | 500 devices, 24 representative flows, 120 packets |
| Traffic from all 482 robots | One flow per robot, 2,410 packets |
| Dijkstra and ECMP comparison | Identical arrivals; one Dijkstra case and three ECMP hash seeds |
| Spine failure | Alternate-spine routing during failure and recovery |
| Zone-switch failure | Zone A isolation while unaffected communication continues |

### Viewer controls

1. Choose a **Traffic flow**, then a **Recorded packet**.
2. Read its status, latency and observed path. Green links show the actual recorded route.
3. Use **Next hop**, **Previous hop**, and **Show full path** to inspect the route.
4. Toggle **Show all robots** to simplify the diagram; this does not change the simulation.
5. Click a device for its identity; read **Availability events** for failure state at packet creation.
6. In the zone-switch demo, keep the first flow and compare #1 (20 ms), #2 (40 ms), and #4 (80 ms). The `no_route` outcome during failure is intentional. Zone B flow `zone-B-robot-0-same-zone`, packet #297, still delivers at 40 ms.
7. In the spine-failure demo, select `A-cross-zone` and compare #6, #7 and #9.

This website displays saved observations: controls do not rerun routing or inject failures. Hop stepping is not real-time animation; the layout is schematic. **Do not open `scripts/warehouse_viewer.html`**, which is an unpopulated source template.

No Python, ns-3, Internet connection or server is needed to view the extracted demos. If your browser restricts local HTML, optionally run this from the extracted folder with Python installed:

```sh
python -m http.server 8000 --bind 127.0.0.1
```

Open `http://127.0.0.1:8000/OPEN_WAREHOUSE.html`; keep the terminal open and press Ctrl+C to stop. Use `python3` if that is your system's Python command.

Optional archive integrity check:

```sh
python scripts/verify_delivery.py
```

This checks delivery hashes and local links. It requires the delivery ZIP's `DELIVERY_MANIFEST.json`, which is not present in a normal source clone.

## 2. Obtain source to run new simulations

Install [Git](https://git-scm.com/downloads) and [Python 3.12](https://www.python.org/downloads/) if needed, then:

```sh
git clone https://github.com/Saurabh10402/warehouse_routing.git
cd warehouse_routing
```

Alternatively use **Code → Download ZIP**, extract it and open a terminal in its root. All following commands run from the directory containing `CMakeLists.txt`, `configs` and `scripts`. A source clone has no saved result folders; generate your own or download the delivery asset.

### Windows PowerShell — validated platform

1. Install Python 3.12 and reopen PowerShell. Check `py -3.12 --version`.
2. Create a project-local Python environment:

```powershell
py -3.12 -m venv .venv
```

3. Install pinned portable compiler/build tools, Boost and plotting dependencies:

```powershell
.\.venv\Scripts\python.exe scripts/setup_portable.py --plots
```

This downloads checksum-verified dependencies into `.tools`; it does not edit global PATH or the registry. Internet access and disk space are required. The Windows package is x86-64; Windows ARM is not validated.

4. Run the independent core check:

```powershell
.\.venv\Scripts\python.exe scripts/first_run.py
```

Expected fixture: **180/180 delivered, zero deadline misses**. This verifies the abstract-link core, not native ns-3.

**For every later command written as `python ...`, use `.\.venv\Scripts\python.exe ...` in Windows PowerShell.** You do not need to activate the environment or change execution policy.

### Ubuntu 24.04 — prepared, not runtime-validated

```sh
sudo apt update
sudo apt install git build-essential cmake ninja-build libboost-json-dev python3 python3-venv
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r scripts/requirements.txt
python scripts/first_run.py
```

Check `python --version`: native dependency extraction needs Python 3.12+. These Ubuntu steps have not been executed as part of the Windows validation.

### macOS / Apple Silicon — prepared, not runtime-validated

Install Xcode Command Line Tools and Homebrew from [brew.sh](https://brew.sh/) if absent:

```sh
xcode-select --install
```

Complete the installer before proceeding:

```sh
brew install cmake ninja boost python@3.12
python3.12 -m venv .venv
. .venv/bin/activate
python -m pip install -r scripts/requirements.txt
python scripts/first_run.py
```

Use native ARM64 tools on Apple Silicon; do not mix Intel and ARM64 dependencies. Never copy build/toolchain folders between operating systems. See [the first-run guide](docs/first-run.md).

## 3. Install and check native wired ns-3

After your platform setup, use the same Python environment:

```sh
python scripts/setup_ns3.py --jobs 4
python scripts/run_ns3_smoke.py
```

The installer downloads pinned **ns-3.46.1**, checks its SHA256, builds the wired runtime and installs it beneath `.tools/ns3-wired-install`. The first build can take substantial time. Use `--jobs 2` on low-memory PCs. Some upstream tests are quiet for several minutes; preserve logs on failure rather than deleting evidence.

The smoke fixture should deliver **3 packets with 4.416 ms mean delay** and deterministic replay. It cannot silently fall back to the abstract core. This setup does not install 5G-LENA/NR.

## 4. Run a warehouse simulation and export its website

Representative workload:

```sh
python scripts/run_warehouse_layout.py
```

Traffic from every robot:

```sh
python scripts/run_warehouse_layout.py --layout configs/warehouse_layout_all_robots.json
```

The runner builds/tests, generates inputs, runs native simulation and replay, and independently audits both. Wait for **PASS** and note the printed `results/layout-...` directory. `RUN_STATUS.json` must say `PASS`; partial `RUNNING`, `FAILED` or `TIMEOUT` results are not validated.

Substitute that actual batch directory for `YOUR_BATCH` below:

```sh
python scripts/visualize_warehouse.py --batch results/YOUR_BATCH --output warehouse-viewer.html
```

Open the newly created `warehouse-viewer.html`. Choose a new filename for subsequent exports: overwriting is deliberately rejected. The root menu points to packaged demos and does not automatically discover new runs.

Other demonstrations:

```sh
python scripts/run_warehouse_layout.py --spine-outage
python scripts/run_warehouse_layout.py --layout configs/warehouse_layout_all_robots.json --zone-outage A
python scripts/compare_warehouse_routing.py
```

Export either layout scenario as above. The comparison runner writes `comparison.html` directly in its printed `results/routing-...` folder. The outage flags are mutually exclusive; zone isolation needs a selected zone with exactly one switch.

### Change devices and traffic

Edit a copy of `configs/warehouse_layout_all_robots.json`. Each entry in `zones` declares `id`, `robots`, `switches`, and `edgeServers`; `sharedSpines` sets the backbone count. Add/remove zone entries to change zone count. The `traffic` object sets mode, destination scope, interval, payload, priority and service candidates.

```sh
python scripts/run_warehouse_layout.py --layout path/to/your-layout.json
```

Keep JSON valid (double quotes, no comments/trailing commas). Read [the layout/traffic reference](docs/warehouse-demo.md) for valid ranges and attachment rules. Devices remain stationary. Larger or faster workloads can increase runtime, congestion and losses; example performance is not a guarantee.

## 5. Results and complete validation

| Demonstration | Validated observation |
|---|---|
| Representative baseline | 120/120 delivered |
| All-robot baseline | 2,410/2,410 delivered from 482 sources |
| Dijkstra / three ECMP seeds | Each 2,410/2,410; ECMP uses both spines |
| Spine failure/recovery | 120/120 with observed rerouting |
| Zone A switch failure | 2,252/2,410; 158 no-route losses/deadline misses; recovery verified |

Windows evidence includes 25 native build tests, 21 core tests, independent audits, exact replays, and historical full native100/core132 regressions. See [routing results](docs/routing-comparison.md), [zone failure](docs/zone-outage.md), [native validation](docs/ns3-warehouse-results.md), and [current checkpoint](checkpoints/CURRENT_STATUS.md).

```sh
python scripts/verify.py
python scripts/run_ns3_warehouse.py
python scripts/run_all_experiments.py
```

The last command is the **abstract core** 132-run/three-seed study; `--quick` is only a smoke check. Native and abstract measurements are distinct models, neither radio results. Native subprocesses have timeouts; build and audit durations are additional.

## 6. Troubleshooting

| Problem | Action |
|---|---|
| Missing/blank viewer | Download the delivery asset, extract everything, and open the root menu; a source clone has no saved results. |
| Python not found | Install Python 3.12; reopen the terminal. On Windows create the environment using `py -3.12`. |
| Compiler/CMake/Ninja/Boost missing | Complete platform setup; on Windows use `setup_portable.py` with the same Python environment. |
| ns-3 prefix absent | Complete `setup_ns3.py`; `--ns3-prefix` is only for a compatible installed native runtime. |
| Windows path too long | Extract to a short folder such as `C:\Warehouse`. |
| Build killed / memory exhausted | Retry dependency setup with `--jobs 2`; retain logs. |
| Checksum mismatch | Stop and inspect the retained download/log; never disable checksum checks. |
| Native case failed/timed out | Read `RUN_STATUS.json`, `processes/*.log` and `processes/*.json`; partial traces are not a successful result. |
| Export file exists | Choose a fresh output filename. |
| Zone-outage reports losses | Expected while isolated; inspect phase reports and recovery packets. |

## 7. Scope, research history and remaining work

Powered infrastructure, peer/service communication, priority scheduling and resilience are the primary direction. EALR remains **legacy** relay research. Protected QoS and capacity/admission studies are experimental: fixed P0 admission can worsen emergency-flood results, and no tested setting eliminated all burst/flood deadline misses. See [study design](docs/warehouse-study.md), [protected-queue results](docs/protected-queue-results.md), and [capacity results](docs/capacity-results.md).

The native model is stationary wired IPv4/UDP with immediate centralized failure knowledge. It is not Ethernet switching, Wi-Fi, private 5G/NR, hardware convergence, or guaranteed deployment performance. Mac/Ubuntu execution, calibrated real warehouse workloads and additional research integration remain pending. The comprehensive illustrated project report is the next requested deliverable.

Git contains source, configs, tests and checkpoint summaries. Release ZIPs separately preserve saved viewers, raw/replay results, integrity manifests and a recovery bundle. Toolchains/build trees are intentionally not uploaded. Preserve previous results and manifests. See [reproducibility](docs/reproducibility.md) for recovery and [START_HERE](START_HERE.md) for the presentation walkthrough.
