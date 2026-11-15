## Runner update —14 November2026

Explicit runner support and boundaries are now tested. Run in Ubuntu/WSL:

```sh
python3 scripts/run_access_authorization.py configs/access_500_device_probe.json --probe-500 --outcome-measurement --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

Replace dependency paths with your existing pinned build. --probe-500 and
--scale-control are mutually exclusive. Explicit500cap is499local+server,
2000tasks and3600s wall per original/replay execution. Ordinary/intermediate
limits stay unchanged. This enables measurement,not a500-device success claim.
First probe is starting;the preparation/extension discussion below is historical.

# Prepared500-device wireless probe

`configs/access_500_device_probe.json` is prepared and design-validated; it has
not been simulated.500 includes the external server:470stationary robots
(224NR,246Wi-Fi),24zone infrastructure,2spines,3EPC nodes,1server. Eight zones
retain mildly unequal endpoint counts.50ms ARP is an explicit candidate preset;
the omitted default remains1000ms.

Every robot offers a task in each of three phases. Four additional scope/control
tasks per zone produce502tasks/phase,1506total. Tasks alternate normal/protected
cross-access traffic,with same-zone normal/protected and same-access peer controls.
JSON protected task types continue requiring the main-server approval. Deadlines
remain100ms;authorization timeout remains1000ms.

Preserve4.7ms launch spacing rather than increasing the offered task rate merely
to fit the old phase windows. Starts are1200/4300/8300ms;spine0 down4000/up8000ms.
This lengthens the before/during/after phases relative to300devices. It is a scale
probe with a longer schedule,not an identical-arrival comparison to300devices.

The existing runner still rejects499local devices (scale cap300);preparation and
native task loading still cap1000tasks. Before execution,add a distinct explicit
500probe opt-in,keep ordinary64/intermediate300limits,and bound task counts in
Python and C++ consistently (proposed upper2000). The native inventory bound4096
is already sufficient. Review wall-time scaling:retain existing limits for old
cases;propose3600s per native execution for the explicit500probe,with original
and replay. This is a resource allowance,not a performance claim. Simulated
deadlines must remain independent from wall time.

Add tests for default/intermediate rejection,new exact boundaries,over-limit
tasks and full470source coverage per phase. Run required source/core checks and
small native replay before the expensive probe. First run seed1/run1 outcome
measurement;retain all offered tasks and any failed deliveries/deadlines. Stop
on execution/audit/replay failure. Multiple seeds follow only after first-run
review. No500-device readiness claim until actual results support it.
