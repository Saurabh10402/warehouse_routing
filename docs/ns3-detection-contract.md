# Delayed availability knowledge: implementation contract

Status: declared design, not implemented or measured. Checkpoint 012 remains the tested native implementation. This contract defines the next native milestone; no timing setting is calibrated to warehouse equipment.

## Configuration and scope

Proposed optional schema-v2 root object `nativeControl` contains `detectionMs` and `installationMs`, both finite nonnegative milliseconds, default zero. Unknown fields are rejected. Nonzero values must be explicitly rejected by the abstract core runner; it must never silently claim to model these delays. Native capability validation retains existing bounds and rejects times whose quantized sum exceeds supported simulator time. Zero settings must preserve checkpoint 012 measurement files.

These are deterministic centralized observation and atomic table-installation delays, not BFD, OSPF, controller transport, distributed convergence or application health checks. Dijkstra/ECMP and FIFO/strict priority remain the primary policies. Nodes stay stationary; endpoint transit remains prohibited.

## Three states and transition rules

Maintain independent physical, detected and installed availability snapshots, initialized from configured node/link availability. A physical event at t changes only physical state. Its observation occurs at t + detectionMs. An installation occurs at t + detectionMs + installationMs. Round each configured delay to native nanoseconds before adding integer ticks; record the effective timestamps.

Each observation applies that event's value to detected state. Capture the complete detected snapshot at observation time for its installation. At installation, atomically rebuild all native tables from the captured snapshot. Never rebuild from current physical state: that would leak knowledge of failures that have not yet been detected. Never read newer detected state at installation: that would erase the declared installation delay for later observations.

Use ordered notification delivery, without debouncing, cancellation or event coalescing. Fixed global delays preserve event ordering, including rapid down/up flaps. An old down notification can legitimately arrive after physical recovery; the subsequent up notification restores knowledge later. This is a declared model behavior, not a preferred controller design. A repeated no-op event still gets observation and installation records.

Pre-schedule the complete control timeline before traffic. At a common tick, process physical changes first, observations second, installations third, then packet callbacks. Preserve original configuration order within each stage. Zero delay must reproduce the final installed state before any packet at that tick. Events beyond simulation cutoff must not be exported as executed. Do not infer equal-time ordering from callbacks dynamically scheduled after traffic has already been queued.

## Packet behavior

Receive-time link/incident-node gates use physical state exclusively. Queue serialization continues across outages, as in checkpoint 012; there is no flushing, migration or retransmission. A frame crossing a complete outage may arrive after recovery. Queue overflow, physical availability loss, IPv4 route loss and cutoff remain distinct.

Application offers continue on schedule. A physically unavailable source produces source_unavailable before IP transmission, independent of controller knowledge. A live source chooses its first reachable service using installed tables, even if the service is physically down but not yet detected. Such a packet is sent toward that service and can be lost; do not magically select backup from physical state. After installation removes the primary, new packets choose the backup. In-flight packets retain the selected IP/five-tuple. Recovery does not trigger failback until installed knowledge permits it.

A recovered source may still have no installed route until the controller catches up. Intermediate forwarding uses installed tables at forwarding time; receive admission uses physical state at receive time. Retain the TTL guard and the full offered-traffic denominator for every loss, including stale-route losses.

## Evidence and independent audit

Preserve availability.csv as the physical-event trace. Add control_updates.csv containing stage, event index, target type, target ID, value, effective time and snapshot revision. Observation and installation records must be emitted by executed callbacks. Export configured/effective delays in provenance; do not relabel old immediate-knowledge summaries as delayed measurements.

The independent Python audit must reconstruct all three timelines from original inputs, not trust exported revision labels. Verify every physical gate against physical state and every next-hop/service choice against installed state. Validate control trace ordering, completeness through cutoff, and absence of future callbacks. Preserve native header, shared-buffer, deadline, IP counter, TTL and replay checks. Forged update timestamps, revisions and destinations must be rejected.

## Predeclared acceptance matrix

Use 8 deterministic new fixtures, each across Dijkstra/ECMP and FIFO/strictPriority: 32 new cases plus the existing 100, each with exact replay. Keep existing hash seeds unchanged; these are acceptance controls, not stochastic reliability estimates.

| Fixture | Required observation |
|---|---|
| delayed alternate link | Physical failure at 5 ms, detection 3 ms and installation 2 ms; stale route before 10 ms, alternate after 10 ms; recovery at 25 ms installs at 30 ms. |
| delayed relay node | Same timing with an incident-node receive gate; a node failure cannot be reduced to only one adjacent link. |
| service failover/failback | New packets keep selecting primary before its down installation, select backup afterward, and return only after up installation. |
| short flap | Down at 5 ms, up at 6 ms, detection 3 ms, installation 2 ms; installed down at 10 ms and up at 11 ms despite physical recovery at 6 ms. |
| overlapping targets | Two different failures separated by 1 ms; first installation must not include the second failure's newer detected snapshot. |
| source recovery lag | Offers while physically down are source_unavailable; live source with stale installed-down state is no_route until installation. |
| equal-time boundary | Packet generation/reception at physical, detection and installation ticks; include simultaneous down/up events and verify declared stage ordering. |
| cutoff before installation | Detection occurs before cutoff but installation after cutoff; no invented installation or post-cutoff failover. |

Use small direct/diamond/backup graphs with explicit wire-delay calculations and packet offers on both sides of each boundary. Freeze exact JSON before execution. Separately test zero-delay equivalence, detection-only delay, installation-only delay, invalid values, unknown fields, abstract-runner rejection and audit tampering. Do not tune traffic after seeing losses.

## Completion gate

Implement parser/validation, backend state separation, callback traces and independent audit together. Run scripts/verify.py and the full native 132-case suite with every replay. Compare the old 100 cases' seven measurement artifacts byte-for-byte with checkpoint 012. Run scripts/run_all_experiments.py for the full 132-run core regression because shared configuration changes affect both runners. Reproduce from clean source; preserve exact inputs, provenance, raw data, logs and recovery archives. Only then mark delayed control implemented and validated.
