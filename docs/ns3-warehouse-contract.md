# Next integration contract: stationary schema-v2 in ns-3

This is the staged implementation and acceptance contract. The initial stationary wired
FIFO/Dijkstra slice is now implemented as `ealr-ns3-fabric`; see its
[supported subset and validation workflow](ns3-wired-warehouse.md). Later gates remain open.
The small `ealr-ns3-smoke` fixture establishes dependency/runtime prerequisites only.
Keep existing abstract-link results and their model labels unchanged.

## First supported slice

Consume the existing validated `fabric::Configuration`. Start with stationary wired nodes,
explicit full-duplex links, a single destination per flow, deterministic traffic, and
shortest-path forwarding. A zone is a configuration label, never an assumed count or size.
Represent the first wired leaf/spine model explicitly as IP routers. This does not establish
transparent Ethernet switching, Wi-Fi association, or a private-5G user-plane implementation.

Reject unsupported fields/modes before starting the simulator. In particular, do not silently
ignore ECMP, protected queues, availability events, backup destinations, energy limits or
local safety events until each has an implementation and tests. A staged backend must clearly
state which configurations it accepts; the existing primary matrix remains on the core runner.

Map each node to an ns-3 node with a constant-position mobility model. Map each link to a
point-to-point device pair with its configured rate and propagation delay. Define exactly
which bytes `traffic.bytes` represents. The core currently models abstract packet bytes;
an ns-3 application also creates transport, IP and link headers. Preserve both payload and
on-wire measurements and explain expected differences rather than requiring equal delays.

Use application packet IDs and generation timestamps so the end-to-end deadline denominator
includes every offered packet. FlowMonitor is an independent IP-layer cross-check; it cannot
alone supply application deadlines, priorities or all admission/drop causes. Keep simulation
drain time distinct from FlowMonitor's timeout-based loss classification.

## Acceptance gates before expanding the matrix

1. One-link and routed multi-link fixtures: exact counts, serialization/propagation delay,
   path and stationarity; an actual link receive-error fixture must lose packets.
2. Same-zone endpoint -> leaf -> endpoint; cross-zone endpoint -> leaf -> spine -> leaf ->
   endpoint; and endpoint -> edge service. Assert the selected path from packet observations.
3. Unequal zone sizes and non-reference zone counts; disconnected destinations and malformed
   or unsupported inputs must fail clearly or produce the documented packet-loss outcome.
4. Finite queue overload: account for packets admitted, transmitted, waiting and dropped.
   Define whether capacity excludes the in-service packet. Avoid hidden buffering in both
   QueueDisc and NetDevice that would invalidate the requested queue capacity.
5. FIFO/strict-priority comparisons with identical offered traffic; test nonpreemption,
   priority order, same-priority order and lower-priority starvation under overload.
6. ECMP must pin flows consistently and document the hash and equal-cost metric. Observe
   actual paths; do not substitute ns-3 random per-packet ECMP for the core's per-flow policy.
7. Add failure/recovery and backup services with an explicit detection/convergence model;
   immediate central knowledge in the core is not a measured routing protocol.
8. Only then add reservation/policing/weighted service and safety fan-out, with separate tests
   for admission loss, queue conservation and the local application processing boundary.

Each completed gate needs raw packet evidence, executable/config/dependency/source hashes,
deterministic replay where applicable, and a separately named ns-3 result batch. Retain losses
and adverse results. Use multiple seeds only for declared stochastic mechanisms; repeating a
deterministic fixture with three seed labels does not establish stochastic performance.

## Radio and target-host gates

NR requires a separately pinned compatible ns-3/5G-LENA pair, actual UE/gNB devices, EPC/UPF
and explicit endpoint/service IP paths. Confirm user-plane packets at the receiver before
evaluating radio QoS. A gNB icon or successful attachment is insufficient. Wi-Fi access likewise
needs its own MAC/PHY/association model and validation. Do not silently convert wired links.

MacBook Air M4 and Ubuntu 24.04 ARM64/UTM need actual clean-host builds and runtime checks.
Prepared commands and Windows source-extraction tests do not fulfill that gate. Keep the
first-run guide usable while these steps remain open; do not label the whole product complete.
