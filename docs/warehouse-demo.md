# Configurable warehouse demonstration

Edit `configs/warehouse_layout.json`. The number of entries in `zones` is the number of zones; add/remove entries rather than maintaining a second conflicting count. Each entry explicitly sets `robots`, `switches` and `edgeServers`. `sharedSpines` controls the common backbone.

The supplied layout has 482 robots, 8 switches, 8 local edge servers and 2 shared spines: exactly 500 devices. Robot counts A–H are 59, 61, 58, 62, 60, 63, 59, 60. These are intentionally fixed, mildly uneven counts, not newly randomized every run. All devices are stationary and all links are wired.

Every switch connects to every shared spine. Robots attach to one switch in their zone. Local edge servers also attach to a zone switch. When a zone has multiple switches, robots and servers are distributed round-robin. A same-zone path can then cross the shared backbone when its endpoints attach to different switches. A single switch per zone gives the simple local path. Robot attachment and local switch failure remain single points of failure.

The generator uses the schema-v2 graph topology because local server-to-switch links differ from the earlier strict leafSpine template, whose servers attach to spines. This is intentional; native IP forwarding is unchanged. These devices model wired IP routers, not radio access points or Ethernet switching protocols.

After following `docs/first-run.md` and installing the wired ns-3 dependency, run from the repository root:

```sh
python scripts/run_warehouse_layout.py
```

On systems where Python is named python3, substitute python3. An external ns-3 installation can be selected with `--ns3-prefix /path/to/install`. The runner verifies/builds the native backend, generates the graph, saves input and source provenance in a unique results directory, runs the simulation and replay, and independently audits both. It prints PASS only after all checks succeed. Native cases have a configurable 600-second timeout; compilation is separate.

The demonstration sends three representative flows from the first robot of each zone: a local peer, a robot in another zone and the local edge service with remote backups (at most 16 ordered service candidates, including local servers). With the supplied layout this is 24 flows and 120 offered packets over 100 simulated milliseconds. All 500 devices exist, but this is not a simultaneous 482-robot load experiment. Missing peers/services omit that traffic scope. Queues use strict priority and routing uses ECMP; this demonstration is not a multi-seed policy study.

To export only the explicit graph for further editing:

```sh
python scripts/warehouse_layout.py --layout configs/warehouse_layout.json --output my_warehouse.json
```

The exporter refuses to overwrite an existing file. The generated `configs/warehouse_8zone_500.json` is a reference snapshot; the run command always regenerates from your editable count file. Use the native executable with an exported schema-v2 file to customize traffic, deadlines, routing or failures. Packets, actual paths, queue events and summary results are written to the batch's run directory.

No delayed detection, NR or wireless behavior is claimed. Actual macOS/Ubuntu ARM64 execution remains untested here.

## Executed validation

Windows native batch `layout-20261101T203251Z-838ca5b1` passed independent audit and exact replay: 120/120 delivered, zero observed deadline misses. The first native process took 107 seconds here. A two-zone layout with 11 devices, multiple switches and one zone without a local edge also passed (30/30 delivered), batch `layout-20261101T203641Z-425a1ee4`. These are deterministic demonstration results, not capacity or deadline guarantees.


## Offline topology and packet-path viewer

Open `OPEN_WAREHOUSE.html` in the project root and click **Open the working topology viewer**. Alternatively, open `results/viewer-checkpoint015/warehouse-viewer.html` directly. The file in `scripts/warehouse_viewer.html` is only a source template. The file embeds the audited 500-device topology and packet records; no server, Internet connection or ns-3 installation is needed to view it. The simulation itself still needs its documented dependencies.

Select a traffic flow and recorded packet. Green links show its observed path; Next hop/Previous hop step through that path. Show full path restores the complete route. Show all robots hides or reveals inactive endpoints without removing them from the simulation. Click a node for identity and configured coordinates. Receive timestamps are taken from native hop traces. The arrangement is schematic, not a warehouse floor plan, and stepping is not time-scaled animation.

To generate a new viewer from a completed layout batch:

```sh
python scripts/visualize_warehouse.py --batch results/YOUR_LAYOUT_BATCH --output warehouse-viewer.html
```

Use the batch directory printed by `run_warehouse_layout.py`. Export reruns the independent raw audit and refuses incomplete batches, inconsistent traces and existing output files. It supports the layout runner's config.json/run structure; research-suite batch structures require a separate adapter. It displays recorded outcomes only: it cannot simulate a newly selected failure. The next demonstration milestone is an actual spine-outage run, followed by exporting its observations.


## Spine failure and recovery demonstration

```sh
python scripts/run_warehouse_layout.py --spine-outage
```

This keeps the configured layout and traffic, takes the second shared spine down at 30 ms and restores it at 70 ms. At least two spines are required. The runner independently audits both executions, requires observed alternate-spine routing, compares replay artifacts and writes `run-outage.json` and `replay-outage.json`. A layout whose offered flows never use that spine is rejected as an uninformative rerouting demonstration.

After a successful run, export its viewer using the command above. Select a cross-zone flow and compare packets generated at 20, 40 and 80 ms. The availability section lists the event times; the failed node is red and marked DOWN for packets generated during the outage. It returns to normal for recovered samples. The display uses recorded paths; it does not invent a route when you click.

This is a deterministic demonstration with immediate centralized failure knowledge and route updates. Packets are spaced away from failure/recovery boundaries. Success does not establish a hardware convergence time or loss-free behavior for packets already in flight at failure. Those boundary cases are covered separately by the earlier native controls, not by this presentation workload.


Validated outage batch: `layout-20261102T065135Z-da39e8c6`. All 120 packets arrived (48 before, 48 during and 24 after recovery); six cross-zone flows changed spine and all 40 local-peer packets remained within their zones. Independent audit and exact replay passed. Open the failure demo from OPEN_WAREHOUSE.html, select A-cross-zone and compare #6 at 20 ms, #7 at 40 ms and #9 at 80 ms.


## Optional traffic from every robot

The original delivered example stays unchanged. Use the separate `configs/warehouse_layout_all_robots.json` profile:

```sh
python scripts/run_warehouse_layout.py --layout configs/warehouse_layout_all_robots.json
```

Its optional `traffic` object is:

```json
{"mode":"allRobots", "scope":"mixed", "intervalMs":20, "bytes":256, "priority":1, "serviceCandidates":1}
```

`representative` remains the default when this object is absent. `allRobots` creates exactly one flow from each robot. `mixed` rotates through eligible same-zone, cross-zone and edge-service destinations in deterministic robot order; it does not create all three flows from every robot. Choose `same-zone`, `cross-zone` or `edge-service` to restrict the destination scope. A robot without an eligible destination in allRobots mode is an error, not silently omitted.

The 500-device profile has 482 robot sources: 163 local-peer flows, 161 cross-zone flows and 158 service flows. Each offers five 256-byte UDP payloads at 0, 20, 40, 60 and 80 ms: 2,410 offered packets. All robots offer packets on those same timestamps. This is functional all-source coverage, not a saturated-capacity test. Native packet and queue audits still determine actual delivery and losses.

`priority` accepts 0 through 3. `bytes` accepts 1 through 1472 for unfragmented UDP. `intervalMs` is finite, between 0.001 and 100; native flow/packet limits also apply. `serviceCandidates` accepts 1 through 16. All-robot mode defaults to one service candidate to keep route initialization bounded; explicitly select two or more if testing service backups. The representative default retains its prior ordered candidate list.

The runtime now omits unused per-flow route entries on endpoints other than the originating endpoint. Endpoints never relay traffic; destination-local delivery is unchanged. Full native regression reproduced all 700 preserved artifacts; the 500-device baseline and outage reproduced all 14 artifacts. Routing costs and simulated link timing are unchanged. The independent audit also caches pure shortest-distance calculations within each immutable availability snapshot; all observational checks remain active.

Validated all-robot batch: `layout-20261102T121400Z-1b3f3906`, from a fresh Windows source build. All 2,410 packets arrived, with five packets from each of 482 sources, zero deadline misses, mean latency 0.317991 ms and maximum latency 0.480080 ms. Independent audit and exact replay passed. The prior uncached audit produced the same seven raw artifacts. Native process durations were approximately 26-44 seconds across these executions; build and audit time are additional, and host load varied. These finite-run observations are not capacity or deadline guarantees. The delivery menu includes this viewer.
