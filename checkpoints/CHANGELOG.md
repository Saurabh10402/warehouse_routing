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
