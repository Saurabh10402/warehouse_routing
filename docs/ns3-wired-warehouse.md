# Native wired warehouse backend: static schema-v2 policies

`ealr-ns3-fabric` consumes the existing schema-v2 configuration and sends actual IPv4/UDP
packets through ns-3 point-to-point devices. Nodes remain stationary. Zone sizes and zone
counts are input data. The new example has three zones with 2, 1 and 3 endpoints.
The existing `ealr-fabric` model and its primary research matrix remain independent.
See [checkpoint_009 results](ns3-warehouse-results.md) for actual tests, raw outcomes and
the source-extraction comparison; the statements below define the implemented model.

## Supported behavior and model boundary

This slice supports available, continuously powered wired nodes, static topology, one
destination per flow, deterministic periodic UDP traffic, finite FIFO or strict-priority queues and Dijkstra or per-flow ECMP forwarding.
See [the declared native comparison](ns3-policy-study.md) for queue and transport semantics.
The wired leaf/spine nodes are modeled as IP routers, not transparent Ethernet switches.
No radio, NR user plane, routing-protocol convergence or physical warehouse calibration is
implied. This is a centrally installed forwarding model with native packet transport.

Each node has a stable virtual /32 address on its loopback interface. Each point-to-point
link has its own /30 subnet. Static next-hop tables are keyed by explicit IP/UDP five-tuple and UDP payload
length, so mixed packet sizes can choose different minimum free-flow-delay routes. The
existing Dijkstra/ECMP implementations supply those tables, with ECMP hashing the actual
transport identity and seed; observer tags never select a route. Costs include propagation and
serialization of the payload plus UDP, IPv4 and PPP headers. Endpoint nodes cannot carry
transit traffic. Native IP forwarding uses the installed output device and next-hop address;
reported paths come from physical receive traces, independently of the planned table.

`traffic.bytes` means UDP payload bytes here. IPv4 bytes are payload+28 and PPP frame bytes
are payload+30. Observer packet-ID tags add no wire bytes and do not decide routes. This
differs from the core's abstract byte convention; raw delays should not be equated without
accounting for headers and ns-3's nanosecond time resolution. Capacity must represent a whole
number of bits per second. Sub-nanosecond timing is rounded to the ns-3 clock.

Each link direction has one native DropTail FIFO or custom strict-priority device queue.
Strict priority reads IPv4 DSCP, keeps FIFO order within class and never preempts a frame.
The buffer remains shared tail-drop: P0 can be rejected when full and lower traffic can starve. `queuePackets` counts waiting
packets, excluding the in-service frame. Device flow control is disabled so address setup
does not create a hidden default QueueDisc. Native attempt/enqueue/transmit/drop traces
establish the admission and service counts. Queue capacity, both conservation equations and
configured service order are independently checked from exported observations. The reported peak includes
the instantaneous enqueue before immediate service on an idle device.

## Strict unsupported-input checks

The backend rejects Greedy/EALR, protected priority scheduling, all configured
events, unavailable nodes/links, energy-constrained nodes, Wi-Fi/NR access, backup destinations,
self-destination flows, nonzero phase jitter, and payloads above 1472 bytes (IPv4 fragmentation).
Safety policy fields are validated by the common schema but are inactive with no events;
temperature/reset/cloud events are rejected. Unsupported inputs fail before simulation/output.
The primary warehouse configuration uses unsupported policies; it is never silently converted.

Initial explicit resource limits: 4096 nodes, 4096 flows, 16384 links, one million offered
packets; seed 1..1,000,000,000; duration at least 1 ns and total duration+drain at most 1e9 ms.
Routes must fit the configured IPv4 TTL of 255. These bounds do not establish performance
at those scales. Route-table construction and detailed traces are intended for controlled
acceptance cases first.

## Run and inspect

After completing the prerequisites in [the first-run guide](first-run.md):

```sh
python scripts/run_ns3_warehouse.py
```

This requires real ns-3, configures/builds/tests the project, runs all 12 declared
acceptance controls plus the 36-run forwarding/scheduling matrix, independently audits every
exported packet/queue/node/header trace, and replays every case byte-for-byte. The original
seven controls are preserved. Seeds 1/2/3 vary the ECMP hash only: this is deterministic hash
sensitivity, not stochastic replication. Scope/class tables retain all offered packets.

The command prints a new `results/ns3-warehouse-...` directory. Each case includes its exact
input plus `packets.csv`, `hops.csv`, `queues.csv`, `nodes.csv` and `summary.json`. The batch
preserves input/source/executable/ns-3 library hashes and the dependency setup receipt.
`RUN_STATUS.json` is written only after all checks and replay pass. A later run creates a new
directory; existing results are not overwritten.

For one supported input, use the built `ealr-ns3-fabric --config ... --output NEW_DIRECTORY`
executable in the environment configured by the verification script. The Python workflow
handles runtime library paths and is the recommended first command.

The communication fixture has three 100-byte packets on 5 Mbps / 2 ms links. Expected paths
and unloaded application delays are:

| Flow | Actual path to check | Analytical delay |
|---|---|---|
| Same zone | A0 -> leafA -> A1 | 4.416 ms |
| Cross zone | A0 -> leafA -> spine -> leafB -> B0 | 8.832 ms |
| Edge service | C2 -> leafC -> spine -> edge | 6.624 ms |

## Loss and deadline interpretation

All offered packets remain in the deadline denominator. Delivered packets can still miss
their configured deadline. `no_route` is a static-table reachability rejection before IP transmission;
`queue_overflow` is a native device drop. `cutoff` means still queued/in flight at the
duration+drain boundary and is censored, not proof of permanent loss.

FlowMonitor's automatic timeout is set beyond this run's cutoff. At the cutoff its explicit
zero-delay lost-packet check counts outstanding IP packets for reconciliation. The exported
field is therefore `ip_unreceived_at_cutoff`; it must not be presented as eventual network
loss. Native application receive counts and IP byte counts are checked independently.

The raw audit reconstructs physical paths and delays, directional queue policy/occupancy,
minimum route cost, stationarity, offered traffic and deadline outcomes. It also detects
deliberately corrupted summary counts and wire-byte records in the CLI tests.

Next extensions are explicit failures/services, protected queues and radio access. Keep the
[integration contract](ns3-warehouse-contract.md) and recorded checkpoint limitations visible.
Mac M4 and Ubuntu ARM64 execution remain separate uncompleted validation gates.
