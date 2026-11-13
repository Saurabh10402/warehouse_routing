# checkpoint_035 — Corrected routed S1 control passes

New separate src/access-backhaul-control.cc and access-fabric-control.h implement
NoBackhaul EPC with explicit gNB->zone gateway->spine->SGW tunnel paths.
SGW/PGW dual-homed to spines; configured direct zone gateway link installed.
No edge servers. Historical control untouched. New prepared19-node small example
has8 diagnostic peer flows,80/80 delivery, exit0. NO AUTHORIZATION/TASK EXECUTION.
Configured protected tasks are NOT run; probes are separate diagnostic workload.

scripts/prepare_access_control.py exports validated tables; audit_access_backhaul.py
independently checks80 outer S1 datagrams have ordered3-link paths and rejects
fabricated shortcut. Full run log and device CSV repeat byte-identically.
Actual NR connected/Wi-Fi associated checks pass. scripts/verify.py21/21 PASS.
Docs/access-backhaul-control.md contains exact setup, scope and limitations.
Raw inputs/results/receipt .recovery/access-backhaul-small; Linux originals
/home/warehousebuild/warehouse-radio/access-backhaul*. Source checkpoint_035.
No running process, no Git upload.

Next: direct-link and spine-fallback packet controls (current peer ring does not
exercise cross-zone Wi-Fi direct path), then actual main-server authorization
state machine and task TX/RX/deadline audit. Current trace audit covers fabric
transmissions only, not full radio/S5/application path/delay. No ECMP/failover/
500-node/authorization success claim. Previous ARP loss not erased by new workload.
