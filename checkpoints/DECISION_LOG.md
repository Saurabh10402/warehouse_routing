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
