# checkpoint_050 — 300-device measurement and wireless diagnosis

Completed 13 November 2026. No active simulation from this worker sequence.
Read docs/access-300-diagnosis.md for the timeline, task table and limitations.

Result results/access-auth-20261113T101202Z-025cb365 is MEASURED, validation PASS:
905/906 delivered,901/906 deadline-met; one authorization timeout with no payload,
four late deliveries. Native exit2 and deliveryControlPassed=false are retained.
Original/replay byte-identical and identical to the earlier failed300-device run.
Native times894.978/838.432s, within1200s each. Single seed1/run1, not a seed study.
ECMP audit:8694 hops,2020 equal-cost selections,1596 restored flow mappings.

Isolated diagnostic smoke and full300 trace/CSV are byte-identical to their known
references; library hashes match. Full diagnostic658.246s. ZoneA/H initial AP ARP
requests to Wi-Fi robot0 drop with BUSY_DECODING_PREAMBLE while another ARP frame
is received. Retries resolve the mappings and flush queued traffic after about
1000.288/973.252ms respectively. This explains the observed late approval/payload
timing. Task16 correctly ignores approval after timeout. Modeled cause only;
not calibrated real warehouse radio performance. No native model, deadlines,
authorization timeout, caches, dependencies or scale limits were changed.

Evidence: .recovery/access-300-resume/measurement-receipt.json,frame-analysis.json,
diagnostic-build-receipt.json,diagnostic-smoke/receipt.json,
focused-diagnostic/receipt.json and raw logs. Worker job-status.json is COMPLETE.
Prior interrupted attempts remain in .recovery/access-300-measured and their
original receipts are preserved. Earlier timeout evidence/results remain intact.

Source regression tests remain71 access/21 core PASS from checkpoint049, not
rerun for this documentation/evidence milestone. New validation is actual native
300 measurement/replay, isolated smoke and full diagnostic equivalence.
Source checkpoint050; separate recovery archive checkpoint_050-300-diagnosis.zip.
No Git commit/push.500-device wireless validation, multiple seeds, visualization
and report integration remain unfinished.200 devices remains the last larger
strict all-delivered/deadline-passing probe.

Next exact action: Predeclare a controlled ARP wait/retry sensitivity study against the unchanged 300-device baseline, then implement an explicit experiment parameter and validate small controls before expensive multi-seed runs. Keep100ms deadlines,1000ms authorization timeout and all-offered denominators. Do not silently prewarm caches or bypass approval. Review native/Python device and task limits before500-device work;500 is unvalidated.
