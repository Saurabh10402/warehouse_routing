# checkpoint_046 — 128-device wireless probe

Eight stationary zones,98 wireless robots (48 NR,50 Wi-Fi),24 zone infrastructure,
2 spines,3 EPC support nodes and1 external server:128 total devices.
Config: configs/access_128_device_probe.json; seed1/run1, ECMP.
390 tasks:130 before/during/after spine outage. All98 sources delivered each phase.
Delivered 390/390; deadline-met 390/390; maximum full-task delay 49.854538ms. Exact original/replay equality.
Result: results/access-auth-20261107T165030Z-18384f0b.
Native seconds: {'original': 240.32492281100002, 'replay': 203.41871870200004}.

Default64-local and explicit128-local ceilings unchanged;300s per native execution.
No native code, dependencies, radio, ARP or deadline changes. Config increases
both inventory and offered task rate; this is not isolated device-count scaling,
not a multi-seed policy comparison and not proof of500-device capacity.
Actual coverage, valid assigned ECMP sources, outage routing and auth audits PASS.
52 access tests and21 core checks PASS. No active process or Git commit/push.
Source manifest checkpoint_046; recovery archive checkpoint_046-128-device.zip.
Historical checkpoint045 four-case result remains1224 delivered/1220 deadline-met;
its four deadline misses are not superseded by a different seed/configuration.

Next exact action: Prepare an explicit approximately200-device probe with bounded runtime based on measured128-device costs; change ceilings only deliberately with validation. Retain100/128 controls and all deadline misses.500-device wireless validation and visualization/report integration remain unfinished.
