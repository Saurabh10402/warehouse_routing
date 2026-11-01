# Native availability and backup-service baseline declaration

Declared before implementation/experiments for checkpoint_012. Extend only the native wired
stationary backend, preserving the core study and the existing 48 static native cases.

Detection and centrally installed table updates take zero simulated time. This is a reference
assumption, not a routing protocol, BFD/OSPF convergence measurement or deployment guarantee.
Nonzero detection/reconfiguration delay is not implemented in this slice. Ground-truth node
and link availability initially comes from configuration. Only node/link events are accepted;
safety/cloud/temperature/reset events remain rejected. Rebuild tables for all candidate
services after every availability event; preserve transport hash and wire-size costs.

Outages use an ns-3 receive ErrorModel on each point-to-point device. A frame is discarded
at its scheduled reception if its link or either incident node is unavailable then. Real
PhyRxDrop observations distinguish this loss from shared-queue overflow. This is a declared
receive-time availability gate, not physical power cycling or a bit-error/cable interruption
model. An outage that begins and ends before reception does not destroy that frame.

Existing queued frames are not flushed, migrated or preempted; device serialization keeps
running. A queued/in-flight frame can therefore arrive after recovery and be accepted. If
its next hop receives while unavailable, it is lost. At a live intermediate router it uses
the current table; losing reachability there produces an observed IPv4 drop. Changes can
make actual paths differ from the route planned at generation. All attempts are audited
against contemporaneous topology; forwarding TTL guard bounds transient loops (described below).

Offered application generation remains scheduled even when the source is unavailable;
record source_unavailable before IP transmission. Otherwise choose the first currently
reachable destination from the configured ordered candidates. No candidate means no_route.
Each packet pins its chosen destination and five-tuple; it is not redirected to backup in
flight or retransmitted. Candidate services have distinct native UDP receivers. There is
no session/application state replication or failback hysteresis.

Events are scheduled before offered-packet events. At equal times, availability events take
precedence over packet generation, native receptions and service callbacks; simultaneous
availability events retain configuration order. Frame times use ns-3 nanosecond rounding.
Event timestamps and initial topology are exported for independent audit. Failure/recovery
can occur during an ongoing queue; cutoff packets remain censored, all losses remain in
full offered/deadline denominators. The link/node gate does not change finite-buffer policy.

Acceptance controls must cover link and node down/up with alternate-route use, source outage,
all routes unavailable, initially unavailable resources, preferred/backup/failback behavior,
no reachable services, same-zone survival of spine loss, leaf single-point outage, reception
at an event boundary, queued frames crossing outage/recovery, and in-flight route loss.
Use both Dijkstra/ECMP and FIFO/strictPriority where applicable. Preserve the old 48 cases,
every replay, independent packet/header/service audit and full 132-run core regression.
No default workload or queue is tuned to conceal losses.

The final declared new matrix is 13 fixtures x Dijkstra/ECMP x FIFO/strictPriority = 52
resilience controls, each seed 1. Fixtures: link-alternate, node-alternate, service-failback,
all-services-down, source-outage, initial-unavailable, queued-recovery, receive-boundary,
inflight-route-loss, same-zone-survival, leaf-outage, simultaneous-events, ttl-loop. Together with the
original 48 static cases, the runner executes 100 cases and 100 replays. Exact generated inputs
are frozen before the first run. See scripts/ns3_warehouse.py for the declared fixture data.
These are deterministic acceptance controls, not statistical availability/reliability trials.

Before final validation, add the ttl-loop stress control: 510 availability events alternate
the reachable exit of two relays, forcing one packet to traverse 255 physical hops. A live
router with TTL=1 invokes the native IPv4 route-error callback before forwarding; ip_drops.csv
records actual reason 5 and TTL 1. This bounds forwarding without generating unsupported ICMP
control packets. Destination-local delivery with TTL=1 remains valid. This explicit guard is
not a claim that a deployed router suppresses ICMP. The independent audit checks the exact
255-hop count, TTL, loss accounting and absence of a delivery. Original 96-case results are
retained as an intermediate validation milestone; the final suite has 100 cases.
