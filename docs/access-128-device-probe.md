# 128-device stationary wireless probe

This intermediate control uses `configs/access_128_device_probe.json` and the
existing explicit `--scale-control` path. No device ceiling, native timeout,
radio parameter, authorization timeout or task deadline was increased.

| Component | Count |
|---|---:|
| NR robots (six per zone) | 48 |
| Wi-Fi robots (six in A-F, seven in G-H) | 50 |
| Zone gateways / APs / gNBs | 24 |
| Shared spines | 2 |
| Shared EPC nodes | 3 |
| External main-server model | 1 |
| Total | 128 |

All 98 stationary wireless robots offer tasks in each of three phases. Each
phase contains 130 tasks, including normal/protected cross-zone traffic and
additional local and same-access peers: 390 offered tasks overall. Task spacing
is 11 ms within phases starting at 1200, 3300 and 5300 ms. Spine0 is down at
3000 ms and restored at 5000 ms. Deadlines remain 100 ms. Seed1/run1 and ECMP
are explicit. This is a single-seed probe with deterministic replay, not a
multi-seed comparison or a capacity/latency guarantee.

Relative to the 100-device control, robot counts and offered task rate both
increase. Runtime differences therefore cannot isolate the effect of inventory
alone. Geographic origins, fixed-position rule and shared local EPC architecture
are retained. All NR access remains actual 5G-LENA NR; Wi-Fi access remains
actual ns-3 Wi-Fi. Upper network/server behavior has the previously documented
abstraction limits. The public wired 500-device demonstration is a separate model.

## Reproduction

Use the pinned Linux/WSL installation described in the access runner guide:

```sh
python3 scripts/run_access_authorization.py configs/access_128_device_probe.json --scale-control --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

The ordinary limit remains 64 local devices; the explicit scale path allows
128 local devices and 300 seconds per native execution. This configuration uses
127 local nodes plus one external server. Result status must be inspected along
with task deadlines, actual source coverage, assigned-source ECMP audit and
replay. A timeout is a failed probe, not justification to silently raise limits.

## Scaling constraints to review next

The runner also caps a control at 1,000 tasks. An eventual 500-device all-source,
three-phase workload would exceed that cap with the current task pattern. Both
the inventory ceiling and task/runtime bounds must therefore be reviewed explicitly
before that workload is accepted. Reducing active robot coverage merely to fit
the guard would not establish equivalent validation.

## Measured result

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

### Deadline misses

No deadline misses in this single-seed probe. This does not erase earlier misses.
