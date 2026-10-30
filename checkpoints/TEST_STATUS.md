# Final test status — checkpoint_004

| Area | Status | Evidence |
|---|---|---|
| Original archive preserved | PASS | Original SHA256 in CURRENT_STATUS.md |
| Bootstrap configure/build/CTest | PASS: 16/16 | evidence/final-bootstrap-tests.txt |
| Default configure/build/CTest | PASS: 16/16 core-only | evidence/final-default-tests.txt; ns-3 explicitly not found |
| Full experiment matrix | PASS: 114 runs | evidence/final-experiment-run-status.json and batch execution.log |
| Fair comparisons | PASS | Identical per-seed offered packet tuples across algorithms |
| Raw paths, positions, counters, means/deadlines | PASS | Full runner checks on all 114 runs |
| Deterministic replay | PASS | Byte-identical selected baseline outputs |
| Charts | PASS: 72 PNGs | Final batch graphs/ |
| Optional adapter syntax | PASS: three .cpp files | Actual ns-3.46.1 headers with C++23; not linking/runtime |
| External ns-3.46.1 Release build | FAIL | evidence/ns3-build-failure.txt; Wi-Fi maybe-uninitialized diagnostic |
| Project adapter runtime / FlowMonitor / NetAnim | BLOCKED | External prerequisite did not build/install |
| 5G-LENA NR | NOT RUN | Module absent; no NR result claimed |
| macOS / Linux execution | NOT RUN | Commands documented; this host is Windows |

The preserved earlier evidence describes earlier milestones; this table supersedes their
in-progress status. Commands executed: scripts/run_all_experiments.py (build/CTest/full
matrix/replay/plots), scripts/verify.py --preset default, and the ns-3 configure/build and
syntax commands documented in docs/ns3-integration.md. The default verification was rerun
with approved tool access after the sandbox denied access to portable compiler directories.
