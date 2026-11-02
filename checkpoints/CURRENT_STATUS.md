# checkpoint_022 - zone-switch isolation and recovery

Read checkpoint021 state/next/session, verified clean Git and source manifest, and read docs/warehouse-study.md. Added --zone-outage ZONE to the layout runner, mutually exclusive with --spine-outage. A selected single-leaf zone fails at30ms and recovers at70ms. Existing spine mode and default configuration preserved. No native implementation or queue-policy changes.

Batch layout-20261102T171806Z-fe0a210a PASS, all-robot 500-device topology. Both native runs independently audited; all seven replay artifacts matched exactly. 2410 offered,2252 delivered,158 no_route losses counted as deadline misses. Affected:59 Zone A source flows plus20 incoming cross-zone flows. Before failure:158/158 affected and806/806 unaffected delivered. During outage:0/158 affected and806/806 unaffected delivered. Recovery:79/79 affected and403/403 unaffected delivered. Source-time route rejection does not imply retransmission after recovery.

Checks:25 native build tests,21 final core tests,6 layout tests,2 zone-outage tests PASS. Export independently re-audited raw data. Browser via localhost verified packet2 at40ms no_route with Zone A switch down, packet4 at80ms delivered with all devices available, and Zone B packet297 delivered at40ms while Zone A is down. Screenshot saved with batch. Previous full100 native/full132 core regressions remain historical evidence and were not rerun for this orchestration-only change.

OPEN_WAREHOUSE.html links the new viewer; START_HERE and docs/zone-outage.md explain reproduction and sample selection. Complete raw/replay/provenance data included by package_delivery.py. Exact final archive and extraction checks are recorded in recovery_packages/receipt-checkpoint_022-delivery.json; successful packaging requires that receipt. Prior archives/manifests/results retained.

Limitations: powered infrastructure can still fail; one leaf per zone remains a single point of failure. Immediate centralized failure knowledge, spaced samples, no hardware convergence claim, no radio, light illustrative workload, no capacity/deadline guarantee. Mac M4 / Ubuntu ARM64 execution remains untested. EALR legacy and protected-QoS experimental scope unchanged.

Source manifest: manifests/checkpoint_022.json. Next: rehearse the complete offline delivery and verify the final extraction/start guide; prioritize handoff reliability before optional extra scenarios. Target-platform runtime testing requires access to that host.
