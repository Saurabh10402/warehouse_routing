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
