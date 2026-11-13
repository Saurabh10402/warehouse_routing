# Agreed access-routing architecture — 6 November 2026

This specification supersedes the older experimental wireless topology. Preserve
the earlier code, inputs and results as historical controls, not final-product evidence.

## Scope agreed with the user

Implement Layer 3 stationary wireless robots and access equipment, local peer
communication and Layer 2 spine routing. Use actual Wi-Fi and 5G-LENA NR radios.
Direct robot-to-robot radio and mobility are deferred. Main-server internals,
Layer 1 TSN, shim and upper 5G backhaul are external to this work. No zone edge
servers are part of this architecture. Target eight configurable zones and about
500 total local modeled devices, with delivery/latency and spine-failure evaluation.

Normal and approved payload stays below the external upper-network boundary.
The wired baseline dual-homes each zone routing gateway to the spines. Explicit
direct zone-gateway links form an editable chain in the examples, not a full mesh.
These are authorized implementation choices, not proven lowest-latency links.
Measure their benefit rather than assuming shorter paths always have lower delay.

## Cellular placement decision

Wi-Fi APs and NR gNBs are distinct access devices. Select one shared local EPC
support core below Layer 1, with SGW and PGW connected to both spines. Each gNB
connects to its zone gateway. The pinned NrNoBackhaulEpcHelper provides AddS1Interface
so the gNB-SGW tunnel can use explicit routed infrastructure instead of automatic
direct point-to-point bypasses. Internal S5 and control links remain explicit core
support infrastructure. This is EPC-supported NR, not a standalone 5G core.

Configuration: `placementStatus: shared-local-core-via-fabric`. Implementation
is still pending. Actual tunnel forwarding must be verified with packet traces.
This choice keeps one core and unchanged device counts. It does not pretend to
provide distributed per-zone cellular breakout. Same-zone NR traffic may traverse
spines and the core even though it stays below Layer 1. Direct zone links can
shorten Wi-Fi paths but need not shorten cellular paths. No hidden gateway delay
or unmodeled gNB shortcut is allowed.

Expected user-plane paths, not measured results:

- Wi-Fi same AP: robot -> AP -> robot.
- Wi-Fi cross-zone: robot -> AP -> gateway -> direct gateway link -> AP -> robot.
- Wi-Fi alternate: robot -> AP -> gateway -> spine -> gateway -> AP -> robot.
- NR to Wi-Fi: robot -> gNB -> gateway -> spine -> SGW -> PGW -> spine ->
  destination gateway -> AP -> robot.
- NR to NR: source radio/fabric -> SGW -> PGW -> SGW -> destination fabric/radio.

`scripts/access_paths.py` produces expected logical paths and infrastructure
links. It uses deterministic shortest-hop illustrations, not measured routing or
ECMP. Core and external nodes cannot become arbitrary transit shortcuts. Control
plane paths are not shown. Native IP/tunnel implementation must audit these paths.

## Main-server authorization

Every task type listed in `authorization.requiredTaskTypes` requires a request
to the MAIN SERVER, regardless of intra-zone or inter-zone placement. Unlisted
task types do not require authorization. Task names in the examples are illustrative,
not a hazard classifier. No task payload may be sent before a matching approval.
Requests cross a spine and the external boundary; replies return before local
payload communication starts. Denial or timeout blocks the task. Baseline approvals
are not cached between tasks. Late/duplicate responses need explicit handling in
the future state machine. Configuration validation does not execute this behavior.

The external service represents the excluded upper network and main server, with
configurable one-way delay, processing delay and allow/deny/timeout behavior.
It is not real authentication, cryptography, TSN or upper-backhaul simulation.
Example 10 ms one-way delay, 5 ms processing, 1000 ms timeout and 100 ms task
deadlines are illustrative, not requirements or measured performance.

Measure authorization round trip, post-approval payload delay and full task time
separately. Full time starts at the original task request; never hide authorization
waiting. Report denied, timed-out, undelivered and deadline-missed tasks separately
with offered-task denominators retained.

## Inputs, validation and limitations

`configs/warehouse_access_small.json`: 19 local modeled devices, eight robots.
`configs/warehouse_access_500.json`: 471 robots, 24 zone infrastructure devices,
two spines and three EPC entities, totaling 500. External boundary/main-server
models are outside this local count. Future core changes must update counts.

From the repository root (use python3 if needed):

```sh
python scripts/access_study.py configs/warehouse_access_small.json
python scripts/access_study.py configs/warehouse_access_500.json
python scripts/access_paths.py configs/warehouse_access_small.json
python tests/test_access_study.py
python tests/test_access_paths.py
```

These configs remain DESIGN ONLY, not inputs to the old native runner. There
are no measured authorization/direct-link/ECMP/failover results for this contract.
The old ARP queue shortfall remains historical evidence; cold-start address
resolution must be measured rather than hidden in the new tests.

## Next implementation gates

1. Implement explicit routed S1 backhaul with the shared local support core.
2. Implement small access/gateway/spine/direct-link topology and IP routes.
3. Implement per-task server authorization with allow/deny/timeout tests.
4. Audit identities, tunnel/payload paths, losses, timings and approval ordering.
5. Compare shortest path/ECMP and spine-failure behavior, then scale to 500.
6. Update visualization and local college report only from completed measurements.
