# checkpoint_020 - configurable traffic from every robot

User authorized an additional day of features after checkpoint018 delivery. Added an optional traffic object and separate all-robot profile, preserving the original representative configuration exactly. The 500-device topology now supports 482 active robot sources, configurable scope, interval, payload, priority and service candidates. Viewer and delivery menu include the audited all-robot run.

Validated fresh Windows source build: 25 tests PASS. Final native regression ns3-warehouse-20261102T121634Z-4c776a9c: 100 cases, every replay (200 processes), all 700 artifacts identical to checkpoint012. Full core regression experiments-20261102T120006Z-a802bc3c: 132 cases PASS. Large baseline/outage regression: all 14 artifacts identical. Generator tests: 6 PASS. Verification logs and comparisons are in evidence/checkpoint_019.

Fresh all-robot batch layout-20261102T121400Z-1b3f3906: 2410/2410 delivered, 482 sources, five packets each, zero deadline misses, mean 0.317991 ms and max 0.480080 ms. Independent audit and exact replay PASS; all seven artifacts match the original uncached run layout-20261102T120343Z-c17f3ded. Browser verified 482 flow options and both cross-zone and last-zone service paths without console errors through localhost. Direct file:// browser execution was not tested.

Runtime improvements omit unused non-origin endpoint route entries and cache pure reference-distance calculations per immutable availability snapshot. No routing cost, queue, arrival or trace semantics changed. Fresh-run missing results parent was found and fixed. Intermediate native batch ns3-warehouse-20261102T121203Z-249ce94e is FAILED because source changed during execution; retained, excluded from delivery PASS evidence, superseded by the final frozen-source regression.

Source commits c5c6887 and fb60dad. Source manifest: manifests/checkpoint_020.json. Final package identity and extraction verification belong in recovery_packages/receipt-checkpoint_020-delivery.json. Previous ZIP and manifests remain intact. Main/fresh native process times are observations under differing host load, not controlled speed benchmarks.

Limitations: synchronized illustrative light workload; no saturation capacity guarantee, no radio/NR, no hardware convergence model. Mac M4/Ubuntu ARM64 execution remains untested. Next action: controlled Dijkstra-versus-ECMP comparison using identical all-robot arrivals and explicit measured outcomes, while retaining this working delivery.

Packaging follow-up: the expanded regression archive initially exceeded Windows path limits during fresh extraction. Retained that unsuccessful archive/extraction; shortened the packaging verification directory and documented short Windows extraction roots. Checkpoint019 manifest remains immutable. No simulator changes.
