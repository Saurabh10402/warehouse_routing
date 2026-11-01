# Changelog

## checkpoint_001
- Preserved and extracted original archive, inspected source and stale caches.
- Added source-manifest recovery tool. No substantive source changes yet.
- Attempted `cmake --preset bootstrap`: command not found.
- WSL unavailable; no C++ compiler found. Portable dependency installation started.

## checkpoint_002
Added schema-v2 explicit graph and 3 examples; stationary discrete-event forwarding; reusable link-cost routing; priority/FIFO queues; failures/recovery; local safety; measured metrics. Fixed type collision and nonfinite safety thresholds. Commands: scripts/verify.py (15/15 PASS). Experiment runner authored, execution next.

## checkpoint_003
16/16 core/default tests PASS. Full 114-run matrix, replay, exports and charts PASS. Added accurate model/reproducibility/report documentation, recovery tools and resume instructions. Fixed duplicated-availability event packet loss, access-type consistency and legacy count overflow; expanded guard/congestion tests. Removed GCC false dangling-reference diagnostics with string_view error paths, without disabling warnings. Optional ns-3.46.1 configured and compiling; adapter syntax PASS. NR unverified.

## checkpoint_004
Final 114-run matrix from b8214a5 passed raw-metric/fairness checks, deterministic replay
and plotting (72 charts). Both core presets pass 16/16 tests. Preserved final evidence,
updated validation report and recovery state. External ns-3.46.1 Wi-Fi build failed with
GCC maybe-uninitialized; diagnostic retained, runtime and NR unverified. Implementation
unchanged since the tested commit.

## checkpoint_005
Shifted the primary study to powered peer/service communication. Added per-flow ECMP,
FIFO/priority factorial comparison, scope/class metrics and plots. Preserved EALR under
--suite legacy-ealr. 18/18 tests both presets; 132 primary runs and 45 legacy smoke runs
with artifact checks/replay/charts PASS. Reported unresolved destination/queue bottlenecks.

## checkpoint_006
Declared traffic/acceptance specification; optional protectedPriority with reservation,
per-direction P0 policing and weighted lower-class service; queue accounting/diagnostics.
19/19 tests both presets, 144 protected runs, 132 byte-identical baseline regressions and
replays/charts PASS. Preserved both improved and adverse P0 results; defaults unchanged.

## checkpoint_007
Added explicit unpoliced reservation control and a preregistered 288-run capacity/admission
study with new seeds. Both presets 19/19 PASS; 144 protected and 132 warehouse exact regressions
PASS. Independent 96-row raw table audit, replay and 60/40/55 charts PASS. Retained adverse
burst/flood and lower-class results; no default retuned. ns-3/NR remains unfinished.

## checkpoint_008
Pinned local wired ns-3 installer, strict dependency/prefix checks, optional NetAnim separation,
actual UDP delay/loss fixtures, archive-safe first-run/provenance, and Mac/Ubuntu setup guide.
Wired build/install + 150 upstream suites; 22 linked project tests; final extracted 20 tests
and 180/180 delivery; 132 exact warehouse regressions/replay/55 charts PASS. Preserved failures
and fixed invocation/report parsing without suppressing tests/warnings. Full schema-v2 ns-3,
NR and target-host validation remain pending.

## checkpoint_009
Implemented an initial stationary schema-v2 native wired IPv4/UDP FIFO/Dijkstra backend,
strict feature/numerical bounds, actual packet/queue/position traces and independent raw
artifact auditing. Main and fresh source: 24 tests and seven deterministic cases/replay PASS;
35 measurement artifacts byte-identical. Core 20 tests and 132 exact regression runs/replay/55
charts PASS. Preserved interrupted timeout diagnostics and adverse loss/deadline outcomes.
Native strict priority/ECMP/dynamic failures/NR and actual Mac/Ubuntu tests remain pending.

## checkpoint_010
Native DSCP shared priority queues, explicit transport ECMP, raw headers, independent audits,
12 controls and declared 36-run matrix. Main/fresh 24 tests and 48 cases/replays PASS; 240
identical artifacts. Core 20 tests/132 exact regressions/55 charts PASS. Retained adverse P0/P3
tradeoffs and an interrupted Windows exit delay; bounded supervision is the next task.

## checkpoint_011
Bound native cases/replays, retain direct-file logs and atomic receipts/status, save provenance
before experiments, reject premature PASS. Real process-failure tests pass; native 25/core 21
tests and 48 native cases/replays PASS. All 240 measurements and executable/library hashes
match checkpoint_010. No simulator/model changes; prior exit cause remains unknown.


## checkpoint_012 completed validation
# Native wired resilience results

Implementation: 1c4944d and 93516df. Final batch: `ns3-warehouse-20261101T155343Z-9c1d253c`. All 100 cases passed independent artifact audits and deterministic replay (200 completed processes). Native verification passed 25/25 tests. The original 48 static cases retain all 240 original measurement files byte-for-byte against checkpoint 011.

The suite adds 13 controlled resilience fixtures across Dijkstra/ECMP and FIFO/strict-priority: alternate links and nodes, preferred-service failback, all services unavailable, source outage, initial unavailability, queued recovery, receive-time boundaries, in-flight route loss, same-zone survival, leaf outage, simultaneous events, and repeated route changes exhausting the hop limit. The static matrix uses three ECMP hash seeds; resilience controls are deterministic, not stochastic reliability trials.

The 132-run core regression `experiments-20261101T154333Z-7a7bd456` passed baseline comparison, replay and plots; core verification passed 21/21 tests. Core abstraction results are separate from native wired measurements.

Availability is checked when a frame reaches the receiving device. Queues continue serializing during outages. Route tables rebuild with zero simulated detection/reconfiguration delay. Newly generated packets choose the first reachable configured service; in-flight packets retain their target. No retransmission or application-state replication is modeled. The loop guard records native route-error reason 5 at TTL 1, avoiding unsupported ICMP traffic.

Clean-source batch `ns3-warehouse-20261101T201540Z-3d3ee628` passed 25/25 tests and all 100 cases/replays. All 700 measurement files and both comparison tables match the main batch byte-for-byte. Nonzero detection timing, native protected queues/safety, NR and actual Mac M4/Ubuntu ARM64 execution remain pending. These controlled results do not guarantee deployment deadlines or hardware failure behavior.

Next exact action: declare separate physical and detected availability with nonzero detection/reconfiguration timing, then add controlled native tests before broader evaluation.


## checkpoint_013: delayed-control contract
Declared three-state delayed availability semantics and 32 deterministic controls in docs/ns3-detection-contract.md. No runtime change or new measurement. Captured detected snapshots prevent newer knowledge leaking into earlier installations; physical receive gates remain immediate. Implementation and validation are the next milestone.
