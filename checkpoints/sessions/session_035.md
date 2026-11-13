# checkpoint_039 — Stable infrastructure identities

Implemented stable loopback /32 identities for native task-fabric routers in
10.254.0.0/16; physical /30 addresses remain next hops. Robot radio and EPC
helper interfaces unchanged. Main-server socket explicitly binds its identity.
Audit verifies unique identities and server request/reply addresses. ARP-drop
logging reports payload bytes instead of interpreting task text as SeqTsHeader.

Actual maintained-runner controls, each original/replay byte-identical and PASS:
- allow: results/access-auth-20261106T190736Z-72af3943 (3 delivered)
- deny: results/access-auth-20261106T190910Z-c0535c0a (normal delivered, 2 blocked)
- timeout: results/access-auth-20261106T190931Z-89053a8b (normal delivered, 2 blocked)
Allow timings match preceding baseline: 15.991259,41.449187,35.242855 ms.
Initial diagnostic results/access-auth-20261106T190541Z-256a6f8f FAILED because
reply socket selected physical source IP; protected tasks correctly timed out
with no payload. Preserved; fixed by explicit binding, not relaxed identity check.

Project verify21/21 PASS with normal Windows permissions; first restricted run
had temporary-directory permission errors in5 tests. Log .recovery/verify-stable-identities.log.
Access contract/path/capability tests11/11 PASS. No upstream suites repeated.
No background process active. No Git upload. Source checkpoint_039.

This is a prerequisite milestone, NOT completed live failure recovery. Outage
configurations remain rejected. Next add configurable task start times and
spine-down/up events; persist fabric graph; disable incident interfaces and
rebuild only generated host routes while preserving EPC routes. Test before,
during and after outage with actual spine wire traces and authorization gates.
Account for interface-down route removal and queued packets. State immediate
centralized knowledge explicitly. Preserve historical controls. ECMP, broader
radio/S5 audit, multi-seed studies and500-device wireless scaling remain pending.
