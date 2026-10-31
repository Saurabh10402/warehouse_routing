# Native wired priority / ECMP comparison declaration

Declared before execution for checkpoint_010. This extends the separate native ns-3
wired backend; it does not replace the abstract core study or validate NR.

Run all 12 deterministic acceptance controls plus a 36-run factorial matrix:
three scenarios (mixed peer/service communication, leaf-uplink contention, destination
bottleneck) x seeds 1/2/3 x Dijkstra/ECMP x FIFO/strictPriority. The exact generator is
scripts/ns3_warehouse.py. Its inputs are saved before any case executes. The original
seven FIFO/Dijkstra controls remain; five additions cover priority ordering, a full
shared buffer, starvation at cutoff, and ECMP size/endpoint eligibility.

The matrix adds a second equal-cost spine to the unequal 2/1/3-endpoint three-zone
fixture. Twelve flows span same-zone, cross-zone and edge-service traffic at all four
priorities. Payloads P0..P3 are 64/128/256/1000 bytes; intervals 4/6/8/10 ms. Start times
are priority*0.1 + scope_index*0.01 ms; stop/duration 80 ms, drain 1000 ms. Base links
retain 5 Mbps, 2 ms and their configured buffers. Contention changes both spine links
from leafA and leafC to 0.2 Mbps / four waiting packets; the destination case instead
changes both edge-service links to those values. All traffic is illustrative.

Identical offered traffic is required across all four policies within every scenario
and seed. Seeds change only deterministic ECMP hashing here: no stochastic arrivals or
radio process exists, so this is three-seed hash sensitivity, not stochastic replication,
confidence intervals, or deployment reliability evidence. Compare every scope/class,
including dropped and censored packets in delivery and deadline denominators. Report
observed delay only for delivered packets. Preserve FIFO controls and every replay.

Strict priority uses one finite shared tail-drop waiting buffer per direction. P0 is
highest; equal priorities keep admission order. In-service frames are never preempted.
No eviction, reservations or policing: high priority may be dropped when full, and
lower priority may starve. Classification reads actual IPv4 DSCP: P0/P1/P2/P3 map to
CS7/CS5/CS3/CS0 (TOS 224/160/96/0, ECN zero). This is a declared local mapping, not a
claim that external equipment is configured for it. Header fields are exported/audited.

Each flow binds explicit IPv4 source and UDP source port 20000+flow_index; destination
port is 10000+flow_index. Addresses are 172.16.0.1+node_index. ECMP keys use the real
(source IPv4, destination IPv4, protocol 17, source port, destination port) and payload
size; observer IDs never affect forwarding or classification. FNV-1a hashes big-endian
4/4/1/2/2-byte tuple fields and the four-byte seed. The existing ECMP algorithm applies
its SplitMix64 finalizer with forwarding node index. Eligible next hops have equal
minimum propagation + (payload+30)*8/rate cost with relative tolerance 1e-12 and floor
1e-12 ms, strictly decreasing remaining distance; candidates sort by node index.
Tables are installed centrally per flow. Array ordering is therefore part of input
identity; this is not an IGP, resilient hashing, per-packet spraying or adaptive routing.
Hash collisions and uneven branch use remain possible.

Acceptance: real wire headers match each flow; all delivered paths have minimum cost;
independent Python calculation agrees with ECMP hash/next hops; native transmission
order matches queue policy without overlap; admission, occupancy, drops and cutoff
conserve packets; all runs replay exactly. C++ controls separately test both eligible
branches, stable multi-packet flows, changed seed, nonpreemption, FIFO within priority,
full-buffer P0 rejection and finite-horizon starvation. Original FIFO results compare
on all pre-existing fields (new wire-header columns are additionally audited).

Failures/recovery, backup services, protected queues, random phase, radio and
fragmentation remain rejected. Static no-route is detected from the installed table
before sending because explicit-address ns-3 UDP sockets otherwise report acceptance
when IPv4 has no route. It remains in offered/deadline totals and out of IP transmit totals.
