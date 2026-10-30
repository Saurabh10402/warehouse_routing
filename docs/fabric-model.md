# Stationary warehouse model (schema version 2)

## Scope and fidelity

`ealr-fabric` is an independent C++17 discrete-event packet simulator. It is not ns-3,
an Ethernet switching implementation, a Wi-Fi MAC, a 5G NR scheduler, or a full 5G core.
It explicitly models graph connectivity, finite packet queues, application payloads,
per-direction link capacity, serialization, propagation delay, priorities, energy,
and component availability. Access labels describe abstract service links. Their
configured rates/delays are experiment inputs, not calibrated radio measurements.

No physical positions change. Configuration is immutable; the only mutable node fields
are availability and battery. Runtime and tests compare initial/final coordinates;
there is no mobility event or setter in this simulation. Legacy `randomWaypoint` and
return-to-charge animation are outside this primary workflow.

## Configuration and topology

The root requires `schemaVersion: 2`, `name`, `topology`, `simulation`, `zones`, `nodes`,
`links`, and `traffic`. Optional `routing`, `deadlinesMs`, `safety`, and `events` have
the documented defaults in `fabric_configuration.hpp` and below. The shipped
examples are fully expanded; the C++ simulator never guesses connectivity or zone counts.

- `topology.kind`: `graph` for arbitrary test graphs, or `leafSpine` for role-validated
  endpoint/access/leaf/spine/edge paths. `minimumSpinesPerLeaf` and `minimumEdges` default
  to zero; the reference requests two of each. Single-spine/edge layouts are supported.
- `zones`: distinct string IDs. Unequal membership comes directly from each node's `zone`.
- Nodes require `id`, `role`, `x`, `y`. Roles: endpoint, access, leaf, spine, edge, relay.
  Endpoint/access/leaf nodes require a zone. Edge/spine nodes may be zone-independent.
  `access` defaults to wired, also accepts wifi/private5g. `available` defaults true.
- Leaf-spine endpoint attachment must match its access technology. Wireless endpoints
  attach to a matching access node in their zone; access nodes attach to their zone's leaf.
  Wired endpoints may attach directly to the leaf. Leaf-spine and spine-edge links form
  the fabric. Other graph arrangements must deliberately select `graph`.
- Links require `id`, `from`, `to`, `capacityMbps`, `delayMs`. They are full-duplex with
  independent queues in each direction; `queuePackets` defaults 100 **waiting** packets,
  excluding the transmitting packet. `available` defaults true. Parallel links/self-links
  are rejected. Use intermediary nodes if distinct parallel paths are needed.
- Structural reachability to an edge is required for every leaf-spine endpoint, ignoring
  runtime availability so failure experiments can intentionally start disconnected.
  Graph mode allows disconnected components for negative routing tests.
- Only endpoint/relay nodes may specify `energy`: batteryPercent required, criticalPercent
  default 10, txPercentPerByte/rxPercentPerByte default zero. Fixed infrastructure has no
  battery penalty. This is percentage-point accounting, not a calibrated joule model.
- `simulation`: durationMs required; drainMs defaults 100, seed 1, scheduler strictPriority.
  Alternative scheduler `fifo` is an explicit priority ablation. No physical mobility field.
  Optional `protectedPriority` and `simulation.protectedQueues` add reserved emergency slots,
  a per-direction P0 admission bucket, and weighted lower-class service; see protected-queues.md.
- Traffic requires id, source, ordered destinations, priority 0..3, bytes, stopMs, intervalMs.
  startMs defaults zero, phaseJitterMs zero. One seeded random phase per flow is uniform
  in [0,jitter); subsequent arrivals are periodic in [start+phase,stop). Jitter < interval.
  Packet size is 1..1,000,000 bytes; interval >= 1e-6 ms; at most 10 million packets/flow.
- IDs use letters, digits, dash, underscore and dot, allowing unambiguous CSV paths.
  Coordinates are finite and within +/-1e9 metres; capacities 1e-6..1e9 Mbps; delays
  0..1e9 ms. Invalid/unknown fields and references are rejected, never silently replaced.

## Routing and scheduling

Existing Dijkstra, Greedy and EALR classes are reused through `RoutingRequest.linkCost`.
The legacy no-callback API retains its original distance/energy/queue-length model.
The configuration enum is now `RoutingAlgorithmKind` to avoid colliding with the interface.

For the stationary fabric:

- Dijkstra minimizes link propagation + serialization of this packet (milliseconds).
- ECMP selects among equal-cost paths using the same positive link costs and a stable
  flow hash. It does not inspect queue occupancy or consume energy weights. Full hash,
  equality tolerance and failure semantics are specified in `docs/warehouse-study.md`.
  Dijkstra/ECMP are the primary study policies; Greedy/EALR are retained legacy comparisons.
- Greedy selects the eligible, unvisited neighbour geographically closest to the destination,
  using numeric node index for ties, then reports the configured link costs. It does not
  backtrack. Geography can be a poor heuristic for a switch fabric; failure is reported.
- EALR minimizes `alpha * baseMs/latencyScaleMs + beta * energyFraction + gamma * waitMs/latencyScaleMs`.
  Energy fraction is depletion of the receiving constrained device, otherwise zero.
  WaitMs estimates residual transmission plus queued bytes of equal/higher priority (all
  queued bytes for FIFO). All terms are dimensionless. Defaults alpha=beta=gamma=1,
  latencyScaleMs=1 and maxStretch=1.5 are neutral illustrative settings, not fitted winners.
  Alpha must be positive; beta/gamma nonnegative; all must be finite.
- A guard compares the selected route's estimated latency with the Dijkstra route at the
  same instant. It falls back if the candidate exceeds maxStretch times that estimate,
  or is predicted to miss the remaining deadline while the baseline estimate is lower.
  This is a snapshot heuristic, **not** a strict deadline guarantee, reservation, or
  multi-objective optimum. Future arrivals can invalidate the estimate. A constrained
  shortest-path solver was avoided to keep a small, auditable research prototype.
- Routes are recomputed at **every forwarding hop**, and when waiting packets are flushed
  by a component failure. Ordered primary/backup destinations are tried for reachability.
  A backup is not automatically chosen solely because the primary is congested.
  Already-visited nodes are excluded; endpoints cannot act as intermediate relays.
- Strict priority 0 before 1 before 2 before 3; FIFO within each class. Transmission is
  nonpreemptive. A safety packet waits for an already-transmitting frame. The finite shared
  waiting buffer uses tail drop, so even safety packets can be lost if it is full.
  Lower classes can starve. There is no bandwidth reservation or priority admission control.
- Simultaneous event order: configured failure/safety inputs, link completion/arrival,
  application generation, service dispatch; insertion sequence resolves remaining ties.
  Nodes and neighbour lists have stable numeric indices in configured array order.

## Failure, energy and safety semantics

Node/link events provide target, timeMs and available. Availability changes are immediately
visible to routing (ideal centralized knowledge; no detection/convergence protocol delay).
Affected waiting packets are rerouted; packets in transmission or propagation are lost
if either endpoint or link changes epoch before arrival, even after a quick recovery.
Packets without routes are dropped, not buffered until connectivity returns. No retransmission.
Restored components become usable for future forwarding decisions.

Each constrained transmitter/receiver consumes configured percentage points per payload byte.
At/below critical threshold it becomes ineligible. A transmission that causes its sender to
cross the threshold may finish; a receiving destination may accept its last packet before
exclusion. Intermediate depleted receivers cannot forward. No charging or idle energy model.

Temperature/reset events require a zone and an explicit local edge target. They represent
readings **already available at that edge**, excluding sensor transport and real actuators.
After configurable processingMs (default 0.1), the existing controller processes the reading
or acknowledgement. Default warning 70C, stop 80C, manual reset below 70C, local processing
required, broadcast enabled. Emergency state is latched until valid reset. Each emergency
reading can generate a 64-byte priority-0 unicast copy to each endpoint in the zone. Copies
traverse the same queues/routes and can fail. This is logical fan-out, not link-disjoint packet
replication or Ethernet broadcast. Disabling broadcast suppresses copies but retains local state.

Cloud events are logged but never gate local processing or traffic. A failed local controller
is explicitly logged as unavailable; it is not silently replaced. Configure the secondary edge
as a safety event target to exercise that controller. This is not certified industrial safety.

## Metrics and output contracts

All latency observations come from discrete packet generation and delivery events, not graph
path costs. This is still abstract-link simulation time, not measured ns-3 packet latency.

| Artifact | Meaning |
|---|---|
| config.json | Exact input bytes used by the executable |
| packets.csv | Every generated packet: status, creation/finish ms, delivered latency, actual hop trace |
| metrics.csv | Aggregate metric,value rows |
| per_priority.csv | Independent class 0..3 counts, delivery, deadlines, latency and throughput |
| queue_stats.csv | Per-direction/class admissions, rejection causes, service starts, waits, peaks, reroutes and cutoff occupancy |
| nodes.csv | Initial/final positions, availability, battery for every node |
| events.csv | Availability/cloud/energy event log |
| safety.csv | Local controller response time and state; broadcast delivery is in packets.csv |
| recovery.csv | First delivered post-failure packet per configured flow, or blank when none |
| provenance.json | Runner-generated source/config/binary hashes, seed and algorithm |

- Observation window is [0,durationMs+drainMs]. No ordinary traffic is generated at/after its
  stop time. Pending packets at cutoff are losses, including in-flight/queued packets.
- Sent denominator includes every generated application packet, including safety copies.
  PDR = delivered/sent; loss = lost/sent; zero sent yields zero rates.
- Deadline success is strictly latency < configured class deadline (defaults 10/20/40/100 ms).
  Deadline misses include lost packets; denominator is all generated packets in that class.
- Mean/median/percentiles use **delivered packets only**. No delivery yields zero mean/median
  with delivered=0; interpret this as no observations, not zero latency. Median averages the
  middle pair; p95/p99 use nearest rank and appear only with >=20/>=100 delivered observations.
  Percentiles use raw packet delays, never averages. Small sample estimates remain uncertain.
- Application throughput = delivered payload bits / entire observation window; Mbps. No
  protocol overhead, headers, retransmissions or PHY throughput are included.
- Battery statistics include only constrained devices, one final sample/device. Consumed
  energy sums percentage-point depletion; this is not additive physical energy across unlike batteries.
- Network lifetime = first constrained device's critical exclusion time, including zero for
  initially excluded devices. If none, report the window end with censored=1. Omitted when
  there are no constrained devices. It is not the time until all network service disappears.
- route_recomputations counts route-selection attempts (including backup attempts and initial
  routes), not CPU time or distributed routing protocol messages.
- observed_recovery_ms is the first delivery of a packet **generated after the failure**, minus
  failure time, per flow. It includes time until the next arrival and delivery; may refer to a
  flow unaffected by the failure. It is not an isolated convergence measurement. Use paths
  and events together to establish which traffic actually used an alternate path.

The experiment runner uses identical topology/traffic/seeds across policies, emits sample
counts and between-seed sample standard deviations, and keeps ablations in separately named
cases. A no-jitter case has one deterministic sample; stochastic cases use three seeds.
These small illustrative samples are not confidence claims or proof of general superiority.
