# Warehouse communication results — 30 October 2026

## Scope and source

This study evaluates powered, stationary wired-link abstractions with same-zone peer,
cross-zone peer and edge-service traffic. It does not evaluate real Ethernet hardware,
Wi-Fi airtime, private-5G radio/UPF paths or distributed routing convergence.

Implementation commit: `1da7f3f`. Primary batch:
`results/experiments-20261030T140941Z-dd164497`.
The full matrix passed: **132 runs, raw artifact/fairness checks, deterministic replay and
55 charts**. Both bootstrap and default core-only CTest passed **18/18 tests**. The separate
legacy smoke matrix passed 45 runs, replay and 75 charts; it uses only one seed per case.
Execution status is recorded in each batch's RUN_STATUS.json and checkpoint_005.
Source hashes and exact per-run inputs are stored in the batch. Later documentation records
the results; it does not change the implementation used in those runs.

## Findings

The lightly loaded mixed-communication example delivered 180/180 packets under every policy
in seed 1, with mean delivered latency 0.307637 ms. The extra forwarding/scheduling choices
provide no improvement when there is no meaningful contention. These configured link delays
are illustrative rather than measured deployment inputs.

Below are **means across seeds 1, 2 and 3**, with sample standard deviation in parentheses.
Delivery covers all traffic; P0 deadline misses include both loss and delivered latency
at least 10 ms. They aggregate same-zone, cross-zone and service traffic; use the scope
tables/plots to inspect affected paths separately.

| Scenario | Policy | Delivery % (SD) | P0 deadline misses % (SD) |
|---|---|---:|---:|
| Uplink contention | shortest-fifo | 82.607 (0.206) | 58.333 (0.417) |
| Uplink contention | shortest-priority | 83.440 (1.085) | 24.861 (1.879) |
| Uplink contention | ecmp-fifo | 99.402 (1.036) | 5.556 (6.365) |
| Uplink contention | ecmp-priority | 99.551 (0.777) | 0.833 (1.443) |
| Destination bottleneck | shortest-fifo | 72.457 (1.126) | 60.833 (0.000) |
| Destination bottleneck | shortest-priority | 73.013 (1.512) | 26.667 (9.214) |
| Destination bottleneck | ecmp-fifo | 83.632 (0.596) | 20.833 (7.217) |
| Destination bottleneck | ecmp-priority | 84.038 (0.449) | 13.750 (0.417) |

Using both spine paths reduces the deliberately configured uplink bottleneck. Priority
service improves P0 delivery within deadline in this workload. Neither guarantees success:
ECMP hash imbalance remains visible between seeds, and the destination bottleneck is shared
by both spine paths. The combined policy still misses 13.75% of P0 deadlines on average there.
For the affected cross-zone P0 traffic specifically, misses average **41.25%**; same-zone
and edge-service P0 traffic have zero misses with that policy. The aggregate therefore
understates the impact on the affected communication scope.
No costs or traffic parameters were retuned after seeing these results to favor a policy.

The paired comparisons matter: shortest-fifo versus shortest-priority isolates scheduling,
while shortest-fifo versus ecmp-fifo isolates path selection. The full four-policy matrix
also exposes interactions. It does not establish superiority for untested traffic patterns.

When both spines fail at 300 ms in seed 1, same-zone flows deliver all 60/60 packets across
the run; cross-zone and edge-service flows each deliver only 24/60, all before the outage.
The combined 108/180 delivery count hides this difference if reported alone. Single-spine
failure retains an alternate path. A failed zone leaf interrupts its attached endpoints
despite two working spines. Edge-service failover selects the reachable secondary edge,
but does not simulate replication of application state or transaction continuity.

## Validation and artifacts

The 18-test CTest suite includes the existing routing/safety/metrics regressions and new
directed-graph ECMP, stable-flow, peer-path, spine-failure and scope-denominator checks.
Full execution evidence, replay and plotting status are recorded in the current checkpoint.
The preserved legacy suite is additionally exercised as a single-seed smoke matrix, not
presented as a new full multi-seed EALR evaluation.

`scope_priority_comparison.csv` is the primary diagnostic table. `packets.csv` inside each
run retains actual paths, packet outcomes and times. `priority_comparison.csv` gives class
totals; `aggregate.csv` gives seed means/SD; `graphs/*scope-priority-deadlines.png` separates
scope and class. The runner checks identical offered traffic across policies, per-class
counts, raw deadline flags, endpoint non-transit, fixed positions and deterministic replay.

## Next engineering question

The current strict-priority queues have a shared tail-drop buffer. Emergency packets can be
discarded when background traffic fills it, and low-priority traffic can starve. The next
controlled extension is reserved emergency buffer space plus bounded high-priority admission
and a documented service allocation for lower classes. Compare it against these unchanged
baselines under the same offered traffic, including overload and destination bottlenecks.
Do not infer that reservation can overcome an endpoint link's finite physical capacity.

Before deployment conclusions, supply the actual warehouse layout, rates/bursts, class
assignments, access technologies and acceptable failure domains. Three seeds give limited
variability evidence. Real ns-3/NR integration remains blocked/unverified as documented in
ns3-integration.md; this study has no dependency on that external build.
