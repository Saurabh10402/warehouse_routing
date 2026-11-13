# Four-zone stationary wireless control

> **Historical ECMP source-address caveat:** the ECMP results below predate the
> correction described in [the eight-zone guide](access-eight-zone-control.md).
> Some outer NR tunnel packets used ns-3's uninitialized source-address sentinel.
> Their delivery records remain preserved, but they do not satisfy the stronger
> source-validity audit. Use the corrected controls for current ECMP validation.
> The shortest-path results and documented cold-ARP delay are not invalidated.


`configs/access_four_zone_control.json` expands the native access study to four
active zones. Counts, positions, attachments, tasks and outage times are explicit
JSON data; no movement or wired robot substitution is introduced.

| Device type | Per zone | Total |
|---|---:|---:|
| NR robots | 4 | 16 |
| Wi-Fi robots | 4 | 16 |
| Wi-Fi access point | 1 | 4 |
| NR gNB | 1 | 4 |
| Zone routing gateway | 1 | 4 |
| Shared spines | — | 2 |
| Shared local EPC support: PGW, SGW, MME | — | 3 |
| External main-server model | — | 1 |
| **Total** | | **50** |

There are49 local modeled devices plus one external server model. The existing
64-local-device guard stays in place. This is a larger functional control, not a
500-device radio capacity result. All32 robots use actual ns-3 NR or Wi-Fi device
types. Native checks verify NR connection/cell and Wi-Fi association/BSSID.

## Run

From the project directory inside Linux/WSL with the existing pinned radio build:

```sh
python3 scripts/run_access_authorization.py configs/access_four_zone_control.json --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

For the matched eight-case comparison:

```sh
python3 scripts/run_access_seed_comparison.py configs/access_four_zone_control.json --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

The matrix uses seeds1/2/3 at run1 and seed1/run2, with shortest-path and ECMP for
each setting. Every case has an original/replay and the existing authorization,
path, failure, RNG and inventory audits. See [the run guide](run-access-authorization.md)
for dependencies and result-file meanings. This does not install a fresh toolchain.

## Traffic and failure schedule

Zones A/B/C/D each have four robots of each access type. Every robot offers a
cross-zone task to the next zone using the other access type. Each zone also has
protected same-zone NR-to-Wi-Fi traffic, normal local Wi-Fi traffic, cross-zone
Wi-Fi peer traffic and cross-zone NR peer traffic. There are48 tasks per phase,
144 overall:60 protected tasks and84 normal tasks. Protected tasks always obtain
main-server approval, then send payload locally below the upper-network boundary.

| Phase | Task starts (ms) | Spine availability |
|---|---|---|
| Before outage | 1200–2610, every30ms | Both |
| During outage | 3300–4710, every30ms | spine-1 only |
| After restoration | 5300–6710, every30ms | Both |

Spine0 is administratively down at3000ms and restored at5000ms, with immediate
centralized route rebuilding. The main configuration has no direct zone links.
A separate ring-link control connects A–B, B–C, C–D and D–A. Wired infrastructure
remains the approved baseline; robots are wireless in both variants.

## Coverage audit

`scripts/audit_access_coverage.py` compares actual native device IDs, roles,
radio types, attachments and total node count against the expanded inventory.
The helper-created three EPC nodes and external server are included in the native
total count; the per-device CSV covers the explicitly constructed zone/spine/robot
nodes. It rejects missing devices, duplicate node IDs, incorrect radio types,
wrong attachments and mismatched task results.

The audit reports each phase's offered sources, payload sources and sources whose
payload arrived, broken down by zone/access for offered traffic. These counts
remain separate so a correctly blocked task is not described as a delivery.
The general runner does not require every robot to offer traffic in every study;
it reports `allRobotsOfferedInEveryPhase` and missing source IDs. This four-zone
control explicitly checks that every phase covers all32 robots. In allow cases,
all32 also send and deliver payload in each phase.

## Interpretation limits

Tasks are staggered, with small one-shot256-byte payloads. This validates topology,
radio attachment, routing, approval and recovery at a larger device count; it is
not a saturated-load or real warehouse interference calibration. All zones share
the configured radio model and remain stationary. No movement, cryptographic
identity, real upper TSN/shim/backhaul, routing-protocol detection delay or per-UE
inner-GTP load balancing is introduced. The earlier targeted boundary-loss study
remains separate; this traffic is placed away from outage event boundaries.

## Executed multi-seed comparison

Batch `access-seeds-20261107T063613Z-8dbf345b` passed all eight cases with exact
replay. All32 robot sources delivered in every phase in every case. Every native
record reports49 local devices and50 nodes including the external server.

| Seed | Run | Policy | Delivered/offered | Deadline misses | Median task ms | Max task ms |
|---:|---:|---|---:|---:|---:|---:|
| 1 | 1 | shortest-path | 144/144 | 0 | 10.260712 | 48.967749 |
| 1 | 1 | ecmp | 144/144 | 0 | 10.260712 | 48.967749 |
| 2 | 1 | shortest-path | 144/144 | 0 | 9.315095 | 47.786678 |
| 2 | 1 | ecmp | 144/144 | 0 | 9.315095 | 47.786678 |
| 3 | 1 | shortest-path | 144/144 | 0 | 9.430791 | 48.703343 |
| 3 | 1 | ecmp | 144/144 | 0 | 9.430791 | 48.703343 |
| 1 | 2 | shortest-path | 144/144 | 1 | 10.182514 | 979.171427 |
| 1 | 2 | ecmp | 144/144 | 1 | 10.182514 | 979.171427 |

Total:1152 offered tasks,1152 delivered,1150 met100ms and2 missed. The same
task in seed1/run2 took979.171427ms under both policies. Delivery/replay PASS
does not imply all deadlines met. Replays are not counted as
additional independent samples. This is a bounded multi-seed functional check.

The initial single ECMP control `access-auth-20261107T063414Z-3ad81450` also
delivered144/144. Its independent path audit checked1272 wire hops,296 equal-cost
selections,32 repeated multipath-flow groups and228 restored flow mappings.

## Deadline-miss diagnosis

In seed1/run2, task6 (`before-A-wifi-2`) takes979.171427ms under each policy.
It is a protected Wi-Fi-to-NR task with a100ms configured deadline. The server
receives its request at1398.791733ms and grants approval at1403.791733ms.
The reply reaches zone A's AP at1413.992621ms, where the ARP entry for
192.168.0.4 is unresolved. The initial ARP request at1416.992621ms does not yield
a completed exchange before the retry at2356.118889ms. The AP receives the reply
and flushes its pending packet at2356.403224ms, after942.410603ms waiting.
The robot receives approval at2356.691266ms; payload then takes2.480161ms.

The diagnostic reuses the recorded executable hash, configuration and seed/run,
changes only ARP logging, and reproduces the native stdout trace and device CSV
byte for byte. Evidence is retained in `.recovery/access-four-zone/arp-diagnostic`.
The waiting/retry mechanism is established; the PHY/MAC reason for the initial
exchange failure is **not** established by these ARP-only logs. No ARP settings,
deadlines, cache prewarming or workload were changed to remove the misses.
The1000ms authorization timeout was not exceeded, so approval correctly released
payload, while the separate100ms task deadline was counted as missed.

## Additional controls and regression

| Control | Offered | Delivered | Correctly blocked | Result |
|---|---:|---:|---:|---|
| Denied approval | 144 | 84 | 60 | `access-auth-20261107T111938Z-a86281cc` |
| No approval response | 144 | 84 | 60 | `access-auth-20261107T112034Z-1ee684ed` |
| Direct gateway ring links | 144 | 144 | 0 | `access-auth-20261107T112131Z-9b89519e` |

Each has exact replay and all32 offered sources per phase. Blocked tasks emit no
payload and are not counted as deliveries. The ring-link run meets all144
configured deadlines for its seed1/run1 setting; it is not evidence that adding
links fixes the separate seed1/run2 ARP delay.

The earlier small ECMP control also passed as
`access-auth-20261107T112246Z-46b7028e`, reproducing its checkpoint043 native trace
and device CSV exactly. All46 access tests and21 standard project checks pass.
Coverage tests reject wired robot substitutions, omitted devices, wrong native
node counts and missing task results. No native model or dependency was changed
in this milestone: the additions are configuration, coverage auditing and results.

Next validate an explicit eight-zone layout around100 devices before approaching
500. Larger tests must continue reporting deadline misses honestly. The present
64-local-device guard has not been raised by this milestone. The known ARP delay
and the unresolved first-exchange PHY/MAC cause remain documented limitations.
