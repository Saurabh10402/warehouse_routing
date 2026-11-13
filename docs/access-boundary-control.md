# Traffic crossing a spine outage boundary

This small stationary ns-3.46/5G-LENA control tests what happens to traffic already
in flight. It complements the phase-separated outage controls; it is not a claim
that failover delivers every packet or meets every deadline.

## Run

From the project directory in Linux/WSL with the existing pinned radio build:

```sh
python3 scripts/run_access_authorization.py configs/access_boundary_control.json --boundary-control --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

`--boundary-control` explicitly selects loss/accounting validation. The native
program keeps its all-delivered exit code:2 when any task is not delivered.
The wrapper records original and replay exit codes, permits only0 or2 for this
mode, and then checks them against the observed outcomes. Any other native error,
crash, timeout, replay difference or failed audit still fails the run. Ordinary
runs retain their original delivery gate and never silently accept exit2.
A boundary PASS means that the loss and fail-closed behavior are verified,
**not** that all tasks delivered. `status.json` states this distinction.

## Workload and observed results

The topology remains19 local devices plus the external server model: stationary
NR and Wi-Fi robots, zone access/gateways, two spines and a shared local EPC.
Direct zone links are disabled in this control so cross-zone Wi-Fi payloads use
a spine. Twelve phase-control tasks retain mixed NR/Wi-Fi coverage. Three extra
protected Wi-Fi tasks exercise a reply crossing shutdown, a request crossing
shutdown, and a request crossing restoration. A31-packet Wi-Fi burst starts at
1998.0–2001.0 ms in0.1 ms increments. These deliberately chosen probes are not
calibrated warehouse traffic or a statistical performance study.

The failed spine is spine-0, down at1999.79 ms and restored at3500 ms. The shutdown
time was selected using a preliminary trace to place a data packet in flight on
a100-microsecond fabric link. This is a targeted functional test, not an unbiased
estimate of outage loss probability.

| Outcome | Offered-task count |
|---|---:|
| Delivered and within configured deadline | 43 |
| Authorization timeout; no payload sent | 2 |
| Payload sent, unreceived at stop, with observed interface drop | 1 |
| Total offered | 46 |
| Total deadline misses, including blocked/unreceived tasks | 3 |

The verified primary run is `access-auth-20261106T193539Z-236294af`. Raw packet
identity and receive-side `IP_DROP` records establish these actual losses:

| Traffic | Transmission before shutdown / drop after shutdown | Result |
|---|---|---|
| Data task18, port20018 | 1999.743055 / 1999.845343 ms | Dropped at spine-0; no receive |
| Approval reply for task12 | Before shutdown / 2003.345307 ms | Dropped at spine-0; client timed out |
| Authorization request for task13 | Before shutdown / 2005.346039 ms | Dropped at server-side down interface; client timed out |

Both timed-out protected tasks emitted zero payload. Task14 requested approval
before restoration and was received by the server after restoration, then
completed normally. The audit requires such a successful restoration-crossing
request as well as healthy deliveries before, during and after the outage.

A companion run, `access-auth-20261106T193705Z-60586053`, uses10.5 ms upper delay,
5.25 ms processing and1000.5 ms timeout. It also recorded43 deliveries, two
authorization timeouts and one observed payload drop, with exact replay.

## Timing correction discovered by the test

The first diagnostic, `access-auth-20261106T193104Z-52f6472f`, failed the independent
start-time audit: the integer ns-3 `MilliSeconds` overload truncated fractional
values. It remains preserved, and its timings are not accepted results.
`ControlMs` now converts task starts, outage events, service delays, timeouts and
deadlines to the nearest nanosecond, with positive half ties rounded upward.
The Python auditors use the same declared rounding rule. Static integer-time
controls retain exactly their previous task outcomes and timings.

The corrected exploratory2000 ms outage run,
`access-auth-20261106T193343Z-7e4491a3`, observed two authorization losses but no
payload loss. It motivated the targeted1999.79 ms timing above. The final boundary
auditor requires an actual payload loss, so this preliminary authorization-only
run is not a complete instance of the final boundary control.

## Validation and limits

Both accepted boundary runs have byte-identical original/replay logs and device
inventories. The audit matches every observed interface drop to a pre-outage
transmission using node, IP endpoints, UDP destination port and IP identification.
It checks timeout deadlines, approval ordering, task starts, restored routes,
source identity, payload isolation from the server, offered denominators and
native exit codes. Unreceived payloads are not assumed permanently lost unless
a drop is actually observed; the result retains both categories.

Healthy phase control `access-auth-20261106T193553Z-a9c0a19f` and static control
`access-auth-20261106T193603Z-f33a7f3e` pass, with task outcomes and timings exactly
matching checkpoint040. Tests include corrupted drop evidence, hidden deadline
misses, unauthorized payload, falsified native success, truncated start time and
nanosecond rounding. All29 access tests and21 standard project checks pass.

Failure knowledge and route rebuilding remain instantaneous and centralized.
The outage is administrative IPv4 interface shutdown, not physical power loss or
a routing-protocol convergence implementation. There are no application retries.
The deliberately lost probes use Wi-Fi endpoints; NR is exercised in the healthy
phase tasks, not independently correlated for a lost inner GTP payload here.
Single seed1/run1 only. Real upper TSN/shim/backhaul and cryptographic authentication,
NR-specific boundary-loss correlation, ECMP and500-device wireless scaling remain
outside this result. Earlier source manifests and all diagnostic data are retained.
