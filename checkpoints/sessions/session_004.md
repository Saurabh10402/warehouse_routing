# Session 004 — capacity/admission study and resumed finalization

User authorized the next study, then requested continuation after the experiment wait.
Read checkpoint_006, verified its manifest and clean worktree before implementation.
Added emergencyPolicingEnabled (true by default), with reservation/service preserved when
false. Tests cover inactive burst size, invalid boolean JSON, reservation retention and
lower-class starvation under unpoliced P0 backlog. Prespecified the 288-run matrix before
execution and committed source as 8fe3f31. Old behavior stays byte-identical in both regressions.

Initial sandbox Python temp-file tests failed on filesystem permissions; the identical
verification completed outside the sandbox with authorization: 19/19 both presets.
The full capacity batch and both regression batches completed during the wait. On resume,
checkpoint_006's mismatch was investigated: all ten changed source paths belong to committed
8fe3f31, the worktree was clean, and source exactly matched batch provenance. No user changes
were discarded. Final edits only add documentation and recovery evidence.

288 new runs PASS, 144 protected and 132 warehouse regressions match exact inputs/packets.
60/40/55 charts and replay PASS; flood-c5 scope/class chart visually inspected. Independent
raw recomputation verifies all 96 operating rows including all class means/worst/SD/delivery
and all admission losses. Report preserves both improvements and adverse results.

No burst/flood setting eliminates P0 misses. Mixed at 2 Mbps demonstrates zero P0 misses
with reservation but continued P3 failures; unpoliced flood can starve lower traffic.
Fixed 20% policing at 5 Mbps flood averages 46.6875% P0 misses. No default was changed.

Next: recorded ns-3 dependency build repair, minimal real packet runtime, then schema-v2
forwarding/QoS integration and NR. Existing external blocker remains; no new build attempted.
Keep the calibrated physical-input and cross-platform limitations explicit.

## Handoff requirement reaffirmed — 31 October 2026

User reiterated that they cannot execute/debug the project now and need a well-finished
product for later use. Re-read the original target: MacBook Air M4 / Ubuntu 24.04 in UTM.
Recorded explicit handoff acceptance checks in PROJECT_STATE.handoff_requirements.
Source checkpoint_007 still verifies; this update changes recovery metadata only.
Do not substitute Windows-only verification, a roadmap, or undocumented manual fixes for
finished target-platform setup and runtime validation. Do not promise zero future issues;
report exact evidence and remaining unverified integration/platform requirements.
