# checkpoint_048 — 300-device probe failed; partial diagnosis preserved

300 total nodes:270 stationary wireless robots (128NR,142Wi-Fi),24 zone infrastructure,
2 spines,3 EPC support nodes and1 external server. Eight zones,906 tasks,seed1/run1 ECMP.
Native source hashes match the200-device case; radio/deadline/ARP settings unchanged.
Result: results/access-auth-20261108T085859Z-4f29fbed
Original native exit2; strict delivery-control status remains FAILED.
905/906 tasks delivered;901/906 met100ms deadlines. One authorization timeout,
four delivered tasks late. All270 sources offered every phase;269 delivered sources
before outage,270 during/after. Partial independent checks validate8694 wire hops,
assigned ECMP sources and expected spine use. This is not an overall audit PASS.

Task16 before-A-wifi-0: start1275.2ms, server allow1294.426423ms,
timeout2275.2ms, ignored late response2305.269791ms. No payload sent or received.
Authorization blocking behaved correctly. Four late delivered tasks:33,226,280,298.
Their full delays:950.527262,1013.312108,762.671427,675.366262ms respectively.

Diagnostic rerun with same executable/seed and ARP logging hit1200s wall timeout.
Captured190464 native bytes exactly match original prefix; no complete replay or
final device CSV. Partial ARP log: zone-A AP waits for192.168.0.2 at1304.627335ms,
initial request1309.627335ms, still awaiting reply1391.679230ms. Log ends at2185.728349ms,
before task16 timeout/late response. Complete delay attribution and first-exchange
PHY/MAC failure cause remain unconfirmed. No claim that diagnostic fully reproduces.

Runner bounds: ordinary64 local/180s; explicit scale300 local,300s through128,
600s through200,1200s above200;1000 tasks. This is permission to probe, not capacity.
Added timed_command so failed, timed-out and interrupted native executions retain
elapsed time without swallowing errors. Historical failed run has no exact elapsed
receipt; do not invent it.56 access tests and21 core checks PASS.
No native model/dependency changes. No active simulation or Git commit/push.
Source manifest checkpoint_048; separate checkpoint_048-300-device-failure.zip.
200-device checkpoint047 remains last passing larger probe.500-device validation,
visualization integration and final report remain unfinished.

Next exact action: Add an explicit general outcome-measurement mode that audits legitimate wireless loss/timeouts and runs replay while preserving strict delivery-control failure semantics and all offered/deadline denominators. Then use focused ARP/MAC tracing (prefer Linux-local log output to avoid mounted-file overhead) to complete task16 diagnosis. Do not increase to500, relax deadlines/timeouts, or prewarm caches to hide this failure.
