# Corrected cellular backhaul control

This is a separate diagnostic probe for the agreed access architecture. It does
not execute configured tasks or authorization. The old wireless-construction
experiment remains unchanged for historical comparison.

Sources: `src/access-backhaul-control.cc`, `src/access-fabric-control.h`.
Preparation: `scripts/prepare_access_control.py` validates the design and exports
nodes and explicit direct-zone links. Existing output directories are refused.
Configured task types, deadlines, external service and authorization settings
are retained in input.json but are NOT executed by this control. Instead, each
robot sends ten 256-byte UDP packets to the next robot in sorted ID order.
These probes must never be presented as successfully authorized tasks.

The actual radio devices, stationary positions and shared channels retain the
construction-control defaults described in wireless-layout.md. EPC uses
NrNoBackhaulEpcHelper. gNBs connect to zone gateways; SGW and PGW connect to both
spines. AddS1Interface binds the cellular tunnel to explicit fabric addresses.
Static host routes carry the tunnel through zone gateway and spine. Internal EPC
links remain helper-provided. No invisible direct gNB-SGW link is created.
Routing is deterministic minimum hop count; ECMP and failure recovery are pending.

## Reproduction in the prepared Linux environment

From the project root, substitute the actual ns-3 source and build paths:

```sh
python3 scripts/prepare_access_control.py configs/warehouse_access_small.json /tmp/access-probe
cp src/access-backhaul-control.cc /path/to/ns-3/scratch/warehouse-access-backhaul.cc
cp src/access-fabric-control.h /path/to/ns-3/scratch/access-fabric-control.h
cmake --build /path/to/build --target scratch_warehouse-access-backhaul -j 2
/path/to/ns-3/build/scratch/ns3.46-warehouse-access-backhaul-relwithdebinfo --inventory=/tmp/access-probe/nodes.txt --links=/tmp/access-probe/links.txt --output=/tmp/access-probe/devices.csv > /tmp/access-probe/run.log 2>&1
python3 scripts/audit_access_backhaul.py /tmp/access-probe/run.log
```

Use fresh destinations to preserve results. Native inventory tables are internal
validated inputs, not a public untrusted-input parser. This control requires NR
UEs/gNBs and one zone gateway per zone. Do not run the 500-device example as if
scale, port/address limits and performance had already been established.

## Observed small result

The corrected two-zone example has 19 actual nodes and eight robot peer flows.
It delivered 80/80 packets. The independent fabric trace checker grouped 80 S1
UDP tunnel datagrams by outer source/destination and IPv4 identification and
verified their ordered three-link transmission paths in both directions:

`gNB -> zone gateway -> spine-0 -> SGW` (and reverse).

The trace checker rejects a fabricated direct gNB-core shortcut. This evidence
covers fabric transmission, not radio-hop reception, application sequence-level
latency/deadlines, the internal S5 path or authentication. Flow totals come from
the actual UDP receivers. Full trace/device repeat comparison is recorded in
the checkpoint. The direct zone link is installed but cross-zone Wi-Fi traffic
that exercises it remains a separate required control. Both spines are connected;
this deterministic shortest-hop run selects spine-0 and does not prove failover.

Raw results: local `.recovery/access-backhaul-small/`; Linux originals under
`/home/warehousebuild/warehouse-radio/access-backhaul*`. The old 153/160 control
had a different topology/workload. This 80/80 result does not resolve or erase
its ARP-loss finding and is not a radio performance guarantee.

Next add direct-link/fallback controls and the actual main-server authorization
state machine, retaining offered-task accounting and explicit upper-network scope.

## Direct-zone versus spine-only control

The current probe adds a ninth flow: first Wi-Fi robot on the first AP to the
first Wi-Fi robot on another AP (in the two-zone example, A to B). Port9008
identifies this additional diagnostic flow. Prepared input datasets retain the
same inventory, with either the configured A-B direct link or an empty link list.
This is a static topology comparison, NOT a live link/spine failure event.

Observed: nine flows,90/90 received in both datasets. All ten added datagrams
traversed AP-A -> gateway-A -> gateway-B -> AP-B with the direct link, and
AP-A -> gateway-A -> spine-0 -> gateway-B -> AP-B without it. Eighty S1 tunnel
paths remain independently checked per run. Original/replay full trace and device
CSV files match byte-for-byte for both datasets. A negative check rejects the
direct trace when a spine route is required. No latency improvement claim yet.

Evidence is retained in .recovery/access-link-control with source/input/output
hash receipt and audit.json. Run:

    python scripts/audit_access_links.py .recovery/access-link-control

The checker is intentionally specific to this small probe/port assignment.
Authorization remains unimplemented, and configured protected tasks are not run.
Next implement the task/main-server request-response gate and end-to-end timing;
live spine outage/recovery remains a separate required experiment.
