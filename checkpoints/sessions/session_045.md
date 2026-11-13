# checkpoint_049 — Explicit outcome measurement validated

Added scripts/audit_access_outcomes.py and --outcome-measurement to the maintained
runner. Native exits0/2 are provisionally allowed only in this explicit mode;
original and replay must match byte-for-byte, with consistent task accounting,
authorization ordering/timeout/late replies, deadlines, server boundaries, outage
schedule and route restoration. Existing coverage and ECMP audits remain applied.
Crashes, other exit codes, wall timeouts and audit mismatches still fail.
Default strict delivery and separate boundary-control behavior are unchanged.
Boundary and measurement flags are mutually exclusive; ordinary seed-comparison
wrapper remains strict. No native/radio/dependency changes or scale guard changes.

Successful mode status is MEASURED with validation PASS and separate nativeExit,
deliveryControlPassed,allTasksDelivered,allDeadlinesMet. This is not all-delivered
PASS. All offered tasks remain in deadlines, including denied/blocked/unreceived.
Payload unreceived by stop is not automatically called an observed radio drop.
Expected deny/timeout controls may satisfy native blocking contract without payload
completion; explicit allTasksDelivered stays false.

Six real20-node controls, each with exact replay:
- allow:36/36 delivered/deadlines, native0
- deliberately late approval:30/36 delivered/deadlines,6 timed out with ignored
  replies and no payload, native2 retained
- deny:30/36 delivered/deadlines,6 denied, native0 expected blocking contract
- silent server:30/36 delivered/deadlines,6 timed out, native0 expected contract
- outage boundary:43/46 delivered/deadlines,2 auth timeouts+1 observed payload drop,
  native2 retained
- static direct-zone link/no outage:3/3 delivered/deadlines, native0

Results and final re-audit receipt: .recovery/access-outcomes/validation-summary.json.
Healthy allow and boundary traces/CSVs exactly match prior known-good controls.
Existing strict allow/static and boundary auditors also pass; strict delivery
rejects the late-response fixture. Native late-response fixture retained with
provenance.71 access tests and21 core checks PASS, including corrupt evidence,
hidden misses, missing/duplicate tasks, unapproved/late payload and replay changes.
The20ms authorization timeout is confined to the dedicated late-response test;
the300-device1000ms timeout and100ms task deadlines remain unchanged.

300-device evidence remains checkpoint048:905/906 delivered,901/906 deadline-met;
ARP diagnostic timed out and only its prefix matched. No new300-device run or
completed diagnosis is claimed.200-device case remains last larger strict PASS.
No active process. No Git commit/push. Source checkpoint_049 and separate
checkpoint_049-outcomes.zip recovery package. Read docs/access-outcome-measurement.md.

Next exact action: Run configs/access_300_device_probe.json with --scale-control --outcome-measurement to obtain a complete native replay and outcome audit; preserve old FAILED results. Then finish focused ARP/MAC diagnosis with bounded Linux-local logging instead of broad logging to mounted Windows files. Do not warm caches, relax deadlines/auth timeout, or claim500-device readiness.500-device validation, multi-seed measurement, visualization/report integration remain unfinished.
