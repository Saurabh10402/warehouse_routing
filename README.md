# WIPRO warehouse networking research prototype

A configurable **stationary warehouse network** with priority-aware packet forwarding,
same-zone/cross-zone communication, local safety events, failures, and reproducible metrics.
The reference leaf-spine design is supported; the physical warehouse layout is not finalized.
Ten zones and 500 endpoints are example inputs, not algorithm requirements.

The primary executable is `ealr-fabric`, an independently tested C++17 discrete-event
**abstract-link simulator**. It models transmission, propagation, finite queues and packet
paths. Wi-Fi/private-5G labels do not implement radio scheduling or a full 5G core.
Legacy ns-3/5G-LENA scaffolding is separate and has its own verification status.

The primary research focus is now **priority scheduling and resilience in powered
infrastructure**. The default experiment matrix compares shortest-path and ECMP forwarding
with FIFO and strict-priority queues. EALR remains preserved as earlier relay research;
it is not the primary warehouse routing proposal. See [the study design](docs/warehouse-study.md).

The next comparison adds [protected emergency queues](docs/protected-queues.md), bounded
P0 admissions and byte-weighted lower-class service. The [traffic specification](docs/traffic-requirements.md)
declares inputs and acceptance checks; these are experimental assumptions, not measured
warehouse requirements. Run it with `python scripts/run_all_experiments.py --suite protected-qos`
(144 runs). The original default warehouse matrix remains unchanged.
See [protected queue results](docs/protected-queue-results.md) for both improvements and
admission losses; the fixed 20% P0 budget is not a production recommendation.

The completed [capacity/admission study](docs/capacity-study.md) compares eight settings at
four destination attachment capacities with new seeds (288 runs). Its [results and operating
limits](docs/capacity-results.md) include reservation without policing, burst/flood failures,
and the cost to lower-priority traffic. Run `python scripts/run_all_experiments.py --suite capacity-admission`.

## Start or resume

For a fresh extraction, start with the [first-run guide](docs/first-run.md). It gives the
Mac M4/Ubuntu 24.04 setup commands, a small checked example, expected outputs and log paths.
`python scripts/first_run.py` validates the independent core; `python scripts/run_ns3_smoke.py`
separately requires a real installed ns-3 wired runtime. Neither implies NR validation.
See [runtime validation](docs/runtime-validation.md) for the executed Windows tests and
remaining schema-v2/NR and Mac/Ubuntu boundaries.
The initial [native wired warehouse backend](docs/ns3-wired-warehouse.md) now accepts a strict
schema-v2 FIFO/Dijkstra subset. Run `python scripts/run_ns3_warehouse.py` for its deterministic
packet/queue/path acceptance cases; it has separate outputs and does not replace the core study.

Read `checkpoints/CURRENT_STATUS.md`, `checkpoints/PROJECT_STATE.json`, `checkpoints/NEXT_STEPS.md`, and the latest
session summary. Verify the recorded manifest with:

```sh
python scripts/checkpoint.py CHECKPOINT_ID --verify
```

## One-command validation and experiments

With Python 3.10+, CMake 3.22+, Ninja, a C++17 compiler and Boost.JSON available:

```sh
python scripts/run_all_experiments.py
```

This configures a host-specific build, runs CTest, runs four forwarding/scheduling policies on the same
inputs/seeds, checks packet paths and stationary positions, exports raw/per-class metrics,
replays a run, and produces comparison plots when matplotlib is installed. It exits nonzero
on required-step failure and prints a unique results directory. `--quick` uses one seed and
omits the two large layouts; it is not the full research matrix. The full default study
has 132 runs (11 scenarios, three seeds, four policies). Use `--suite legacy-ealr` to
reproduce the preserved 114-run EALR comparison instead.

Windows local setup (optional, no global installation or PATH changes):

```powershell
python scripts/setup_portable.py --plots
python scripts/run_all_experiments.py
```

The current recovered workspace already has the portable toolchain under `.tools/`.
See [reproducibility](docs/reproducibility.md) for the exact local Python command, platform
setup, direct CMake commands, dependency hashes and recovery packaging.

## Primary configurations

| File | Purpose |
|---|---|
| `configs/warehouse_communications.json` | Primary wired peer/service traffic, unequal zones, dual spines and edges |
| `configs/warehouse_protected.json` | Destination bottleneck with protectedPriority and explicit queue parameters |
| `configs/warehouse_small.json` | Five-node deterministic graph with four traffic classes |
| `configs/warehouse_uneven.json` | Four zones: 10, 35, 75, 20 endpoints |
| `configs/warehouse_reference.json` | 500 endpoints in 10 zones, dual spines and edges |

Each schema-v2 file explicitly defines zones, nodes, endpoint access, links, capacities,
delays, queue sizes, ordered service destinations, workload priorities and optional failures.
Graph mode supports arbitrary test graphs. Leaf-spine mode validates role/zone connectivity.
The old `warehouse_leaf_spine.json` is a preserved legacy planning schema, not the executable
primary configuration. Legacy robot/mobility experiments remain outside the primary scope.

```sh
python scripts/verify.py
./build/verified-bootstrap/ealr-fabric --config configs/warehouse_communications.json --output results/my-new-run
```

On Windows use `build/verified-bootstrap/ealr-fabric.exe` and ensure the compiler DLL directory
is on PATH; the Python runner manages this automatically. Existing result directories are rejected.

## What is implemented

- Validated explicit topology with arbitrary zone membership and unequal endpoint counts.
- Reused routing algorithms with explicit fabric link costs; legacy distance APIs preserved.
- Stable per-flow ECMP using equal minimum-cost next hops, with unavailable paths excluded.
- EALR normalized latency/queue/energy cost and a snapshot latency guard.
- Per-direction finite queues: strict priority 0..3, FIFO within each class, nonpreemptive frames.
- Optional reserved P0 slots, token-bucket P0 admission and byte-weighted P1/P2/P3 service.
- Actual packet forwarding over selected links, with serialization and propagation delay.
- Per-hop route selection, node/link outages, ordered backup edges and component recovery.
- Battery accounting only on constrained endpoints/relays; no infrastructure battery penalty.
- Local configurable temperature safety, latched stops, manual reset and priority-0 fan-out.
- Raw packet traces, measured latency distributions, delivery/deadline rates and application throughput.
- Source/configuration/executable provenance, seed comparisons, ablations and recovery checkpoints.

The [model specification](docs/fabric-model.md) defines defaults, algorithms, units, timing,
loss semantics, safety behavior and fidelity limits. [Current warehouse results](docs/warehouse-results.md)
and [earlier recovery validation](docs/validation-report.md) separate executed evidence from
limitations. Do not interpret configured latency targets as guarantees.

## Layout and build boundaries

`src/config` validates JSON; `src/routing` selects paths without parsing JSON;
`src/simulation/fabric_simulator.cpp` runs events/forwarding; `src/metrics/fabric_metrics.cpp`
aggregates packet observations; `scripts/warehouse_study.py` defines the primary comparisons;
other `scripts` build, run and plot experiments; `tests` contains
legacy and primary regression suites. Public contracts are under `include/ealr/`.

The core remains C++17 and never requires ns-3. `EALR_BOOST_SOURCE` optionally compiles the
existing Boost.JSON implementation from an official extracted Boost tree; no replacement parser.
The optional ns-3 target has its own C++ standard setting. See [ns-3 integration](docs/ns3-integration.md).

## Explicit limitations

No calibrated radio PHY/MAC, collision/retransmission model, full 5G core, distributed route
convergence, admission control, hard deadline assurance, disjoint packet replication, or
certified emergency-stop behavior. Safety readings start at a local edge; sensor transport
and actuator response are outside the model. Energy uses configured percentage points, not joules.
Strict priority can starve lower classes, and a full shared queue can drop safety traffic.
The protected policy counts policing/reservation losses explicitly; it cannot promise deadlines
or admission for every packet. Lower-class fairness applies to packets admitted to its queues.

The retained [legacy project documentation](docs/legacy-project.md) describes the original
robot experiment and its incomplete ns-3 orchestration. It is historical context, not the
current primary workflow or evidence of verified 5G behavior.
