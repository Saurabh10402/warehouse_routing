# Decisions

1. Preserve legacy robot algorithms and examples; primary workflow will be a stationary warehouse model.
2. Keep Boost.JSON. Acquire a portable compiler instead of replacing the parser.
3. Build an independently tested C++ discrete-event abstraction, because ns-3 and NR are absent.
   Explicit link serialization, propagation, queueing, packet paths and failures are required.
   Wi-Fi/5G access labels will describe abstract links, not radio PHY/MAC behavior.
4. Version the explicit graph configuration alongside legacy schemas. Use the existing Boost.JSON
   dependency and reuse the temperature safety controller; do not silently reinterpret legacy plans.

5. Fabric routing reuses the legacy algorithms through a validated link-cost callback. Fixed infrastructure has no energy penalty. Dimensionless latency/queue terms use an explicit scale; an estimated-latency guard bounds energy detours without claiming future deadline guarantees.
6. Full-duplex per-link strict-priority queues are nonpreemptive, finite and tail-drop. FIFO is an explicit ablation. Immediate centralized topology knowledge and per-hop recomputation are documented assumptions, not protocol convergence measurements.
7. Safety events begin at an explicitly selected local edge. Broadcast is per-endpoint packet fan-out, not disjoint replication. Cloud state cannot gate local processing.
8. Keep graph-level simulation and optional ns-3 adapters distinct. The downloaded ns-3.46.1 source requires C++23; only that optional target selects the higher standard. Core remains C++17.
9. Preserve negative research findings: Greedy can dead-end on the switch fabric; overload can miss every deadline; EALR may spend latency to preserve a constrained relay. No parameters are retuned to hide these effects.

10. Preserve the external ns-3.46.1 maybe-uninitialized failure without suppressing it or
    asserting it is harmless. Complete the independent core handoff; leave schema-v2
    ns-3 forwarding and NR as explicit integration work.

11. User approved powered-fabric communication/QoS/resilience as the primary direction after
    questioning energy-aware routing. Preserve EALR as legacy, not the primary proposal.
12. Isolate wired fabric forwarding/scheduling first; real Wi-Fi/NR access needs separate
    models and explicit 5G UPF paths. No direct base-station switching is assumed.
13. Compare shortest/ECMP and FIFO/priority factorially with identical offered traffic.
    Preserve losses and scope-level deadline misses, including ECMP hash imbalance.
14. Do not claim queue reservation/admission exists: shared-buffer P0 drops are a measured
    limitation and motivate the next controlled extension.

15. Keep the 132-run baseline unchanged; evaluate protected queues in a separate 144-run suite.
16. Count every rate-policed/reservation-rejected packet as loss and deadline miss. Per-hop
    queue admission/service statistics are distinct from end-to-end packet outcomes.
17. Fixed 20% P0 policing is an always-on experimental ceiling, not flow negotiation or a
    production recommendation. Adverse emergency-overload results require capacity/admission
    sensitivity and per-port traffic envelopes before choosing a default.
18. Weighted fairness concerns admitted lower-class packets. It does not promise admission
    for every flow, prevent all overload loss or certify emergency-stop behavior.

19. Isolate policing by disabling only token admission, retaining reservation and weighted
    lower service. Explicitly demonstrate possible lower-class starvation without policing.
20. Declare capacity/rate/burst variants before new seeds 4/5/6; preserve every outcome.
    Source-time endpoint envelopes are not downstream arrival or deadline guarantees.
21. Zero mixed-load P0 misses at a tested capacity do not establish burst/flood safety or
    acceptable lower-class service. Keep defaults unchanged and preserve both regressions.

22. Make NetAnim optional for the wired packet runtime. Its Wi-Fi dependency failure remains
    unresolved and recorded; do not claim excluding it repairs or validates radio modules.
23. Explicit ns-3 verification must fail on missing/wrong prefixes. A core-only success cannot
    fulfill that request. Verify actual packets and analytical delay/loss independently.
24. Pin and verify external source; retain warnings-as-errors and all discovered upstream
    UNIT QUICK suites. Run one suite per process and match full completion reports, not noisy
    subcase PASS diagnostics. Keep failed harness attempts and successful rerun evidence.
25. Recovery sources must run without Git or parent-repository provenance. Windows extraction
    checks do not replace actual Mac/Ubuntu validation. The whole product remains unfinished.

26. Implement schema-v2 ns-3 as a separate strict wired backend. Use real UDP/IP forwarding,
    constant positions and observed physical receive paths; never relabel core traces.
27. Model wired fabric nodes as IP routers with stable /32 node addresses and /30 links.
    Install size-aware Dijkstra tables using actual payload+UDP/IPv4/PPP wire size; this is
    central static forwarding, not Ethernet switching or routing-protocol convergence.
28. Use one finite native FIFO per direction without a default hidden QueueDisc. Retain raw
    attempt/enqueue/tx/rx/drop traces and verify conservation/order independently.
29. All offered packets remain in deadline denominators. Separate pre-IP no-route, observed
    queue loss and cutoff censoring. Name FlowMonitor reconciliation unreceived-at-cutoff.
30. Reject unsupported native features before simulation/output; preserve the full core study.
    Seven deterministic native acceptance cases are not a multiple-seed stochastic study.

31. Native scheduling reads actual DSCP; shared tail-drop, nonpreemption and FIFO within class.
32. Native ECMP uses actual explicit IP/UDP transport identity and seed, never observer tags.
33. Three native seeds are hash sensitivity only. Preserve losses and lower-class tradeoffs.
34. Successful retries do not establish the cause or repair of intermittent process-exit delays.

35. Supervise each native case with a configurable positive finite timeout (default 60 s),
    direct-file logs and separate five-second cleanup bound; no hidden automatic retry.
36. Persist provenance before execution. Only completed audits/replays can set batch PASS;
    process EXITED is insufficient. Partial outputs and TIMEOUT/FAILED/RUNNING remain distinct.
37. Preserve historical core/fresh-build evidence without relabeling it as newly executed when
    only Python supervision changes; verify unchanged simulator/executable hashes explicitly.
