# checkpoint_031 — Small wireless fabric implemented; delivery shortfall open

Added src/wireless-fabric.h, included by wireless-construction.cc.
Actual P2P leaf/spine/AP/edge and PGW-to-spine links, static shortest-hop host
routes, Wi-Fi subnet addressing and 16 UDP server/peer flows now implemented.
EPC gNB/core backhaul remains helper-provided, outside zone leaf/spine fabric.
NR/Wi-Fi radio models unchanged. No ECMP/failure/priority comparison claim.
Linux build PASS. Small run 153/160 received: 15 flows 10/10, zone-A-nr-robot-1
to zone-A-wifi-robot-0 only 3/10. All attachment/count checks passed.
Original first diagnostic aborted at first shortfall; revised run reports every
flow and returns failure for shortfall. Cause UNDIAGNOSED; possible ARP/radio
behavior requires tracing, not parameter tuning or assertion suppression.

Linux evidence /home/warehousebuild/warehouse-radio/fabric-build-v2.log,
fabric-small-v2.log, fabric-small-v2-audit.csv; initial failed logs retained.
Windows copies in .recovery/fabric-small-v2.log and matching audit CSV.
Project scripts/verify.py 21/21 PASS. Manifest checkpoint_031.
No background processes active. No Git upload.
Next: trace offered/received sequence IDs and ARP/IP/MAC drops around failing
NR-to-Wi-Fi flow; identify cause, validate correction/control, then replay and
independent per-hop audit. Full warehouse validation and scaling pending.
