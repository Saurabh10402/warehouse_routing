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
