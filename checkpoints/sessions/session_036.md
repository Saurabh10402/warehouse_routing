# checkpoint_040 — Live spine outage and restoration validated

Implemented native administrative down/up events for all interfaces incident to
one configured spine. Both ends are disabled; loopback identities remain up.
Value-owned fabric graph rebuilds deterministic shortest-hop routes immediately,
removing generated gateway host routes and preserving connected/WLAN/EPC routes.
Restoration re-enables interfaces and rebuilds original routes. No protocol delay.

Configuration: optional root outage {spine,downMs,upMs}; task startMs optional.
Maintained runner accepts explicit scheduled single-spine-outage, still rejects
ECMP, unscheduled outage, >64 local nodes. Example configs/access_spine_outage_control.json:
19 local +1 external server,12 tasks, down2000ms/up3500ms, three traffic phases.
All robots remain stationary real NR/Wi-Fi endpoints; no robot wired substitution.

Actual pinned ns-3/NR maintained runs, each original/replay byte-identical PASS:
- allow: results/access-auth-20261106T191625Z-ebf3b877 (12/12 delivered)
- deny: results/access-auth-20261106T191816Z-2ae8fc2e (6 normal delivered,6 protected blocked)
- timeout: results/access-auth-20261106T191829Z-ea72f857 (6 normal delivered,6 protected blocked)
- no direct zone link: results/access-auth-20261106T191839Z-ea853c40 (12/12 delivered)
- other spine (spine1): results/access-auth-20261106T191851Z-797ddf64 (12/12 delivered)
- static regression: results/access-auth-20261106T191902Z-3436c690 (3/3 delivered)
Static task outcomes/timings exactly equal checkpoint039 allow. No upstream suite rerun.
Main outage proves actual server and NR tunnel traffic spine0 -> spine1 -> spine0.
All five incident links/ten interfaces change at configured times. No failed-spine
UDP transmissions during outage. Separate Wi-Fi path receipt verifies the same
spine transition in the no-direct-link case (.recovery/spine-outage/wifi-spine-paths.json).
Blocked tasks send zero payload and remain in offered denominators.

Audit additionally checks task starts, events, rebuild counts and identities.
Compact native fixture tests rejection of failed-spine traffic, missing restoration,
missing rebuild, changed start time and mismatched replay. Access tests22/22 PASS.
Required scripts/verify.py21/21 PASS; log .recovery/spine-outage/verify.log.
A local Bash launcher initially failed before any simulation because of CRLF;
corrected to LF, then all controls passed. No hidden failed simulation in this milestone.

See docs/run-access-authorization.md for full reproduction/config/results/limits.
Source checkpoint_040. No Git commit/push. No background process active.
Raw result directories are preserved, plus separate checkpoint040 recovery ZIP.

Limits: immediate centralized knowledge, administrative interfaces (not hardware
power loss); tasks away from event boundaries. In-flight packets can be lost;
no zero-loss guarantee, request retries, real TSN/shim/upper5G or cryptographic auth.
NR still uses shared local EPC below Layer1, not per-zone local NR breakout.
Single seed1/run1 only; no full multiple-seed or500-device wireless claim.

Next exact action: add a transition-boundary control (requests/data spanning
spine down/up) with explicit loss/timeout accounting, rather than claiming
zero-loss recovery from phase-separated controls. Then implement native ECMP
as a separately audited capability. Keep static regression and historical sources.
