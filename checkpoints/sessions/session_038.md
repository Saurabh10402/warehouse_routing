# checkpoint_042 — Native Layer-3 ECMP validated

Added src/access-ecmp.h, a higher-priority IPv4 routing protocol preserving helper
routes for local delivery/EPC. Hashes node name, source/destination IPv4 and IP
protocol (FNV1a32 + avalanche), not UDP ports. Per-IP-flow grouping and sorted
equal shortest-hop next hops. Core/server are endpoints, never transit shortcuts.
Route rebuild removes failed spine, restoration returns original candidate sets.
Locally originated wildcard-source traffic uses stable router identity. NR outer
GTP tunnels stay grouped; no inner-flow balancing claim.

Maintained runner accepts routing:ecmp. Native task table includes policy;
source snapshots include access-ecmp.h. Original shortest-path implementation
preserved. Explicit boundary-control+ecmp rejected pending targeted validation.
configs/access_ecmp_control.json:19 local+1 external,36 tasks across three phases,
repeated Wi-Fi endpoint pairs plus NR/protected tasks, no direct zone link.
Independent audit_access_ecmp.py reconstructs reverse distances from links and
checks every actual UDP wire hop and hash against destination ownership.

Accepted original/replay byte-identical results:
- ECMP allow/outage: access-auth-20261107T061316Z-552a3f63 (36/36)
- ECMP deny: access-auth-20261107T061459Z-872adede (30 delivered,6 blocked)
- ECMP timeout: access-auth-20261107T061513Z-6a50d151 (30 delivered,6 blocked)
- ECMP direct link: access-auth-20261107T061524Z-bc43613c (36/36)
- ECMP static: access-auth-20261107T061537Z-412d1830 (36/36)
- shortest-path matched36-task input: access-auth-20261107T061552Z-1ecea1bb (36/36)
- prior phase control: access-auth-20261107T061605Z-1ab8db46 (12/12)
- prior static: access-auth-20261107T061617Z-224f9f53 (3/3)
- prior boundary: access-auth-20261107T061628Z-2e5b5f72 (43/46,2 auth timeouts,1 observed payload drop)
All directories under results/. Summary .recovery/access-ecmp/summary.json.

Main ECMP audit:210 hops,42 equal-cost choices,12 repeated multipath groups,
41 restored mappings. Both spines before/after; only spine1 during outage.
Matched policies differ only by routing input. Both median1.2494385ms and
max45.002193ms; no latency/congestion advantage established by this small run.
Prior phase/static/boundary traces and CSVs exactly match checkpoint041 references;
receipt .recovery/access-ecmp/regression-receipt.json.

Access tests34/34 PASS; project scripts/verify.py21/21 PASS. No upstream suite
repeat. Actual-trace tests reject wrong equal-cost selection, wrong owner,
unmapped packet destinations and unsupported ECMP boundary audit combination.
Initial failed build access-auth-20261107T061104Z-6c1569c5 retained: callback
required const-reference arguments in pinned ns-3; fixed before native runs.

Docs: docs/access-ecmp-control.md and run-access-authorization.md; README now
clearly separates small wireless controls from preserved500-device wired demo.
Source checkpoint_042; separate checkpoint_042-access-ecmp.zip recovery package.
No Git commit/push. No background processes active. Prior archives/results kept.

Limits: L3 grouping (no port/inner-GTP splitting), hop-count metric rather than
old wired weighted Dijkstra, fixed hash, radio seed1/run1, immediate centralized
failure knowledge. No protocol convergence, movement, calibrated load, cryptographic
auth, upper TSN/shim/5G implementation, wireless queue-priority comparison or
500-device wireless performance claim. ECMP-specific boundary loss not validated.

Next exact action: make radio seed/run configurable (preserve seed1/run1 defaults),
then run a bounded matched shortest-path/ECMP multi-seed comparison with exact
replay per seed before increasing layout scale. Preserve all current regression
and fail-closed gates. Later: ECMP/NR-inner boundary loss and wireless scaling.
