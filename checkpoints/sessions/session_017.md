# All-robot routing comparison in progress

Read checkpoint020 state, next steps and latest session; verified manifest and clean Git before edits. User requested continuation. Predeclared one Dijkstra seed1 control and ECMP seeds1/2/3, each with exact replay; only algorithm/hash seed vary. Default all-robot profile: 500 devices, 482 sources, 2410 packets, strict-priority queues, synchronized arrivals. Metrics: offered/delivered, deadline misses, delivered mean/nearest-rank P95/max latency, per-scope outcomes, observed per-spine packet and distinct-flow counts. Seeds test hash sensitivity, not stochastic arrivals. No presumption of ECMP improvement. Reusable comparison runner preserves inputs, native library/executable/source provenance, raw traces, independent audits and process receipts. Source and input drift fail the batch.

Next: native build/tests, all four cases and replays, inspect measured outcomes, deliver offline comparison table and checkpoint. No simulator changes.

## Completed comparison


User requested continuation. Read checkpoint020 state, next steps, session017 and docs/warehouse-study.md. Manifest mismatch was exactly the two pending comparison source/test files from the interrupted turn; no delivered source changes were discarded. Host recovered; no old native processes remained and no earlier comparison batch existed. Updated user AGENTS instructions govern work.

New reusable scripts/compare_warehouse_routing.py declares Dijkstra seed1 and ECMP seeds1/2/3 on identical 500-device/482-source traffic, with strict-priority scheduling fixed. This is a routing-only subset, not replacement of the existing FIFO/strict-priority studies. All inputs/metrics saved before runs; source and input drift checks, native provenance, bounded native processes, independent audit and exact replay included. No simulator or queue-policy changes.

Batch routing-20261102T152642Z-82d74709 PASS: four cases, eight native executions, seven matching replay artifacts per case, identical arrival signatures. Every case delivered 2410/2410 with zero deadline misses. Dijkstra used spine0/spine1 805/0; ECMP seeds1/2/3 used 435/370, 390/415, 375/430. Mean delays respectively 0.321698, 0.317991, 0.317948, 0.318024 ms. All seven ECMP seed1 artifacts exactly match the previously delivered all-robot run. Three focused comparison tests and 25 native build tests PASS. Final core/package verification recorded separately in checkpoint evidence. Previous full100 native/full132 core studies were not rerun: no simulator change.

Browser verified all four rows and report layout using localhost; saved comparison-preview.png. OPEN_WAREHOUSE.html links the report; START_HERE and docs/routing-comparison.md explain reproduction and limitations. scripts/package_delivery.py includes complete raw comparison data. Final delivery validation and exact ZIP identity are in recovery_packages/receipt-checkpoint_021-delivery.json; require that receipt before calling packaging complete. All older archives/manifests/results retained.

Scope: synchronized illustrative light traffic, hash sensitivity only, delivered-packet latencies and loss-inclusive denominators. Better spine distribution and slightly lower observed latency are not capacity/deadline guarantees. No radio or hardware convergence model; Mac M4 / Ubuntu ARM64 execution remains untested.

Source manifest: manifests/checkpoint_021.json. Next action: add a reproducible failure scenario demonstrating the remaining single-zone-switch failure point and recovery, retaining existing delivery and immediate centralized-update caveat. Keep EALR legacy and protected QoS experimental.
