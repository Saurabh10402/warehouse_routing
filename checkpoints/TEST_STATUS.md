# Initial test status

| Area | Status | Evidence |
|---|---|---|
| Original archive preserved | PASS | SHA256 in CURRENT_STATUS.md |
| Bootstrap configure | BLOCKED | cmake not on PATH |
| Core C++ tests | NOT RUN | Compiler absent |
| ns-3 / NR | BLOCKED | Dependencies absent |
| Primary packet experiment | NOT IMPLEMENTED | Runner only sets up nodes/mobility/animation |
| Warehouse variable zone sizes | FAIL (inspection) | zoneCount * endpointsPerZone enforced |

## checkpoint_002
PASS: 15/15 CTest tests (core, legacy routing/configuration/metrics, fabric configuration/forwarding/metrics).
PASS: exact serialization/propagation delay; priority overtaking; queue overflow; node/link/edge failover; in-flight failure; recovery; energy exclusion; stationary invariant; safety without cloud; percentile samples and deadline boundaries.
NOT RUN: experiment matrix and exported CLI artifacts.
BLOCKED: ns-3 / 5G-LENA.
