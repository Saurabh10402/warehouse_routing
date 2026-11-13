# checkpoint_041 — Transition-boundary loss and timing verified

Added receive-side IP_DROP evidence in native task fabric, plus separate
--boundary-control maintained-runner mode and scripts/audit_access_boundary.py.
Native all-delivered gate remains intact: exit2 is recorded for lost tasks;
only boundary mode accepts it provisionally and independently audits outcomes.
Ordinary delivery control is not weakened. Boundary status explicitly says
PASS is correct loss/accounting, not all-delivered.

configs/access_boundary_control.json:19 local+1 external,46 tasks, no direct
zone links, spine0 down1999.79ms/up3500ms. Twelve mixed NR/Wi-Fi phase tasks,
three protected boundary probes,31 Wi-Fi data probes spaced0.1ms. Targeted down
time chosen from preliminary trace to straddle an actual fabric transmission.

Accepted results, every original/replay byte-identical:
- results/access-auth-20261106T193539Z-236294af:43 delivered,2 auth timeouts,
  1 observed payload drop,3 deadline misses/46 offered. Native exit2 preserved.
- results/access-auth-20261106T193705Z-60586053: fractional upper10.5ms,
  processing5.25ms,timeout1000.5ms; same counts, exact replay.
- results/access-auth-20261106T193553Z-a9c0a19f: phase control12/12 PASS.
- results/access-auth-20261106T193603Z-f33a7f3e: static3/3 PASS.
Phase/static task outcomes and timings exactly match checkpoint040.
Final auditor rechecks stored in .recovery/access-boundary/reaudit.json.

Drops independently matched to pre-shutdown wire transmissions: data task18,
approval reply12 and request13. Tasks12/13 sent zero payload, exact timeouts.
Protected task14 request crossed restoration and completed. Healthy phase
traffic remains delivered. All blocked/unreceived tasks count as deadline misses.

Discovered/fixed fractional-time truncation by native integer MilliSeconds:
ControlMs now rounds positive milliseconds to nearest ns (half ties upward),
used for starts/events/service delays/timeouts/deadlines; Python time_ns matches.
Failed diagnostic193104Z-52f6472f retained with wrong-start audit, not accepted.
Corrected exploratory193343Z-7e4491a3 at2000ms observed authorization loss only;
retained as exploratory, does not meet final audit requirement of payload loss.

Validation: access tests29/29 PASS incl real trace fixture corruption tests;
required scripts/verify.py21/21 PASS (.recovery/access-boundary/verify.log).
No upstream suites rerun. docs/access-boundary-control.md describes methods,
provenance, exact reproduction, counters, exploratory cases and limitations.
Source checkpoint_041; recovery package checkpoint_041-boundary-loss.zip.
No background processes active. No Git commit/push. Prior data preserved.

Limitations: single seed1/run1; immediate centralized failure knowledge; admin
IPv4 interface outage, not physical power loss or protocol convergence. Boundary
loss probes use Wi-Fi endpoints; NR continues in healthy phase tasks, not yet
inner-GTP-correlated boundary losses. No retries, calibrated load, crypto,
real TSN/shim/upper5G, or500-device wireless capacity claim.

Next: implement native ECMP as an explicit capability, retain shortest-path
baseline and all current authorization/failure/boundary gates. Before adding it,
review equal-cost next-hop selection/per-flow consistency and how it interacts
with outage/restoration. NR inner-packet loss correlation and multi-seed/scale
remain later validation work; do not silently promote controls to production.
