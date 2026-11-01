# Two-day delivery: configurable 500-device wired demonstration

User superseded the research roadmap: deliver a working routing simulation in configurable desired topology. Confirmed about 500 total devices; shared-spine arrangement left to engineering judgment. Delayed-control contract, NR and native protected-queue work deferred.

Read checkpoint 013 status/state/next steps/latest session; Git clean and manifest verified. Implemented explicit count JSON, deterministic graph exporter and audited native run/replay launcher. Example: 8 zones, robots 59/61/58/62/60/63/59/60 (482), 8 switches, 8 local edges, 2 shared spines = 500. Local edges attach to switches; graph schema deliberately used instead of restrictive legacy leafSpine template. 506 links. 24 representative flows, not simultaneous all-robot traffic.

Core verification 21/21 PASS, native verification 25/25 PASS, three generator tests PASS. Native 500-device run/replay batch layout-20261101T203251Z-838ca5b1 currently RUNNING; do not claim packet/audit success until complete. Large topology route initialization is being measured; source behavior is unchanged. Final documentation/runtime provenance additions are not yet represented by that initial batch's source hash.

Next: finish native validation, inspect delivery and actual paths, validate final runner, checkpoint final source and preserve generated raw results separately. Target Mac/Ubuntu execution remains untested.


# checkpoint_014 - configurable warehouse demonstration

User narrowed two-day delivery to a working wired routing simulation. Eight-zone reference: 482 robots + 8 switches + 8 local edge servers + 2 shared spines = 500 devices, 506 links. Edit configs/warehouse_layout.json; its zone array and explicit counts control generation. Run python scripts/run_warehouse_layout.py. See docs/warehouse-demo.md.

Native batch layout-20261101T203251Z-838ca5b1: 120/120 packets delivered, zero observed deadline misses; independent audit and all seven replay artifacts PASS. This is 24 representative flows, not all robots transmitting or a multi-seed evaluation. First process elapsed 107 seconds on this Windows host; target-platform speed is unknown. Alternate two-zone/11-device layout layout-20261101T203641Z-425a1ee4: 30/30 delivered, independent audit and replay PASS. Core 21/21 and native 25/25 verification tests PASS; three generator tests PASS. Existing C++ backend unchanged. Full historical study was not rerun for this generator/launcher change.

The final launcher adds generic scope metadata and runtime library/platform/input provenance after the first 500-device run; final launcher was exercised on the alternate layout. Original raw batches retain exact tested-source provenance. Documentation was completed afterward. No old manifest or result was replaced.

Delayed detection, native protected queues/safety and NR are deferred by the user's delivery scope. Existing immediate outage behavior remains. Actual Mac/Ubuntu ARM64 execution remains untested. Next: prepare a presentation-ready topology/path view and streamlined handoff for this validated wired demo; define additional traffic only if needed for submission. Source manifest: manifests/checkpoint_014.json.
