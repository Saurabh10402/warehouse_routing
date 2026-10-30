# Declared capacity and admission study

Declared before running the new matrix. This is a stationary wired link abstraction,
not measured Ethernet, ns-3 or NR. No settings will be selected by hiding failed runs.

Three workloads use the existing destination-bottleneck topology and traffic: mixed,
mixed plus the existing synchronized 32-message burst, and mixed with every P0 flow
changed to interval 0.5 ms / initial jitter 0.1 ms. This last flood differs from the
previous uplink-only emergency-overload case: it also has a slow destination attachment.
Each uses attachment capacity 0.5, 1, 2 or 5 Mbps in both directions at zone1-ep0.
Uplinks stay 10 Mbps; buffers, payloads, deadlines and other traffic stay fixed.

Eight ECMP policies: strict priority; reserved space/weighted lower service with
policing disabled; policing fractions 0.1, 0.2, 0.4, 0.6 with 512-byte burst; and
128/2048-byte burst at fraction 0.2. All protected variants reserve 8 waiting slots
and use lower-class byte weights 4:2:1. Rate/burst changes are one-factor comparisons
to rate20, not a full joint optimization. There is no per-port parameter adaptation.
Seeds 4,5,6 are new relative to prior studies: 12 cases x 8 policies x 3 = 288 runs.
Quick mode uses seed 4 only and is not multi-seed evidence.

`emergencyPolicingEnabled` defaults to true, preserving existing inputs/behavior.
When false, P0 skips tokens and retains nonpreemptive strict service and finite queue
admission. Reservation and lower-class weighting stay enabled. P0 may then starve
lower classes. Rate/burst fields still require valid numeric values but are inactive;
the packet-versus-burst-size restriction applies only with policing enabled.

Run `python scripts/run_all_experiments.py --suite capacity-admission`.
Every policy uses identical offered traffic per case/seed, with all losses counted as
deadline misses. Export raw traces, per-scope/class results, queue accounting, replay,
source/executable provenance, all 96 policy/capacity rows in operating_limits.csv,
and endpoint_envelopes.csv. Report means, sample SD, worst seed, every class's delivery,
and all rejection causes. Zero P0 misses across tested seeds is an observed criterion,
not a deployment guarantee or an invented acceptable loss requirement.

Endpoint envelopes use source-time generation: sigma is one payload per participating
P0 flow (including all one-shot burst flows); rho is the sum of periodic payload rates.
Ingress counts intended primary destinations, before upstream losses. Sigma + rho*t
bounds generation in an interval, not arrival after network queuing. Compare rho with
each directional policer rate; do not interpret rho below budget as sufficient for
deadline success, buffer admission, burst compatibility or downstream admission.
No uplink envelope is inferred without accounting for routing and queuing.

Regression must also preserve the complete earlier protected-qos input/packet traces;
existing warehouse results remain valid and immutable. Run both build presets. Real
traffic calibration, uplink/reservation-size sensitivity and ns-3/NR remain separate work.
