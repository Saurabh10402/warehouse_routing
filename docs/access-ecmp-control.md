# Stationary wireless ECMP control

> **Historical ECMP source-address caveat:** the ECMP results below predate the
> correction described in [the eight-zone guide](access-eight-zone-control.md).
> Some outer NR tunnel packets used ns-3's uninitialized source-address sentinel.
> Their delivery records remain preserved, but they do not satisfy the stronger
> source-validity audit. Use the corrected controls for current ECMP validation.
> The shortest-path results and documented cold-ARP delay are not invalidated.


The native mixed NR/Wi-Fi task model supports `fabric.routing: ecmp` in addition
to the preserved `shortest-path` baseline. ECMP means equal-cost multipath: when
several next hops lead to equally short routes, a stable hash selects one. It
does not choose routes based on instantaneous congestion or guarantee equal load.

## Reproduce on the existing Linux/WSL radio installation

```sh
python3 scripts/run_access_authorization.py configs/access_ecmp_control.json --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

This uses the pinned ns-3.46/NR v4.1.1 build already described in
[the authorization run guide](run-access-authorization.md). It does not download
or install dependencies. The control has nineteen local devices plus an external
server model, eight stationary wireless robots across two zones, and36 tasks.
Four basic mixed-access tasks and eight repeated Wi-Fi pair tasks run in each
of three phases. Spine0 is down at2000ms and restored at3500ms. Direct zone links
are initially disabled so cross-zone Wi-Fi traffic exercises the spine choices.

The schema retains its historical `implementationStatus: design-only` label
because it also describes pending capabilities and larger layouts; the maintained
runner's capability checks define the implemented subset. ECMP is now accepted.
The conservative64-device control ceiling remains, not a validated capacity claim.
`--boundary-control` with ECMP is explicitly rejected: that targeted loss audit
is currently validated for shortest-path only. Ordinary ECMP outage tests are
phase-separated and do not establish transition-boundary loss behavior.

## Route selection

This wireless subset uses **hop count** as cost, matching its existing BFS baseline.
It is separate from the older wired model's size-aware weighted Dijkstra study.
Core and external-server nodes may be destinations, but never transit shortcuts.
The route builder tests every first hop, retains only equal shortest choices,
and sorts them by next-node identifier. During an outage it excludes the failed
spine; restoration recreates the original candidate sets. Routes decrease the
remaining shortest-hop distance, preventing a fabric forwarding loop.

The hash input is the forwarding node name, source IPv4 address, destination
IPv4 address and IP protocol. FNV-1a32 followed by a32-bit avalanche mix selects
an index modulo the candidate count. This is **Layer-3 per-IP-flow ECMP**, not a
UDP five-tuple hash: different application ports between the same IP endpoints
stay together. UDP headers are not guessed during a local socket route lookup,
when a transport header may not yet exist. Locally generated traffic with an
unspecified source uses the node's stable identity address.

The higher-priority `AccessEcmp` IPv4 protocol handles generated fabric routes.
Existing static/helper routes continue handling local Wi-Fi delivery and EPC
interfaces. `shortest-path` keeps the prior routing implementation. The configured
authorization behavior is independent: protected tasks must obtain approval from
the main server, then route payload locally; denial or timeout blocks payload.

**NR tunnel limitation:** GTP outer packets are hashed using outer IP addresses
and protocol. Traffic inside the same outer tunnel stays grouped. This control
does not inspect TEIDs or inner robot flows, and therefore does not demonstrate
per-task or per-UE load balancing inside a shared tunnel. The shared local EPC
remains below the upper-network boundary; there is no new NR local-breakout claim.

## Independent packet audit

`ECMP_DEST` logs associate destinations with their fabric owner. The auditor
checks owner assignments against the inventory and reconstructs the physical
fabric from `LINK` records. It computes reverse shortest distances independently
of the native per-first-hop search, then recomputes the hash for every observed
UDP `WIRE` hop. Unknown destinations, a wrong equal-cost hop, failed-spine use,
inconsistent repeated flows or changed restoration mapping fail the audit.
Authorization ordering, no payload through the server, event times, route rebuilds
and exact replay checks remain active.

The main control checked210 wire hops,42 selections with multiple equal choices,
12 repeated multipath-flow groups and41 before/after flow mappings. Both spines
were used before and after failure; only spine1 was used during the outage.
All36 offered tasks delivered within their configured100ms deadlines in this run.

## Executed controls

Every row passed its stated audit with byte-identical original/replay traces and
device inventories. Denied/timed-out tasks remain in offered counts. The last row
is a loss-accounting control, not an all-delivered success: it retains one observed
payload drop as well as two authorization timeouts.

| Control | Offered | Delivered | Blocked | Results directory |
|---|---:|---:|---:|---|
| ECMP outage / allow | 36 | 36 | 0 | `access-auth-20261107T061316Z-552a3f63` |
| ECMP outage / deny | 36 | 30 | 6 | `access-auth-20261107T061459Z-872adede` |
| ECMP outage / timeout | 36 | 30 | 6 | `access-auth-20261107T061513Z-6a50d151` |
| ECMP with direct zone link | 36 | 36 | 0 | `access-auth-20261107T061524Z-bc43613c` |
| ECMP without outage | 36 | 36 | 0 | `access-auth-20261107T061537Z-412d1830` |
| Shortest-path, identical traffic | 36 | 36 | 0 | `access-auth-20261107T061552Z-1ecea1bb` |
| Previous phased control | 12 | 12 | 0 | `access-auth-20261107T061605Z-1ab8db46` |
| Previous static control | 3 | 3 | 0 | `access-auth-20261107T061617Z-224f9f53` |
| Previous boundary-loss control | 46 | 43 | 2 | `access-auth-20261107T061628Z-2e5b5f72` |

For a matched comparison, only `fabric.routing` differs between the main ECMP
input and the36-task shortest-path input. Both delivered36/36, with median task
latency1.2494385ms and maximum45.002193ms. The observed result demonstrates path
selection and recovery, not a latency improvement or a congestion benefit.
No lower-traffic starvation, industrial deadline guarantee or equal utilization
claim follows from this small control.

The prior phase, static and boundary raw traces and device inventories also match
their checkpoint041 references byte for byte. Tests reject incorrect equal-cost
choices, wrong destination owners and unmapped packets. All34 access tests and
21 standard project checks pass. The initial build diagnostic
`access-auth-20261107T061104Z-6c1569c5` is preserved: its callback signature needed
const-reference arguments for the pinned ns-3 API; no simulation ran in that batch.

These are deterministic single-seed controls (radio seed1/run1, fixed hash), not
a multi-seed performance evaluation. Failure knowledge remains instantaneous and
centralized; real routing-protocol convergence, movement, upper TSN/shim/5G,
cryptographic authentication, inner-NR-flow ECMP, ECMP boundary-loss testing and
500-device wireless scaling remain outside this result. Native radio priority
queue comparisons are also separate from the older wired queue study.

Next make radio seed/run configurable and perform a bounded multi-seed policy
comparison before scaling the wireless layout. Keep every current regression
and its loss accounting when extending the model.

## Subsequent multi-seed validation

The [radio seed comparison](access-seed-comparison.md) now validates matched
policies across seeds1/2/3 and a separate run2 control. The ECMP hash stays fixed.
Earlier single-seed results above remain preserved.
