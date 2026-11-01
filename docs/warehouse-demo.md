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
