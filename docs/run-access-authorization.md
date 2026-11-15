> The500-device three-seed study is complete. See
> [results and limits](access-500-device-results.md) and the explicit
> [500-device run instructions](access-500-device-plan.md).

> Update, 13 November 2026: the 300-device measurement and full diagnostic
> replay are complete. See [300-device diagnosis](access-300-diagnosis.md).
> Earlier failures and incomplete diagnostics below remain historical evidence.

# Run the small authorization study

This command runs the implemented static or live-outage control on Linux or Ubuntu WSL. It uses
the existing pinned ns-3.46 + NR v4.1.1 dependency build; it does not install or
download dependencies. It is separate from the older wired demo workflow.

From the project directory inside Linux/WSL:

```sh
python3 scripts/run_access_authorization.py configs/access_authorization_control.json --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

Replace the two dependency paths on another machine. This is tested on this
Windows PC through Ubuntu WSL, not a fresh-install portability claim. Native
Windows execution is rejected because the validated NR runtime is Linux-based.

The command validates supported configuration, checks pinned source revisions
and the known Wi-Fi test-only patch, builds the native task target, preserves
input/source hashes, and records executable and linked ns-3 library hashes before
running. It executes original and replay runs, compares traces/device inventories
and checks authorization behavior. Results go to a new unique `results/access-auth-*`
directory, never an old run. A changed existing scratch source is saved before
replacement. Run only one build workflow against this dependency tree at a time.

`status.json` records RUNNING, PASS, FAILED or INTERRUPTED. Inspect `audit.json`
for task outcomes/times, `case/original.log` and `case/replay.log` for raw events,
`provenance.json` for dependencies, and build/prepare logs if anything fails.
Build wait is bounded to900 seconds and each native run to180 seconds; Linux
process groups are terminated on timeout/interruption. Logs remain available.
Power loss can leave RUNNING; that is not a successful result.

## Supported scope and configuration

Use `configs/access_authorization_control.json`, explicitly marked with
`fabric.failureStudy: none` and `routing: shortest-path`. The larger design files
lack an explicit outage schedule and are intentionally rejected by this runner.
Use `configs/access_spine_outage_control.json` for the validated live-outage
control below. ECMP is supported by the separate [ECMP control](access-ecmp-control.md).
The current conservative ceiling is64 local devices. It is a control limit, not
validated capacity. The demonstrated dataset remains19 local devices plus the
external main-server model, three static tasks or twelve phased outage tasks,
one256-byte payload per delivered task.

To test a denial or no-response case, make a separate JSON copy and set
`externalService.response` to `deny` or `timeout`. The configured normal task must
still deliver; protected tasks must send no payload. In allow mode, all configured
tasks must deliver for this control to pass. A delayed/lost approval can therefore
correctly produce a failed allow-delivery check while the fail-closed policy still
works; inspect raw events rather than treating every nonzero run as a software bug.

PASS means the stated behavior and exact replay passed. Outage PASS additionally
checks the scheduled events and actual phase-specific spine use. It is not proof
of protocol convergence, security, industrial deadlines, calibrated channel models,
or500-device performance. Deadline outcomes are in the audit and are not hidden
by the overall behavioral PASS. TSN/shim/upper5G remain external abstractions.

First maintained-command run passed with three delivered tasks and byte-identical
replay: `access-auth-20261106T185413Z-b9d0d304` (identifier is UTC). Earlier
allow/deny/timeout and late-reply controls remain in `.recovery/access-auth`.

The live outage control is now implemented and tested as described below.

## Stable addressing prerequisite

The task control assigns each infrastructure router a unique loopback /32 in
10.254.0.0/16. Physical /30 link addresses are next hops; Wi-Fi and NR robot
addresses retain their existing radio assignments. This separates the advertised
destination from a particular spine-facing interface. The main-server UDP socket
binds its stable address explicitly so replies retain the identity checked by
the requesting robot. This is routing identity, not cryptographic authentication.

`IDENTITY` records expose the assignments in raw logs. The authorization audit
checks unique identities and stable main-server request/reply addresses when
these records exist; historical pre-identity logs remain readable. The corrected
allow control `access-auth-20261106T190736Z-72af3943` delivered all three tasks
with exact replay and unchanged task times from the previous baseline.
The preceding diagnostic `access-auth-20261106T190541Z-256a6f8f` deliberately
remains preserved as a failed run: an unbound reply socket selected a physical
address, so both protected tasks timed out without sending payload. Explicit
server binding corrected this behavior.

Stable addresses alone do not implement recovery. The outage capability below
adds link shutdown, route rebuilding and restoration; protocol convergence timing
remains outside the model. ARP-drop logs now report
payload byte counts rather than misinterpreting task text as a sequence header.

Denial (`access-auth-20261106T190910Z-c0535c0a`) and timeout
(`access-auth-20261106T190931Z-89053a8b`) also passed with exact replays:
the normal task delivered, and both protected tasks sent zero payloads.

## Live spine outage and restoration

Run from the project directory in Linux/WSL:

```sh
python3 scripts/run_access_authorization.py configs/access_spine_outage_control.json --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

The checked-in configuration keeps all eight robots stationary and wireless
(four NR and four Wi-Fi), with two zones, two spines, six zone devices and three
EPC support nodes: nineteen local devices plus the abstract external server.
Tasks repeat before, during and after a live outage within one simulation.
They cover Wi-Fi cross-zone data, protected NR-to-Wi-Fi same-zone data,
protected Wi-Fi-to-NR cross-zone data, and normal NR-to-NR cross-zone data.

| Phase | Task starts (ms) | Expected server/NR spine |
|---|---|---|
| Before | 1200, 1280, 1360, 1440 | spine-0 |
| During | 2300, 2380, 2460, 2540 | spine-1 |
| After | 3800, 3880, 3960, 4040 | spine-0 |

The JSON adds an optional root `outage` object:

```json
"outage": {"spine": "spine-0", "downMs": 2000, "upMs": 3500}
```

Set `fabric.failureStudy` to `single-spine-outage` with this schedule. Each task
may specify `startMs`; omitted values preserve the static starts of1200+10*i ms.
Task starts are bounded to1000â€“60000 ms, after initial attachment time. The outage
requires1000 <= downMs < upMs <=60000 and an existing spine. Static mode rejects
an outage object; outage mode requires one in the maintained runner. The original
unscheduled design configurations remain design inputs, not runnable outage tests.

At2000 ms, both IPv4 interfaces on each incident link are administratively shut
down: five links, ten interfaces in the reference case. The route builder removes
its generated host routes and installs surviving shortest-hop BFS routes while
preserving connected, WLAN and helper-managed EPC routes. At3500 ms, interfaces
and the deterministic original routes return. Router identity addresses stay up.
`SPINE_EVENT` records interface counts and event times; `ROUTE_REBUILD` records
route counts. These complement actual UDP `WIRE` transmissions, not just a plan.

The audit checks task start times, authorization ordering and blocked payloads,
exact replay, down/up times, incident-interface counts, restored route counts,
absence of transmissions through the failed spine during the outage, and actual
main-server request plus NR tunnel traffic using the expected spine in all three
phases. Thus a successful outage control needs protected and NR traffic in each
phase; delivery alone cannot pass it. Payloads must remain below the server.
The separate no-direct-link case also demonstrates Wi-Fi traffic through spines.

### Executed controls

All runs below used the actual pinned ns-3/5G-LENA runtime on this Windows PC
through Ubuntu WSL. Each run has a byte-identical replay, raw events, device
inventory, source copies and executable/library hashes in its results directory.

| Control | Offered tasks | Delivered | Blocked | Audit/replay | Result directory |
|---|---:|---:|---:|---|---|
| Spine 0, allow | 12 | 12 | 0 | PASS | `access-auth-20261106T191625Z-ebf3b877` |
| Spine 0, deny | 12 | 6 | 6 | PASS | `access-auth-20261106T191816Z-2ae8fc2e` |
| Spine 0, timeout | 12 | 6 | 6 | PASS | `access-auth-20261106T191829Z-ea72f857` |
| Spine 0, no direct zone link | 12 | 12 | 0 | PASS | `access-auth-20261106T191839Z-ea853c40` |
| Spine 1, allow | 12 | 12 | 0 | PASS | `access-auth-20261106T191851Z-797ddf64` |
| Static baseline regression | 3 | 3 | 0 | PASS | `access-auth-20261106T191902Z-3436c690` |

The static regression reproduces checkpoint039 task results and timings exactly.
Denied/timed-out tasks are still included in offered counts and send no payload;
they are policy outcomes, not deliveries. Raw deadline outcomes remain per task.
These are small single-seed controls, not a multiple-seed performance study.

### Limits and next experiments

This is an administrative interface outage with **immediate centralized failure
knowledge and route rebuilding**. It does not implement OSPF/BFD, detection delay,
physical power loss or routing-protocol convergence. The control deliberately
places tasks away from event boundaries. Frames already in flight can finish
physical transmission, but arrivals on a down IPv4 interface are dropped; no
zero-loss claim is made for tasks spanning the transition. Large delays or altered
schedules may therefore fail the all-delivered gate legitimately. There is no
application retry for a lost request. A late approval cannot release payload.

The NR path still traverses the shared local EPC; this is not per-zone NR local
breakout. Upper TSN/shim/5G backhaul and main-server internals remain abstractions.
Detection-delay studies, ECMP-specific boundary-loss studies and500-device
wireless scaling are subsequent work. No wireless movement has been introduced.

## Transition-boundary control

The separate [boundary-loss study](access-boundary-control.md) now tests in-flight
request, reply and payload loss, plus approval crossing restoration. Use the
explicit `--boundary-control` flag; its PASS means correct loss/accounting, not
all-delivered. Fractional-millisecond timing is preserved at nanosecond resolution.

## ECMP routing

[The ECMP guide](access-ecmp-control.md) covers the implemented Layer-3 flow hash,
36-task example, independent actual-path audit, matched shortest-path comparison
and NR tunnel grouping limitation. Use `configs/access_ecmp_control.json`.

## Radio randomness and matched comparisons

Optional `radioRandomness: {"seed": 1, "run": 1}` preserves current defaults.
The [seed comparison guide](access-seed-comparison.md) documents the bounded
eight-case comparison, actual results, failure handling and default regressions.

## Four-zone control

[The four-zone guide](access-four-zone-control.md) documents the50-device layout,
all32 active robot sources, multi-seed results and two observed deadline misses.
Native inventory and per-phase source coverage now appear in every new audit.

## Eight-zone intermediate scale

The [100-device control](access-eight-zone-control.md) uses real stationary NR/Wi-Fi
access with 70 robot sources. Invoke its configuration with `--scale-control`;
the ordinary 64-local-device limit remains in place. This explicit path permits
at most 128 local devices and a 300-second native-run timeout, not a claim of
validated capacity at that ceiling. The guide also records the corrected ECMP
source-address audit and the limitations of earlier ECMP traces.

## 128-device runtime probe

The [128-device probe](access-128-device-probe.md) continues the eight-zone
stationary wireless controls with 98 active robots. Use
`configs/access_128_device_probe.json` with `--scale-control`. Its single seed
and replay test correctness and runtime; they do not establish multi-seed
performance or completion of the 500-device study.

## Current explicit scale bounds (checkpoint047)

The ordinary path remains limited to64 local devices and180 seconds per native
execution. With `--scale-control`, up to200 local devices are now accepted:
300 seconds per execution through128 local devices,600 seconds above128.
These are execution safeguards, not guaranteed performance or validated capacity.
The [200-device probe](access-200-device-probe.md) uses199 local nodes plus one
external main-server model. Older guides record the limits used at their milestones.
The1000-task cap remains unchanged.500-device configurations remain rejected.

## Current larger probe bounds (checkpoint048)

The explicit scale path now accepts up to300 LOCAL nodes:300 seconds through128,
600 seconds through200,1200 seconds above200. Ordinary64/180s and the1000-task cap
remain unchanged. These guards are not validated capacity or latency guarantees.
See [the300-device probe](access-300-device-probe.md), with299 local nodes plus
one external main-server model. Older sections record their milestone bounds.

## Measuring outcomes without hiding losses

Use the explicit `--outcome-measurement` flag to replay and audit native delivery
exit2. The [outcome guide](access-outcome-measurement.md) explains MEASURED status,
separate delivery/deadline flags, retained denominators and validated controls.
Strict default behavior is unchanged; do not combine this with --boundary-control.
