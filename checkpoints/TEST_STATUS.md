# Test status — checkpoint_005

| Check | Result | Evidence |
|---|---|---|
| Bootstrap core build/CTest | PASS 18/18 | evidence/checkpoint005-bootstrap-tests.txt |
| Default core-only build/CTest | PASS 18/18 | evidence/checkpoint005-default-tests.txt |
| Primary full study | PASS 132 runs, 3 seeds | evidence/checkpoint005-primary-status.json; batch provenance/execution log |
| Legacy smoke | PASS 45 runs, 1 seed per case | evidence/checkpoint005-legacy-status.json |
| Primary/legacy replay | PASS | Byte-identical selected packet traces |
| Primary/legacy charts | PASS 55 / 75 PNGs | Batch graphs; primary scope/deadline chart visually inspected |
| ECMP and communication behavior | PASS | Directed equal-cost paths, stable flows, failed/expensive path exclusion, peer paths, spine/service failover |
| Raw artifacts | PASS | Offered traffic equality; path validity/no endpoint transit; stationary positions; counts/class/deadline flags |
| Scope metrics | PASS | Lost packets keep intended scope and count as misses; delivered-only means |
| ns-3/NR runtime | BLOCKED / NOT RUN | Same external ns-3 Wi-Fi diagnostic; no new attempt |
| macOS/Linux execution | NOT RUN | This host is Windows |

Executed: `scripts/verify.py`; `scripts/run_all_experiments.py`;
`scripts/run_all_experiments.py --suite legacy-ealr --quick --preset default`.
A standalone test invocation inside the restricted sandbox was denied access to its temp
file. Approved build/CTest executions of that same test passed in both presets.
No warnings or tests were disabled. The current study does not establish real radio QoS,
physical safety actuation, routing convergence time or deployment deadlines.
