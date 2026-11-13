# checkpoint_036 — Direct-zone and spine-only packet paths validated

Added ninth cross-zone Wi-Fi diagnostic flow in access-fabric-control.h.
Two datasets: direct A-B gateway link present vs absent. Both actual native
runs exit0,90/90 received,19 nodes. Each repeated; complete traces/device CSVs
byte-identical. Audit_access_links.py validates all10 added datagrams use exact
expected fabric path: AP-A/gateway-A/gateway-B/AP-B or same via spine-0.
Existing80 S1 tunnel datagrams per run still pass independent path check.
Negative check rejects direct path as spine path. Project verify21/21 PASS.

Raw input/output/audits/hashes: .recovery/access-link-control.
Driver .recovery/run_access_links.sh; all4 executions finished. No active process.
Source checkpoint_036. No Git upload. Old runs preserved.
This is STATIC topology comparison, not dynamic failure recovery, ECMP or latency
comparison. Configured protected tasks still NOT executed; diagnostic traffic only.
Next implement actual per-task main-server authorization gate (normal/allow/deny/
timeout) with local post-approval payload and full task timing/accounting. Preserve
upper-network abstraction and routing scope. Live spine outage test still pending.
