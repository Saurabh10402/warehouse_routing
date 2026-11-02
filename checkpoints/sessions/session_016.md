# Additional-day traffic extension in progress

User authorized extra features after preserving the delivered checkpoint018 ZIP. Read current state/next steps/latest session and verified checkpoint018; Git clean at start. Added optional traffic object to layout JSON: representative default or allRobots, scope, intervalMs, bytes, priority and serviceCandidates. All-robot mode generates one flow per robot; mixed scope deterministically rotates eligible local/cross/service destinations. Default config remains exactly equal to delivered schema-v2 snapshot. Six generator tests PASS.

Runtime optimization: skip preparing per-flow routes on endpoint nodes other than the originating endpoint. Endpoints cannot relay packets; destination-local delivery is unchanged. No changes to costs, ECMP hash, queues, receive gates, traffic schedule or native trace schema. Full 100-case native regression plus every replay and full132 core baseline regression are running. Before claiming optimization valid, compare all 700 native artifacts against checkpoint012 and 500-device baseline/outage artifacts against their delivered batches.

Next: finish regressions, run all-robot 500-device native workload and replay, record actual process duration/delivery/latency/active-source coverage, test a clean build, export viewer, checkpoint/recovery. Native batch currently ns3-warehouse-20261102T115922Z-7cd3bc9c. No capacity or convergence guarantee, no target-host execution claim. Prior delivery ZIP unchanged.


Runtime findings and follow-up: native original baseline now 6.313 s and outage 14.734 s, with all 14 artifacts byte-identical. Initial all-source native execution delivered 2410/2410 in 26.016 s; full audit/replay still pending. The independent Python audit was recomputing identical pure distances; added an invocation-local cache keyed by immutable availability snapshot identity, source, target and payload. All packet/queue/event/deadline checks remain, and a second full native100 suite runs with the cached audit (ns3-warehouse-20261102T121203Z-249ce94e). Initial uncached native100 suite already PASS, 700 artifacts identical to checkpoint012. Core132 batch experiments-20261102T120006Z-a802bc3c PASS including baseline/replay/plots.

Clean source compiled and passed25 tests, then failed before batch creation because results/ did not exist. Fixed runner batch.mkdir(parents=True,exist_ok=False). Retained original preparation receipt .recovery/checkpoint019-fresh.json and recorded final source receipt .recovery/checkpoint019-fresh-final.json; same freshly compiled C++ source, Python audit/runner updated. Rerunning clean workflow in .recovery/traffic clean d7a3dcc7. Do not confuse the first failed pre-batch startup with a successful experiment.

## Completed milestone


User authorized an additional day of features after checkpoint018 delivery. Added an optional traffic object and separate all-robot profile, preserving the original representative configuration exactly. The 500-device topology now supports 482 active robot sources, configurable scope, interval, payload, priority and service candidates. Viewer and delivery menu include the audited all-robot run.

Validated fresh Windows source build: 25 tests PASS. Final native regression ns3-warehouse-20261102T121634Z-4c776a9c: 100 cases, every replay (200 processes), all 700 artifacts identical to checkpoint012. Full core regression experiments-20261102T120006Z-a802bc3c: 132 cases PASS. Large baseline/outage regression: all 14 artifacts identical. Generator tests: 6 PASS. Verification logs and comparisons are in evidence/checkpoint_019.

Fresh all-robot batch layout-20261102T121400Z-1b3f3906: 2410/2410 delivered, 482 sources, five packets each, zero deadline misses, mean 0.317991 ms and max 0.480080 ms. Independent audit and exact replay PASS; all seven artifacts match the original uncached run layout-20261102T120343Z-c17f3ded. Browser verified 482 flow options and both cross-zone and last-zone service paths without console errors through localhost. Direct file:// browser execution was not tested.

Runtime improvements omit unused non-origin endpoint route entries and cache pure reference-distance calculations per immutable availability snapshot. No routing cost, queue, arrival or trace semantics changed. Fresh-run missing results parent was found and fixed. Intermediate native batch ns3-warehouse-20261102T121203Z-249ce94e is FAILED because source changed during execution; retained, excluded from delivery PASS evidence, superseded by the final frozen-source regression.

Source commits c5c6887 and fb60dad. Source manifest: manifests/checkpoint_019.json. Final package identity and extraction verification belong in recovery_packages/receipt-checkpoint_019-delivery.json. Previous ZIP and manifests remain intact. Main/fresh native process times are observations under differing host load, not controlled speed benchmarks.

Limitations: synchronized illustrative light workload; no saturation capacity guarantee, no radio/NR, no hardware convergence model. Mac M4/Ubuntu ARM64 execution remains untested. Next action: controlled Dijkstra-versus-ECMP comparison using identical all-robot arrivals and explicit measured outcomes, while retaining this working delivery.

Packaging follow-up checkpoint020: expanded regression archive hit Windows path length on extraction. Shortened verification directory and documented a short extraction root. Previous failed archive/extraction retained; final receipt020 is authoritative. No simulator changes.
