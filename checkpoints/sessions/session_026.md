# checkpoint_030 — Inventory-driven native radio construction

Resumed 6 November. Added scripts/prepare_wireless_nodes.py and
src/wireless-construction.cc, documented in docs/wireless-layout.md.
Linux target scratch_warehouse-wireless-construction built successfully.
Small JSON exported to .recovery/wireless-small-nodes.txt. Native run created
21 actual nodes (18 inventory rows plus EPC PGW/SGW/MME). All four NR UEs
CONNECTED_NORMALLY on configured gNBs; four Wi-Fi STAs associated to configured
APs. Explicit attachment assertions and total NodeList count passed.
Repeat audit CSV byte-identical. Independent Python comparison checked all
18 IDs, roles, attachments, radio device types and fixed positions against JSON.
Project scripts/verify.py 21/21 PASS. No background process remains running.

Linux logs/audits: /home/warehousebuild/warehouse-radio/construction-build.log,
construction-small.log, construction-small-audit.csv, construction-small-replay.csv.
Windows audit: .recovery/construction-small-audit.csv. Original results preserved.
No Git upload. Manifest checkpoint_030. New control built via ns-3 scratch target,
not main wired CMake. It requires NR UEs/gNBs; no full-scale test performed.

IMPORTANT: construction/attachment only. No warehouse links/routes/application
traffic yet. Radio defaults fixed and illustrative (see documentation), UMi not
calibrated indoor. Next connect leaf/spine/edge/AP/PGW fabric, explicit IP routing
and small peer/service flows, independently audit received packets and paths.
Do not call this a completed warehouse wireless simulation.
