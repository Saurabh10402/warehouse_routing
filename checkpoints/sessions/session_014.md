# Spine outage demonstration in progress

Verified checkpoint 016 and clean Git before work. Added --spine-outage to the layout runner: second shared spine down at 30 ms, up at 70 ms. Retains the baseline 500-device topology and 24 flows/120 packets. Native backend unchanged. Added explicit pre/during/post route report and observed rerouting acceptance; export independently audits all traces as before. Viewer marks physically unavailable nodes at selected packet generation time and lists events; stepping is not timed playback.

Native CTest 25/25 PASS. Batch results/layout-20261102T065135Z-da39e8c6 is running; do not claim success until both simulation, audit and replay finish. Process timeout remains 600 seconds per execution. Final report adds local-peer packet count after launch; final reporting code must be independently run against completed data before finalizing. Source hashes in original provenance remain untouched.

Next: finish batch, verify same-zone preservation and before/alternate/recovered paths, export viewer, inspect it in browser, run core verification/export integrity tests, finalize checkpoint017 and recovery package. No radio, full-load or convergence claim.


# checkpoint_017 - 500-device spine failure/recovery validated

Batch: layout-20261102T065135Z-da39e8c6. --spine-outage keeps the eight-zone/500-device layout and 24 flows; shared-spine-1 fails at 30 ms and recovers at 70 ms. Native backend unchanged. All 120 packets delivered: 48 before, 48 during, 24 after recovery. Six cross-zone flows switched to shared-spine-0 and returned to their original route. All 40 same-zone peer packets avoided spines. Independent native audits and all seven raw replay artifacts PASS. Final report independently matches replay. First native execution took 239.688 seconds on this Windows host.

Viewer: results/layout-20261102T065135Z-da39e8c6/warehouse-viewer.html. Added node availability at packet generation and event list. Browser checked A-cross-zone packet #7 at 40 ms: DOWN spine 1, observed route via spine 0; packet #9 at 80 ms: restored spine 1. No JavaScript console errors. Screenshot in outage-preview.jpg. OPEN_WAREHOUSE.html links baseline and outage viewers, with packet selections.

Validation: native CTest 25/25, core CTest 21/21, exporter integrity tests 3/3 and outage guard tests 3/3 PASS. Read checkpoint016/current state/latest session and verified its manifest before work. Only scenario orchestration/reporting/viewer/docs changed; full historical studies were not rerun. Original batch provenance records source at launch; final report count and final viewer were completed afterward without altering raw native traces. Source manifest: manifests/checkpoint_017.json.

Limits: representative traffic, deterministic replay, immediate centralized route updates, packets spaced away from event boundaries. Not measured control-protocol convergence, full-load reliability, radio/NR or actual Mac/Ubuntu execution. Prior native event-boundary controls remain separate evidence. No user changes or old results removed.

Next exact action: produce the consolidated delivery package with both viewers, raw evidence, pinned dependencies/setup instructions, clear launcher and short submission walkthrough. Validate extraction and links; keep target-platform execution unclaimed until actually run. Advanced research remains deferred under the two-day delivery scope.
