# Three-seed ARP retry comparison

14 November2026.300 total devices,270 stationary wireless robots across8 zones,
128NR/142Wi-Fi,ECMP,spine0 outage3–5s. Each case offers906 tasks with100ms
deadlines and1000ms authorization timeout; all offered tasks remain denominators.
Only the global ARP cache wait parameter varies. Caches start cold. This setting
also affects wired-interface ARP; it is not a Wi-Fi-only change.

## Outcomes across seeds1/2/3,run1

| Wait(ms) | Offered | Delivered | Deadline met | Authorization timeout |
|---|---:|---:|---:|---:|
|1000|2718|2717|2708|1|
|100|2718|2718|2714|0|
|50|2718|2718|2718|0|
|20|2718|2718|2718|0|

## Individual cases

| Seed | Wait(ms) | Delivered | Deadline met | Result directory |
|---|---:|---:|---:|---|
|1|1000|905|901|`results/access-auth-20261114T081820Z-8bca2400`|
|1|100|906|904|`results/access-auth-20261114T085336Z-f2ac9a71`|
|1|50|906|906|`results/access-auth-20261114T071931Z-63e8482d`|
|1|20|906|906|`results/access-auth-20261114T074323Z-98e1b584`|
|2|1000|906|904|`results/access-auth-20261114T092727Z-cb7f2d64`|
|2|100|906|905|`results/access-auth-20261114T095109Z-1723ac8f`|
|2|50|906|906|`results/access-auth-20261114T101227Z-db895fc9`|
|2|20|906|906|`results/access-auth-20261114T103423Z-d66a4ac0`|
|3|1000|906|903|`results/access-auth-20261114T105503Z-f18f4f56`|
|3|100|906|905|`results/access-auth-20261114T111612Z-d221aff5`|
|3|50|906|906|`results/access-auth-20261114T113820Z-9e4d4692`|
|3|20|906|906|`results/access-auth-20261114T115912Z-eb392e9c`|

All12 cases and24 native original/replay executions completed. Independent outcome,
authorization,deadline,coverage,ECMP and radio-seed audits were rerun and match
saved audits exactly. Original/replay traces and device CSV are byte-identical.
The baseline seed1 native exit2 and failed strict-delivery outcome are retained;
MEASURED/PASS describes measurement validity. Other native cases exit0.
The corrected seed1 baseline/100ms traces match earlier results exactly.
The prior50ms observer crash and failed-run evidence remain preserved and are
excluded from successful measurements. Source correction details are in
[seed1 record](access-arp-seed1-progress.md).

50ms and20ms delivered all2718 tasks within deadline across these three finite
runs.100ms had four late deliveries;1000ms had nine late deliveries and one
authorization timeout. These are illustrative experiment observations. Three
seeds do not establish a deployment guarantee, and changed retry timing can
alter subsequent contention and random-number consumption. This study does not
measure ARP overhead or prove the shorter setting is optimal. The1000ms default
is retained pending overhead evaluation.500-device wireless validation is pending.

Raw data/provenance remain in each result directory. The final independent
re-audit is `.recovery/access-arp/matrix-final-audit.json`; aggregate CSV is
`.recovery/access-arp/matrix-summary.csv`. No simulation from this matrix remains
active. See [overhead protocol](access-arp-overhead-plan.md) for the next study.
