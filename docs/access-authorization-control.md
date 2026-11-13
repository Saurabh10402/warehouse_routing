# Main-server authorization control

The separate native task runner executes the task list from the agreed access
configuration. It retains actual NR/Wi-Fi access and routed cellular backhaul.
It does not replace earlier diagnostic peer controls or historical results.

`scripts/prepare_access_tasks.py` validates the JSON and produces node, direct-link
and task tables plus the exact input. Task indices in logs map to `tasks` array
order. Required authorization is derived from task TYPE membership, never zone.
Each task currently has one 256-byte UDP payload; this is not a general workload
generator. Start time is 1.2 seconds plus 10 ms per task index. One pending task
per table entry, no authorization reuse between tasks, no application retries.

## Network and gate

The main-server model is one additional external node connected to both spines.
Each server-facing link represents the excluded upper network with configured
one-way delay; server processing is separately scheduled. Thus there are 19
local nodes and 20 actual nodes in the small task control. No TSN/shim/upper5G
protocol implementation or cryptographic authentication is claimed.

Normal tasks send directly to the destination robot. Protected tasks send an
actual UDP request to server port8000. Server replies allow or deny after the
configured processing time; timeout mode emits no reply. Source checks server
IP/port, task index, socket identity and waiting state. Only matching approval
before the timeout releases local payload. Denial, no reply, or late reply
cannot release it. Payload uses ports20000+task index, requests/responses use
per-task source ports10000+index. Main-server links carry authorization only.

One short plain-text control exchange per task is a behavioral simulation, not
a secure protocol. Malicious traffic, cryptography, reauthentication, task
cancellation and large-scale concurrent authorization have not been evaluated.

## Building and running

From the project root in the prepared pinned Linux environment:

```sh
python3 scripts/prepare_access_tasks.py configs/warehouse_access_small.json /tmp/task-control
cp src/access-task-control.cc /path/to/ns-3/scratch/warehouse-access-tasks.cc
cp src/access-task-fabric.h src/access-authorization.h /path/to/ns-3/scratch/
cmake --build /path/to/build --target scratch_warehouse-access-tasks -j 2
/path/to/ns-3/build/scratch/ns3.46-warehouse-access-tasks-relwithdebinfo --inventory=/tmp/task-control/nodes.txt --links=/tmp/task-control/links.txt --tasks=/tmp/task-control/tasks.txt --output=/tmp/task-control/devices.csv > /tmp/task-control/run.log 2>&1
```

Use a fresh output directory/files. Change externalService.response to allow,
deny or timeout in separate input copies. All use real request traffic. The
small runner limits timeout to60 seconds and task count to1000, not a scale claim.
Configuration routing/failureStudy fields do not enable ECMP or failure events
in this control. Minimum-hop static routing remains the current implementation.

## Validation and measured results

Three task instances per mode: normal cross-zone Wi-Fi, protected same-zone
NR-to-Wi-Fi, and protected cross-zone Wi-Fi-to-NR. Each allow/deny/timeout case
and its replay exited0; full traces and device tables matched byte-for-byte.
Independent `scripts/audit_access_auth.py` checked task classification, request
arrival at the main-server link through a spine, approval-before-payload ordering,
zero blocked payload, payload absence on server links and timing arithmetic.
A tampered approval timestamp was rejected. This is not a complete per-packet
radio/S5 hop audit, independent security assessment or multi-seed performance study.

| Allow case | Full task ms | Payload-only ms | Example100ms deadline |
|---|---:|---:|---|
| Normal cross-zone | 15.991259 | 15.991259 | Met |
| Protected same-zone | 41.449187 | 7.170618 | Met |
| Protected cross-zone | 35.242855 | 4.151139 | Met |

Full time starts at the original task request and includes authorization waiting.
Deny and timeout cases each retain all three offered tasks: normal delivered,
two protected tasks blocked with zero payload sent/received. Blocked tasks have
no completion latency and are not counted as successful deadline completions.
These illustrative single-seed results do not guarantee industrial deadlines.

Raw cases/replays and audit.json: `.recovery/access-auth/`. Audit the three modes:

```sh
python scripts/audit_access_auth.py .recovery/access-auth
```

A separate late-response case uses1200ms server processing versus1000ms task
timeout. See checkpoint for observed outcome. Native allow-mode delivery exit
status can be nonzero when the deliberate late approval leaves tasks blocked;
the important negative-control outcome is no payload after timeout.

Next: make the runner workflow portable and consolidate configuration capabilities;
add live spine-failure recovery, ECMP evaluation, complete packet/deadline audits,
multi-seed controls, then scale and visualization. No500-device run yet.
