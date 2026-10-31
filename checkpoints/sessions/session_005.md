# Session 005 — active wired ns-3 runtime and handoff work

Implementation committed as a2bf99f. Core verification: 20/20 PASS. Fresh source extraction
in a path with spaces compiled successfully; 20/20 tests and 180/180 delivery PASS after
fixing enclosing-repository provenance detection. Saved validated extraction outputs as
results/first-run-20261031T094344Z-05624c73 (copied unchanged from the recorded .recovery path).
The initial overnight interrupted run recorded two timeouts over 43920.75 seconds; preserved
logs and unchanged-limit retry passed. No timeouts/tests/warnings were disabled.

Added strict ns-3 dependency requirement/prefix handling; optional NetAnim separation;
pinned local dependency installer with integrity checks and upstream tests; real UDP smoke
fixture with analytical one/two-hop delay and receive-error loss checks; first_run.py;
streaming logs; archive-safe Git provenance; Mac/Ubuntu first-run guide (not executed there).
New C++ packet files passed actual ns-3 header syntax checks after correcting a missing include.

External wired ns-3 build is STILL IN PROGRESS; do not claim packet runtime success yet.
Active setup log: .tools/ns3-setup-20261030T212142Z-cfcd86e2.log.
Build: .tools/ns3-wired-build; output: .tools/ns-3.46.1/build-wired;
intended install: .tools/ns3-wired-install. Corrected output placement because upstream test
data discovery walks up from the runner executable. Prior outside-source build was stopped
and retained, not relabeled as validated. Old Wi-Fi/NetAnim failure remains unresolved.

Next: wait for setup, inspect upstream UNIT QUICK results, then run scripts/run_ns3_smoke.py.
Resolve actual errors before claiming success. Run full warehouse baseline regression,
finish docs, checkpoint_008 and recovery packages. Schema-v2 ns-3 forwarding/QoS, NR and real
Mac/Ubuntu execution remain pending even if the small wired runtime succeeds.

Resume update: warehouse batch experiments-20261031T094745Z-723bc3c8 completed all 132 runs, exact baseline comparison, replay and 55 charts PASS. Saved checkpoint008 warehouse/first-run/bootstrap evidence. Wired dependency compilation continues; upstream tests and actual project runtime still pending.

Dependency update: wired build completed successfully. Initial bulk UNIT invocation failed because ns-3.46.1 requires separate suite processes. Corrected orchestration committed as 39ff2af. All 150 suites then passed (threaded-simulator 274.530 s), but installer report matching rejected noisy subcase PASS diagnostics. Independent exact summary-format audit passed all 150; regex correction and its test are in the working tree. Final full setup command is now active; log .tools/ns3-setup-20261031T101736Z-1621e863.log. No tests/warnings were disabled. Still do not claim installed dependency or project packet runtime until those gates pass. GDB attach attempt arrived after the slow suite had already exited and collected no stack; diagnostic retained.

## Final checkpoint_008 outcome (supersedes the in-progress notes above)

Final code commit 47088f1. Installer end-to-end PASS: .tools/ns3-setup-20261031T101736Z-1621e863.log,
150/150 UNIT QUICK; local .tools/ns3-wired-install receipt PASS. Upstream source unchanged.
Project strict build 22/22 PASS. Actual two-hop UDP 3/3 and 4.416 ms, one/two-hop loss tests,
constant positions and byte-identical replay PASS. Batch ns3-smoke-20261031T102542Z-fd5d9232.
Final isolated source first run 20/20 and 180/180 PASS; all code hashes match current code,
docs-only differences. Batch first-run-20261031T101929Z-5481bba4 copied unchanged to root results.
Full 132-run baseline regression and 55 charts PASS in experiments-20261031T094745Z-723bc3c8.
Explicit empty ns-3 prefix fails even with a valid global prefix; existing smoke report is
preserved on attempted output reuse. Evidence, source manifest and recovery packages retained.

Read CURRENT_STATUS and NEXT_STEPS for the next exact action: implement the first actual
stationary schema-v2 wired backend slice with observed peer/service paths. Full QoS/NR,
calibration and target Mac/Ubuntu execution remain open. Do not describe the whole product
as finished or the core's warehouse study as measured ns-3/NR results.

Packaging resume: automatic approval review previously hit a usage limit before source-manifest creation or recovery packaging. Resumed on user instruction; confirmed checkpoint_008 manifest absent, with all completed test/results files intact. Final documentation review corrected a UTF-8 title artifact before freezing source. No simulator code changed.
