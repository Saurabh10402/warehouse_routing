# Powered warehouse communication study

This document defines the preserved four-policy baseline. The subsequent protected queue
comparison is in protected-queues.md, with declared inputs in traffic-requirements.md.
Statements below about absent reservations describe the FIFO/strictPriority baseline.

## Research direction

The primary question is how same-zone, cross-zone and edge-service traffic meet configured
class deadlines under competing traffic and component failures. Endpoints do not forward
other endpoints' packets. Fabric switches and access infrastructure have continuous power.
Battery-aware relay selection therefore has no role in this primary experiment matrix.
Legacy EALR algorithms, tests and results remain available for their original scope.

## Architecture used by this milestone

Each zone has one leaf switch. Every leaf connects to both spines, and both edge-service
nodes connect to both spines. Endpoints connect to their zone's leaf. All positions are fixed.
Counts and membership are configuration inputs; the checked-in example has 4 and 8 endpoints
in two zones, and the matrix also includes 10/35/75/20 and 500-endpoint layouts.

This first study uses **wired abstract links** to isolate fabric forwarding and queue service.
It is not an implementation of Ethernet protocols or a calibrated deployment. Existing
Wi-Fi/private-5G examples retain their explicitly abstract access links; they are not used
as evidence that real wireless peer traffic bypasses a UPF or controller. Future private-5G
integration must include an on-site UPF and validated user-plane paths. Access scheduling,
coverage, roaming and radio retransmission are separate unimplemented mechanisms.

Expected communication paths:

- Same-zone peer: endpoint -> leaf -> endpoint.
- Cross-zone peer: endpoint -> source leaf -> one spine -> destination leaf -> endpoint.
- Edge service: endpoint -> leaf -> one spine -> selected edge. The backup is used only
  when the preferred service is unreachable; application state replication is not modeled.

The zone leaf and each endpoint attachment remain single points of failure. Two spines
protect only the redundant portion. A local peer path can survive losing both spines;
a failed zone leaf interrupts its attached endpoints. All of these are explicit experiments.

## Controlled comparison

Every scenario/seed runs the same offered traffic under four policies:

| Policy | Forwarding | Queue service |
|---|---|---|
| shortest-fifo | One minimum free-flow-delay path | FIFO |
| shortest-priority | Same shortest-path rule | Strict priority |
| ecmp-fifo | Per-flow selection among equal-cost next hops | FIFO |
| ecmp-priority | Same ECMP rule | Strict priority |

The shortest-path implementation is Dijkstra, used as an ordinary baseline. ECMP uses the
same link cost (propagation plus serialization for the flow's packet size); it is not
congestion-aware. These are configurable model costs, not claims about hardware IGP metrics.
Reverse Dijkstra finds remaining costs; equal-cost next hops use relative tolerance 1e-12
with an absolute floor of 1e-12 ms. Strictly decreasing remaining cost prevents loops.
Costs must be strictly positive; fabric payload serialization guarantees positive links.

An FNV-1a hash of workload ID, source, selected destination and seed, followed by a SplitMix64
finalizer with the forwarding node index, selects the next hop. Fixed topology/flow/config
keeps the same path: this does not randomly spray consecutive packets. Node array order is
part of the exact configuration. Distinct seeds vary both workload phase and the ECMP hash;
within each seed, offered traffic is identical across all policies. Hash collisions and
unequal load distribution remain possible; equal paths do not promise equal utilization.

Failure knowledge is immediate and centralized, just as in the existing simulator. Paths
can change after a failure/restoration, and in-flight packets may be lost. This is not BGP,
OSPF, BFD, convergence timing, resilient hashing or packet reordering prevention.

Four traffic priorities keep the configurable 10/20/40/100 ms deadlines. They are targets.
Strict priority remains nonpreemptive with a finite shared tail-drop buffer: it has **no
reserved emergency slots, bandwidth reservations or admission control**. A full buffer can
drop an emergency packet; lower classes can starve. These baseline shortcomings are measured,
not silently described as solved. Protected queues and bounded high-priority admission are
the next queue-policy work if the results justify them.

## Reproducible workflow

```sh
python scripts/run_all_experiments.py
python scripts/run_all_experiments.py --suite legacy-ealr
```

Default: 11 scenarios x 3 seeds x 4 policies = 132 runs. The primary cases are mixed
communications, same-zone only, cross-zone only, uplink contention, destination bottleneck,
spine outage/restoration, zone-switch outage/restoration, edge failure, complete spine
outage, unequal zones and the 500-endpoint reference. All traffic rates/sizes are illustrative.
`--quick` uses one seed and omits the last two layouts; it is only a smoke run.

The legacy option preserves the earlier 114-run EALR matrix and original configs. Prior
batch directories and checkpoint manifests are immutable and are not relabeled as new results.

Raw packet traces, per-class counts and provenance remain unchanged. New
`scope_priority_comparison.csv` reports each scope/class/seed/policy separately. Its scope
comes from the workload's intended primary destination, so unreachable and failed packets
remain in the correct denominator. All losses count as deadline misses. Mean delay uses
delivered packets only, and is blank in this scope table when none arrive. Plots include
scope/class deadline misses alongside aggregate latency, delivery and throughput.

Tests verify directed equal-cost selection, rejection of expensive/failed paths, stable
flows, local paths, cross-zone paths, complete and partial spine failures, service failover,
and loss-inclusive scope metrics. The runner checks identical arrivals across all policies,
no endpoint transit, physical positions, exported counters and deadline flags, plus replay.

## What still requires real warehouse input

Actual layout and port/uplink capacities; proportions and burst patterns of peer/service
traffic; packet sizes and class assignment; tolerable failure domains; access technology
per endpoint; and whether the stated deadlines include application processing/actuation.
These do not block an explicitly labeled baseline. They must be established before drawing
deployment conclusions. Current measured delays cover packet generation to delivery only.
