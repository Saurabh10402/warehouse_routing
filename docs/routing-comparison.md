# All-robot routing comparison

Batch: `routing-20261102T152642Z-82d74709`. Native stationary wired ns-3, 500 devices in eight zones; 482 robot sources offer five packets each. All four cases use identical arrivals, 256-byte payloads and strict-priority scheduling. This is a routing-only controlled subset; the existing four-policy FIFO/strict-priority studies remain unchanged. EALR is legacy only.

| Case | Delivered | Deadline misses | Mean delay ms | P95 ms | Cross-zone mean ms | Spine 0 / spine 1 packets |
|---|---:|---:|---:|---:|---:|---:|
| Dijkstra seed 1 | 2410/2410 | 0 | 0.321698 | 0.489232 | 0.472235 | 805 / 0 |
| ECMP seed 1 | 2410/2410 | 0 | 0.317991 | 0.468640 | 0.461136 | 435 / 370 |
| ECMP seed 2 | 2410/2410 | 0 | 0.317948 | 0.468640 | 0.461009 | 390 / 415 |
| ECMP seed 3 | 2410/2410 | 0 | 0.318024 | 0.468640 | 0.461236 | 375 / 430 |

ECMP spreads the 161 cross-zone flows across both equal-cost spines, while deterministic Dijkstra tie-breaking selects one spine. Same-zone and local edge-service traffic bypass the spines. Both approaches delivered everything; the observed delay difference is small. Spine packet counts are not bandwidth-utilization measurements. Latencies include delivered packets only; all offered packets remain in deadline/delivery denominators. P95 is nearest rank.

All eight native executions were independently audited; every case's seven replay artifacts matched exactly. Arrival signatures matched across all four cases. ECMP seed 1 also reproduced all seven artifacts from the previously delivered all-robot batch. Three focused runner tests and 25 native build tests passed. No simulator implementation or queue parameters changed; the previous full 100-case native and 132-case core regressions remain historical evidence, not newly rerun suites for this milestone.

Reproduce with `python scripts/compare_warehouse_routing.py`; use `--layout` to specify another layout and `--ns3-prefix` for a custom native installation. Inputs and declared metrics are saved before execution. Source/input changes during a batch fail validation. Raw data, process receipts and provenance are retained. Each native process has a timeout; setup, build and audit durations are additional.

This is synchronized light traffic, not a saturation-capacity experiment. Seeds vary transport hashing only, not random arrivals. There is no radio, hardware convergence, deployment deadline guarantee or executed Mac/Ubuntu validation. The earlier Windows verification interruption produced no comparison batch; after host recovery, the complete rerun passed.
