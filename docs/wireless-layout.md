# Stationary wireless configuration

This milestone defines and validates device inventory and radio attachment intent.
It does not run ns-3 and is not input for the existing wired runner. Connecting this
inventory to actual NR/Wi-Fi devices and warehouse routes is the next milestone.

Edit `configs/warehouse_wireless_layout.json`. The example has eight zones and
500 modeled devices: 463 robots, eight switches, eight edge servers, eight Wi-Fi
APs, eight NR gNBs, two shared spines, and three shared EPC nodes (PGW, SGW, MME).
Core nodes are simulation entities, not a claim of three physical appliances.
The private cellular access uses the 5G-LENA EPC model, not a full standalone 5G core.

Each entry in `zones` sets its device counts. Add or remove entries to change the
zone count. `robots.nr` and `robots.wifi` explicitly set the access split; their
sum is the zone robot count. IDs include access type (for example
`zone-A-nr-robot-0`); changing that split changes those identities. There is no
automatic latency-based selection or claim that either access meets a deadline.
Other wireless technologies are not currently supported by this contract.

`originM` is the robot grid origin in metres, including height. `grid.columns`
and `grid.spacingM` generate deterministic fixed robot positions. NR robots are
placed first, followed by Wi-Fi robots. APs and gNBs are placed two metres behind
the origin, three metres above its height, with two-metre spacing if multiple
devices are requested. These are illustrative placements, not calibrated coverage.
Robots are assigned to their zone's corresponding access devices round-robin.
The resolved inventory records every robot's ID, access, attachment and position.
Radio channels, frequencies, scheduling, traffic and fabric wiring are not defined
here; the runner integration must specify and validate them explicitly.

From the repository root, using Python 3:

```sh
python scripts/wireless_layout.py configs/warehouse_wireless_layout.json
python scripts/wireless_layout.py configs/warehouse_wireless_layout.json --output wireless_inventory.json
python scripts/wireless_layout.py configs/warehouse_wireless_small.json
python tests/test_wireless_layout.py
```

Use `python3` where that is the Python command. The first command validates and
prints counts; the second creates the fully resolved inventory and refuses to
overwrite an existing file. The small example has two zones, eight robots and
21 total devices. No simulator or toolchain is needed for these commands.
Unknown fields, duplicate zone names, invalid counts, nonfinite positions,
movement, and robots without corresponding radio infrastructure are rejected.
The 4096-device input ceiling is a validation limit, not demonstrated radio capacity.

## Native construction control

`src/wireless-construction.cc` now consumes the validated inventory exported by
`scripts/prepare_wireless_nodes.py`. It creates real NR/Wi-Fi devices and the
EPC helper's three core nodes. The two-zone control ran with 21 actual nodes;
all four NR robots reached CONNECTED_NORMALLY at their configured gNB and all
four Wi-Fi robots associated with the specified AP. Repeated device CSVs matched.
This is construction/attachment evidence only: switches, spines and edge nodes
are not yet linked, and no application traffic or routing measurement is produced.

In the prepared Linux ns-3.46/NR v4.1.1 environment, from the project directory:

```sh
python3 scripts/prepare_wireless_nodes.py configs/warehouse_wireless_small.json /tmp/wireless-nodes.txt
cp src/wireless-construction.cc /path/to/ns-3/scratch/warehouse-wireless-construction.cc
cmake --build /path/to/build --target scratch_warehouse-wireless-construction -j 2
/path/to/ns-3/build/scratch/ns3.46-warehouse-wireless-construction-relwithdebinfo --inventory=/tmp/wireless-nodes.txt --output=/tmp/wireless-audit.csv
```

Choose new output filenames for repeats. Native node-table input is an internal
format; use the validating exporter rather than editing it. The control requires
at least one NR UE and gNB. The native target is separate from the wired project
build. Radio settings are currently fixed construction defaults: NR 3.5 GHz,
20 MHz, numerology 1, gNB 20 dBm, ideal direct-path beamforming, UMi channel with
shadowing disabled; Wi-Fi 802.11ax channel 36/20 MHz, HeMcs0. Both technologies
share their respective channel across zones. These are illustrative settings,
not a calibrated indoor warehouse or a latency guarantee. NR attachment is
explicit; the Wi-Fi SSID identifies each intended AP. Simulation duration is 1 s.
Full fabric wiring, IP routes, traffic and packet/path audits remain next.

## Fabric integration control — 6 November

The current native source now also includes `wireless-fabric.h`; copy that header
alongside the scratch source before compiling. The earlier construction-only
result above remains historical. Current execution is three seconds and includes
16 flows for the small example, each offering ten 256-byte UDP packets at 20 ms
intervals, starting after 1.2 seconds. Each robot sends to an alternating edge
server and the next robot in deterministic ID order.

Every leaf connects to all spines. Each AP/edge connects to its first zone leaf;
the EPC PGW connects to both spines. Point-to-point fabric links use 1 Gbps and
100 microseconds. gNB-to-core links remain those supplied by the EPC helper;
they do not traverse the zone leaf. Infrastructure nodes route IP rather than
modeling Ethernet switches. Static host routes use minimum hop count with
deterministic tie selection, not ECMP or the wired size-aware Dijkstra policy.
Wi-Fi has one /24 per AP, and NR uses helper-provided UE addressing/routing.

First complete run: 153/160 received; 15 flows delivered 10/10, one zone-A
NR-to-Wi-Fi flow delivered 3/10. This is a failed all-delivered control, not a
validated end-to-end warehouse. Output is retained before returning failure.
Packet-level tracing must establish the missing-packet cause; do not assume
radio loss, silently discard those packets, or relax the acceptance result.
No independent per-hop audit, ECMP, resilience, calibration or 500-device run yet.

### ARP loss diagnosis

An unchanged-workload diagnostic traced exactly seven ARP-layer drops at
zone-A-ap-0: source 7.0.0.3, destination 192.168.0.2, UDP port 9003,
sequence IDs 3..9 at 1.26837..1.38837 s. The address-resolution reply was
received around 2.20916 s after a retry. The default pending capacity is three.
This accounts for the 3/10 delivery in that flow; the cause of the initial
unsuccessful ARP exchange itself still needs MAC/PHY tracing.

A separate counterfactual run with command-line
`--ns3::ArpCache::PendingQueueSize=10` delivered 160/160. This is a diagnostic,
not the default or a latency acceptance result. Buffering until ARP resolves
can impose approximately one second of waiting. No deadline claim follows
from full delivery. Default behavior and the original failed data are retained.
The native source now emits ARP drop records with time, node, addresses, port
and sequence number. Logs are in `.recovery/fabric-drop.log`,
`.recovery/fabric-arp.log` and `.recovery/fabric-arp-capacity10.log` locally.
