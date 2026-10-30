# Validation report — 30 October 2026 (India time)

## Verified primary prototype

The repository now has a complete executable **stationary abstract-link** experiment path:
JSON validation, explicit graph construction, application arrivals, routing, actual simulated
packet forwarding, finite queues, class priorities, failures/recovery, constrained-device
energy, local safety handling, raw measurements, provenance and comparison plots.

The recovered baseline compiled and passed 11 tests. The expanded core suite passed 16/16,
including the real CLI and exported artifacts. `checkpoints/evidence/` preserves test logs.
Primary language standard remains C++17; dependencies used: GCC 14.2.0 and Boost 1.85.0.

Final full batch: `results/experiments-20261030T104837Z-6863eef6`:
114 runs, all artifact checks passed, deterministic replay identical, 72 comparison charts.
The first 111-run batch is also retained under `experiments-20261029T210304Z-82f715a2`.
The final batch ran code from commit `b8214a51edf075295aa0b00bf0b20cda2354d9df`.
It also verifies identical offered traffic across algorithms and recalculates mean delay
and deadline misses from raw packet traces. Subsequent changes document these results and
the external ns-3 blocker; the tested implementation is unchanged.

## Executed checks

| Area | Evidence |
|---|---|
| Configuration | Reference 500 endpoints; uneven 10/35/75/20 zones; invalid IDs, memberships, access, links, weights, priorities, safety and workloads rejected |
| Routing | Existing Dijkstra/Greedy/EALR tests; explicit link costs; invalid costs; unreachable and unavailable endpoints; source=destination; battery and queue-sensitive actual paths |
| Stationary nodes | Runtime coordinate invariant and exported initial/final positions checked for every run |
| Timing | Two-hop 500-byte/10-Mbps/0.2-ms links deliver at exactly 1.2 ms within numerical tolerance |
| Priority | Safety overtakes waiting background; cannot preempt active frame; FIFO ablation and finite buffer loss verified |
| Failures | Node/link failure, secondary edge selection, alternate traffic, recovery, complete outage, in-flight epoch loss and redundant availability events |
| Energy | Critical relay exclusion, battery-aware alternate path, zero-energy-weight ablation and excessive-detour guard |
| Safety | 70/80C boundaries, invalid/nonfinite thresholds, latched stop, manual reset, cloud-independent local response, packet fan-out, unavailable controller |
| Metrics | Zero traffic, strict deadline boundary, lost-packet deadlines, mean/median, p95/p99 from packet samples, throughput and cutoff loss |
| CLI/artifacts | Real executable, output conservation, actual graph paths, stationary CSV, replay, invalid JSON and non-overwrite behavior |
| Experiments | Three algorithms; three seeds for random-phase cases; deterministic cases; scaling, priorities, energy/congestion ablations and failures |

## Observed results and limits

These are illustrative, uncalibrated configurations, not warehouse deployment guarantees.
Values below are seed 1 unless specified. Full per-seed and per-class data are retained.

| Scenario | Dijkstra | Greedy | EALR |
|---|---|---|---|
| Small baseline | 36/36 delivered; 1.2 ms mean | same | same |
| Severe congestion | 36/754 delivered; 93.261 ms mean | same | 72/754; 92.275 ms mean |
| Low-battery relay | Final battery 9.5%; relay excluded | same | Final battery 12%; relay avoided |
| Energy/latency tradeoff | 1.2 ms; final battery 34.82% | same | 1.4 ms; final battery 35% |
| 500 endpoints | 2500/2500; ~0.93646 ms mean | same | same |
| Single spine failure | 110/110 delivered | same | same |
| Complete core outage | 42/110 delivered | same | same |
| Safety with cloud off | 120/120 delivered | 113/120 | 120/120 |

Every class misses its deadlines under the severe congestion case for all algorithms.
EALR uses the alternate path and doubles delivered count, but this is still only ~9.55% PDR.
Strict priority does not create capacity and can starve background traffic. FIFO changes
which packets survive; it is not uniformly better or worse across aggregate metrics.

The tradeoff case shows a **16.7% latency increase** from EALR in exchange for avoiding
0.18 percentage points of modeled depletion on one relay. The default weights were not
changed for this case. This is an explicit tradeoff demonstration, not evidence that
battery preservation always justifies extra latency.

Greedy loses seven safety fan-out copies because geographic choices can lead into endpoints
that cannot relay. This is its documented non-backtracking behavior on this switch graph;
the failures were retained. Dijkstra and EALR reach those destinations.

The lightly loaded 500-endpoint reference meets configured deadlines in these runs. Its
abstract capacities/delays and modest workload make that result expected; it does not
establish radio QoS or performance in an actual warehouse. Three random phases per case
provide limited variability evidence, not a confidence bound or general superiority claim.
Compare delivery counts alongside delivered-only mean latency to avoid survivorship bias.

## ns-3, 5G-LENA and external verification

The original runner only installs nodes/mobility/optional NR devices and animation. It
does not wire EALR into ns-3 application forwarding. The primary core results above do not
come from that runner or from FlowMonitor.

This session downloaded official ns-3.46.1 and attempted a portable native build. The three
existing adapter source files passed syntax checks against those actual headers. The external
Release build failed in its Wi-Fi dependency with GCC `-Werror=maybe-uninitialized` involving
`std::optional<NonInheritance>` in `wifi-mgt-header.h`. The diagnostic was retained and not
suppressed. Full ns-3 link/runtime verification is blocked by that failure; syntax success
is not a runtime result. The downloaded release requires C++23, selected only for the
optional adapter target. 5G-LENA NR is not installed or verified in this session.

The primary fabric schema has not yet been mapped into ns-3 NetDevices/QoS queues. That is
an isolated future integration task, not a hidden dependency of the working core model.
Mac/Linux commands are documented but were not executed on those operating systems.

## Deliberate abstractions and remaining research work

No radio PHY/MAC contention, retransmission, calibrated RF propagation, distributed routing
convergence, hard deadline enforcement, disjoint packet replication or physical safety
actuation is claimed. Link failures use ideal immediate knowledge. Safety readings start
at a chosen local edge; local-controller failure is reported rather than automatically
transferred. Energy is configured percentage points, not physical joules. See the model
specification for denominators, drain semantics, censoring and recovery definitions.

Next research inputs are the actual warehouse layout, measured access/link delays, realistic
traffic distributions, and battery characteristics. These should enter JSON configurations,
followed by additional seeds and a declared evaluation protocol, not algorithm hardcoding.
