# Test status — checkpoint_008

| Check | Result | Evidence under checkpoints/evidence |
|---|---|---|
| Core bootstrap | 20/20 PASS | checkpoint008-bootstrap-tests.txt |
| Final code in isolated source extraction | 20/20 PASS, 180/180 packets | checkpoint008-extracted-tests.txt; clean-source-first-run-final.txt |
| Full warehouse regression | 132 exact inputs/traces, replay and 55 charts PASS | checkpoint008-warehouse-status.json; preserved batch |
| ns-3 wired dependency | Release build/install and 150/150 UNIT QUICK PASS | checkpoint008-ns3-build/install/upstream-tests.txt; setup.json |
| Project linked to ns-3 | 22/22 CTest PASS | checkpoint008-ns3-project-tests.txt |
| Actual two-hop UDP / replay | 3/3, 384 IP bytes, 4.416 ms, stationary; exact replay | checkpoint008-ns3-smoke.json/status.json; preserved batch |
| Required dependency and explicit-prefix isolation | Correct failure PASS | ns3-required-missing.txt; ns3-explicit-prefix-isolation.txt |
| Existing output preservation | Correct refusal, bytes unchanged PASS | ns3-existing-output-guard.txt |
| ns-3 schema-v2/QoS, NR, Wi-Fi/NetAnim | NOT VALIDATED | Preserved external failure and integration contract |
| Mac / Ubuntu execution | NOT RUN | Windows host only; prepared first-run guide |

The 132-run regression uses a2bf99f; later changes only fix installer/test orchestration and
documentation. Final extracted-source and linked-project checks cover 47088f1 code. Initial
syntax/include, upstream-invocation, report-parser and overnight timeout diagnostics are
preserved. Final passes used no warning suppression, skipped required suite or increased
timeout. The 150 upstream suites are UNIT QUICK in Release, not all system/extensive tests.
Generated results retain their original hashes and provenance. Stress deadline misses remain
measured failures; validation success is not a performance guarantee.
