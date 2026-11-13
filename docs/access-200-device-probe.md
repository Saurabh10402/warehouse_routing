# 200-device stationary wireless probe

This is a bounded single-seed ECMP probe with an exact replay, using
`configs/access_200_device_probe.json`. It extends the eight-zone architecture
without changing the native radio model, task deadlines or authorization rules.

| Component | Count |
|---|---:|
| NR robots (10 per zone) | 80 |
| Wi-Fi robots (11 in A-F,12 in G-H) | 90 |
| Zone gateways / Wi-Fi APs / NR gNBs | 24 |
| Shared spines | 2 |
| Shared EPC nodes | 3 |
| External main-server model | 1 |
| **Total** | **200** |

All170 stationary robots offer traffic in each phase. There are202 tasks per
phase and606 overall, covering protected and normal local/interzone tasks.
Start times are1200/3300/5300ms plus7ms per task, retaining the3000ms spine0 outage
and5000ms restoration. Seed1/run1 is explicit. Deadlines remain100ms; no ARP
prewarming is introduced. Both access types use actual ns-3 devices, including
5G-LENA NR and shared local EPC. The external server/upper network remain abstractions.

## Execution bounds and reproduction

```sh
python3 scripts/run_access_authorization.py configs/access_200_device_probe.json --scale-control --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

Use the pinned Linux/WSL dependencies in the access runner guide. Ordinary runs
retain64 local devices/180 seconds. The explicit scale path now permits up to200
local devices, retaining300 seconds through128 local nodes and allowing600 seconds
above that. This probe has199 local nodes. Tests cover the129-node timeout boundary,
rejection above200 local devices and all-source phase coverage. The1000-task cap
is unchanged. The previous128-device run took240/203 seconds; the larger bound
provides headroom for this probe, rather than promising a runtime.

Relative to the128-device case, both inventory and offered task rate increase.
This is not an isolated node-count benchmark, multi-seed comparison, calibrated
warehouse load, throughput capacity measurement, or500-device validation. Prior
100-device deadline misses remain part of the evidence. No movement, upper TSN,
shim, full5G backhaul, cryptographic authorization or distributed convergence
is implemented by this probe. L3 ECMP still groups outer GTP tunnels.

## Measured result

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

### Deadline misses

No deadline misses in this single-seed probe. This does not erase earlier misses.
