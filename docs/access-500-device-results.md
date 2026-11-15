# Completed500-device three-seed wireless study

15 November2026. Seeds1/2/3,run1,each execute1506tasks with an exact native
replay. The configuration is identical except radio seed.500total devices include
224NR+246Wi-Fi stationary robots,24zone infrastructure,2spines,3EPC support nodes
and1external main-server model. Eight zones retain unequal configurable counts.

| Seed | Offered | Delivered | Within100ms deadline | Original wall(s) | Replay wall(s) |
|---|---:|---:|---:|---:|---:|
|1|1506|1506|1506|2034.838|1966.390|
|2|1506|1506|1506|2038.773|1947.606|
|3|1506|1506|1506|1863.162|1792.156|

Total4518/4518 delivered and within deadline. No authorization timeouts,denials,
observed payload drops or unsent payload failures in these allow-mode runs.
Each case independently re-audited outcome accounting,authorization ordering,
deadlines,coverage,ECMP and seed markers; results exactly match saved audits.
Original/replay task traces and device CSV match byte-for-byte. Native exits0;
MEASURED validationPASS with allTasksDelivered/allDeadlinesMet true.

All470robots offer and deliver traffic in every phase. Both spines are used
before/after outage;only spine1 during spine0 outage. The configuration uses
explicit50msARP,1000ms authorization timeout,100ms task deadlines,4.7ms task
spacing,phase starts1200/4300/8300ms and outage4000–8000ms. Each phase has502tasks.
Protected task types require main-server approval even for same-zone payload.

This is a finite stationary workload,not a calibrated capacity or deadline
guarantee. Failure knowledge and route rebuilding are immediate and centralized.
Upper TSN,shim and backhaul are abstract;mobility,cryptographic authentication
and distributed convergence are outside scope. The extended phase schedule
differs from the300-device study. See [configuration/run plan](access-500-device-plan.md)
and [ARP selection evidence](access-arp-overhead-results.md).

## Data and reproducibility

- Seed1: `results/access-auth-20261114T153317Z-b9c69ff0`
- Seed2: `results/access-auth-20261114T170446Z-a3aacca3`
- Seed3: `results/access-auth-20261114T181128Z-9496eaf3`

Each directory retains input,source/executable/library hashes,raw original/replay
traces,device data and audits. Final re-audits and a machine-readable summary:
`.recovery/access-arp/500-final/independent-audits.json` and `summary.csv`.
The old500-device wired viewer/report is a separate study. The wireless offline viewer and Linux installation guide are now available;
see wireless-quickstart.md and wireless-installation.md. No simulation
from this batch remains active.
