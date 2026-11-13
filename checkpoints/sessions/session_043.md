# checkpoint_047 — 200-device stationary wireless probe

Eight zones,170 wireless robots (80NR,90Wi-Fi),24 zone infrastructure,
2 spines,3 EPC nodes and1 external server:200 devices total.
Config: configs/access_200_device_probe.json. Seed1/run1, ECMP.
606 tasks (202 per phase), all170 sources delivered before/during/after outage.
Delivered 606/606; deadline-met 606/606.
Maximum full-task delay 50.746369ms; exact original/replay equality.
Result: results/access-auth-20261107T171246Z-8f595a8c
Native wall seconds: {'original': 429.76617930099997, 'replay': 335.05552910800003}.

Explicit --scale-control ceiling now200 LOCAL devices; ordinary64 remains.
Native timeouts: ordinary180s; scale<=128 local300s; scale>128 local600s.
1000-task cap retained. This probe uses199 local+1 external device.53 access
checks and21 core checks PASS. Native source hashes identical to128-device
probe: no radio/ARP/deadline/native routing changes. Inventory and task rate both
increase, so this is not an isolated device-count scaling measurement.

Single-seed functional/runtime control, not multi-seed policy comparison,
calibrated saturation/capacity result, or500-device completion. Historical100-
device four-case result remains1224 delivered/1220 deadline-met. Prior misses
are retained. L3 ECMP groups GTP tunnels. Upper TSN/shim/backhaul, cryptography,
mobility and distributed convergence remain outside scope.

No active process. No Git commit/push. Source manifest checkpoint_047.
Recovery package checkpoint_047-200-device.zip, prior packages preserved.
Details: docs/access-200-device-probe.md and .recovery/access-200/summary.json.

Next exact action: Review200-device runtime and losses/deadlines before choosing the next bounded step toward500. Keep full robot source coverage, explicit task/runtime caps, actualNR/Wi-Fi and earlier deadline misses.500-device wireless validation, visualization and final report remain unfinished.
