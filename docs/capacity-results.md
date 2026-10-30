# Capacity and admission results — checkpoint_007

## Validated evidence

The matrix was declared and committed in `8fe3f315881f197b3af4a7d381913b04602e117b`
before execution; see [study specification](capacity-study.md). No parameters were retuned.
The source manifest embedded in the completed batch exactly matched that committed source.
This report was prepared on 31 October 2026 (Asia/Calcutta); batch IDs use UTC.

- `experiments-20261030T175442Z-cb0b1ccc`: 288 runs, seeds 4/5/6, 60 charts, artifact checks and replay PASS.
- `experiments-20261030T175551Z-a5db7fd7`: 144 protected regressions, exact old inputs/packet traces, replay and 40 charts PASS.
- `experiments-20261030T180019Z-2441a69a`: 132 warehouse regressions, exact old inputs/packet traces, replay and 55 charts PASS.
- Both bootstrap/default core builds: 19/19 CTest PASS. Initial sandbox temp-file access failures were resolved by running the same tests with authorized external execution; no tests or warnings were disabled.
- Independent recomputation from all 288 raw packet files reproduces every class mean, worst seed, sample SD, delivery percentage and rejection-count mean in all 96 operating-limit rows.

Full [operating table](../results/experiments-20261030T175442Z-cb0b1ccc/operating_limits.csv)
and [endpoint source-time envelopes](../results/experiments-20261030T175442Z-cb0b1ccc/endpoint_envelopes.csv)
retain every setting. Per-scope/per-class outcomes and raw packet/queue traces remain in that batch.
These are illustrative stationary wired-link abstractions, not ns-3/NR measurements.

## Emergency protection helps, with costs

Percentages below are seed means. Every loss, including policing, counts as a deadline miss.
P0 aggregates all communication scopes; it is not just traffic entering the slow endpoint.
P3 delivery includes late packets, while P3 misses include both late delivery and losses.
Strict uses ECMP/strictPriority. Unpoliced keeps the same reservation and weighted service as
rate20, disabling only token admission. Rate20 is the original 20%/512-byte setting.

| Case | Policy | P0 misses % | P3 misses % | P3 delivery % |
|---|---|---:|---:|---:|
| mixed-c1 | strict | 15.278 | 25.056 | 79.889 |
| mixed-c1 | reserved-unpoliced | 1.944 | 25.111 | 79.667 |
| mixed-c1 | rate20 | 4.444 | 25.111 | 79.667 |
| mixed-c2 | strict | 13.889 | 23.056 | 85.222 |
| mixed-c2 | reserved-unpoliced | 0.000 | 23.556 | 83.000 |
| mixed-c2 | rate20 | 0.000 | 23.556 | 83.000 |
| mixed-c5 | strict | 9.861 | 6.611 | 93.389 |
| mixed-c5 | reserved-unpoliced | 0.000 | 7.833 | 92.167 |
| mixed-c5 | rate20 | 0.000 | 7.833 | 92.167 |
| burst-c5 | strict | 14.216 | 6.444 | 93.556 |
| burst-c5 | reserved-unpoliced | 6.250 | 7.778 | 92.222 |
| burst-c5 | rate20 | 10.294 | 7.667 | 92.333 |
| flood-c1 | strict | 41.764 | 28.889 | 74.833 |
| flood-c1 | reserved-unpoliced | 36.535 | 30.667 | 71.667 |
| flood-c1 | rate20 | 51.715 | 27.056 | 77.444 |
| flood-c5 | strict | 12.139 | 18.278 | 85.278 |
| flood-c5 | reserved-unpoliced | 1.285 | 24.889 | 79.500 |
| flood-c5 | rate20 | 46.688 | 10.833 | 89.167 |

At 1 Mbps mixed load, disabling policing lowers P0 misses from 4.444% to 1.944% while
preserving reservation. The ordinary slow endpoint generates 0.2304 Mbps of P0 outbound,
above its 0.2 Mbps policer budget. Six packets per seed are policed with rate20.
At 0.5 Mbps, rate20 is worse than strict: 20.139% versus 15.972% P0 misses.

At 5 Mbps flood load, rate20 polices a mean 2,241 P0 packets per seed: 46.6875% of 4,800 offered.
Individual policed counts are 1,894 / 2,543 / 2,286 for seeds 4 / 5 / 6.
Unpoliced reservation reduces P0 misses substantially, but P3 delivery drops to 79.5%.
The deterministic unit test also demonstrates that a sustained P0 backlog can entirely
starve admitted lower-class packets with policing disabled. Do not promote either setting
as a universally safe default.

## Observed operating limits

Smallest tested attachment capacity with **zero P0 misses in all three seeds**:

| Policy | Mixed traffic Mbps | Burst traffic | Flood traffic |
|---|---:|---|---|
| strict | None tested | None tested | None tested |
| reserved-unpoliced | 2 | None tested | None tested |
| rate10 | 5 | None tested | None tested |
| rate20 | 2 | None tested | None tested |
| rate40 | 2 | None tested | None tested |
| rate60 | 2 | None tested | None tested |
| burst128 | None tested | None tested | None tested |
| burst2048 | 2 | None tested | None tested |

“None tested” covers 0.5, 1, 2 and 5 Mbps only. These are observations over a finite run
and three seeds, not a guarantee between capacities or for a new workload. Zero P0 misses
does not mean acceptable service for other classes: at 2 Mbps, unpoliced reservation has
23.556% P3 misses. No tested policy eliminates P0 misses for the burst or flood workload.

The fixed 128-byte bucket fails to accommodate closely spaced 96-byte messages even when
average rate alone appears acceptable. A larger burst allowance changes transient admission,
but does not repair a sustained rate deficit: at 5 Mbps flood load, burst2048 still has
45.528% P0 misses. All eight settings, including these adverse cases, remain in the full table.

## Capacity and burst constraints

The mixed workload offers 9.4016 Mbps to the destination attachment across all classes:
0.1536 P0, 0.4096 P1, 1.6384 P2 and 7.2 P3. Even the 5 Mbps case cannot carry that entire
offered load indefinitely. Source-time P0 flood rates are 4.608 Mbps outbound and 3.072 Mbps
inbound at this endpoint; the latter counts intended arrivals before upstream losses.

| Attachment Mbps | Blocking by active 1200-byte frame ms | Serialization of 32 x 96-byte burst ms |
|---:|---:|---:|
| 0.5 | 19.2 | 49.152 |
| 1 | 9.6 | 24.576 |
| 2 | 4.8 | 12.288 |
| 5 | 1.92 | 4.9152 |

These arithmetic bounds exclude propagation and other queued traffic. A 32-message burst
cannot all meet 10 ms on a 2 Mbps link even in isolation. At 5 Mbps its serialization can
fit, but finite queue admission and the rest of the offered workload still cause misses.
Reservation creates protected waiting space; it creates neither link capacity nor preemption.

Envelope sigma/rho values are generation-time bounds. Network queuing can reshape downstream
arrivals, and ECMP affects shared uplinks. They are not a per-hop admission or deadline proof.
The study varies one attachment in both directions, not every uplink or reservation size.

## Next action

Keep the policies experimental and freeze these results. The next completion milestone is
to resolve the recorded ns-3 build failure without suppressing diagnostics, run a minimal
real packet example, and then implement/test schema-v2 forwarding and QoS before adding NR.
The old adapter syntax checks are not runtime proof. Physical warehouse traffic, burst/loss
requirements, access technology and deadline boundaries are still needed for calibration.
Further queue research should separately declare per-port budgets or burst/reservation-size
sensitivity; it must not silently reinterpret or retune this completed matrix.
