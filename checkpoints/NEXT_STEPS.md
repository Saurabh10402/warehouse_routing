# Resume after checkpoint_009

1. Read CURRENT_STATUS, PROJECT_STATE, sessions/session_006.md, docs/ns3-wired-warehouse.md
   and docs/ns3-warehouse-results.md. Inspect Git status and verify checkpoint_009.
2. Add native strict-priority queue service with one explicit finite waiting buffer per
   direction. Test nonpreemption, priority and same-priority ordering, admission/overflow,
   lower-class starvation and conservation. Preserve the native FIFO controls.
3. Add per-flow ECMP keyed by explicit transport flow identity and seed. Do not substitute
   per-packet random routing or use observer tags to fabricate a wire-level routing policy.
   Test equal-cost path eligibility, stable flows, endpoint exclusion and deterministic replay.
4. Declare the native forwarding/scheduling comparison before running it. Keep payload/IP/PPP
   byte conventions and cutoff/drop/deadline accounting explicit. Do not equate native/core
   delays or describe repeated deterministic fixtures as stochastic seed evaluation.
5. Add explicit failure detection/recovery, backup services, protected queues and safety
   fan-out only with separate native tests; retain unsupported-input rejection until ready.
6. Pin compatible ns-3/5G-LENA versions and verify EPC/UPF receiver traffic before NR claims.
   Existing optional Wi-Fi/NetAnim compiler diagnostic remains preserved.
7. Run actual clean MacBook Air M4 / Ubuntu 24.04 ARM64 UTM builds when available. Current
   source-extraction evidence uses Windows with external verified dependencies, not those hosts.
8. Calibrate warehouse workload/access/deadline requirements before deployment conclusions.

Completed: scripts/run_ns3_warehouse.py (all seven cases, raw audit and replay); the same
workflow from isolated source (24 tests, 35 identical artifacts); full scripts/run_all_experiments.py
--suite warehouse --preset bootstrap --baseline results/experiments-20261031T094745Z-723bc3c8
(132 exact comparisons, replay, 55 charts). The earlier 150-suite ns-3 dependency validation
remains applicable to the unchanged installed runtime; it was not needlessly rerun.

Preserve all prior archives/manifests/raw batches. No random movement in the primary runners.
