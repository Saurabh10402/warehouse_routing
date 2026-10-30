# Protected queue evaluation — 30 October 2026

## What was completed

The traffic and acceptance contract is in traffic-requirements.md. The implementation adds
reserved P0 waiting slots, a per-direction byte token-bucket admission policer and self-clocked
weighted service for admitted P1/P2/P3 packets. Exact semantics/defaults are in protected-queues.md.
The existing FIFO/strictPriority policies remain available and the default study is unchanged.

Tested implementation commit: `2d0dced`. Protected study:
`results/experiments-20261030T173430Z-db8fbaf2` — **144 runs PASS**, three seeds per scenario,
six policies, raw/fairness checks, deterministic replay and 40 charts. The 19-test core suite
checks the mechanism, invalid inputs, real CLI and queue accounting. The separate default
build regression batch is `results/experiments-20261030T173542Z-c74e4faf`: **132 runs PASS**,
every input matches and every packet trace is byte-identical to checkpoint_005's baseline.
Replay and 55 charts passed. Both core build presets pass 19/19 CTest tests. Final evidence
is recorded in checkpoint_006. Post-run edits only document observed results.

## Results: benefit and cost at identical offered traffic

Values below are means across seeds 1,2,3. All losses, including rate-policed and reserved-space
rejections, count as deadline misses. P0/P3 percentages aggregate the three communication scopes;
scope-specific results follow the table. These are illustrative link-model results.

| Scenario | P0 misses: ECMP priority % | P0 misses: ECMP protected % | P3 misses: priority % | P3 misses: protected % | Mean P0 packets policed by protected |
|---|---:|---:|---:|---:|---:|
| Uplink contention | 0.833 | 0.000 | 0.333 | 0.778 | 0 |
| Destination bottleneck | 13.750 | 4.583 | 25.111 | 25.167 | 6 |
| Emergency overload | 1.500 | 25.562 | 5.889 | 3.778 | 1227 |
| Synchronized emergency burst | 24.020 | 15.074 | 25.056 | 25.222 | 35.667 |

For destination bottleneck P0 misses, sample SD is 0.417 percentage points for ECMP priority
and 1.502 for protected. Full seed rows and sample SD for aggregate metrics are preserved.
Three seeds are limited variability evidence, not a deployment confidence guarantee.

**Reservation helps, but the full policy is not universally better.** At the destination
bottleneck, cross-zone P0 misses drop from 41.25% to 6.25%. However, same-zone P0 misses rise
from zero to 2.5% and edge-service P0 misses from zero to 5%. Queue diagnostics show six P0
packets per seed policed on the slow endpoint's outgoing `zone1-ep0 -> leaf1` direction.
That endpoint offers 0.2304 Mbps of ordinary P0 traffic, exceeding its fixed 0.2 Mbps admission
budget despite the link's 1 Mbps total capacity. The ceiling applies without congestion.

During emergency overload, ECMP protected polices 1,227 of 4,800 offered P0 packets per seed.
That is 25.5625% P0 misses before considering any other cause. Admitted lower-class service
improves, but the emergency objective is worse than the strict-priority baseline. This is
an explicit adverse result, not hidden by reporting success only for admitted traffic.

The burst case cannot meet every 10 ms deadline: its 32 extra 96-byte messages alone require
24.576 ms on the 1 Mbps destination attachment. Protection does not create capacity or preempt
an already-transmitting background frame. All nominal communication runs meet their deadlines.

## One-factor ablations

With the destination-bottleneck input otherwise unchanged, removing the reserved P0 slots
increases ECMP protected's aggregate P0 misses from 4.583% to 16.806%. Keeping reservation
but changing lower-class weights from [4,2,1] to [1,1,1] gives 5.139%. The study supports the
importance of buffer admission in this scenario; it does not establish optimal weights.

Protected reservation also rejects lower-class arrivals: on the default destination case,
ECMP protected averages 225 `reserved_capacity` losses per run. They are counted in delivery
and deadline metrics. P1/P2/P3 may still suffer admission loss even though admitted packets
receive weighted service. There is no claim that every offered flow is starvation-free.

## Validation evidence and interpretation

Deterministic tests verify the 8.8 ms completion of a reserved P0 packet behind an active
frame; 100-byte burst/25-byte-per-ms refill; independent directions; refill after idle;
byte-weighted service with unequal packet sizes; progress under a P0 flood; and admission
and queue-occupancy conservation. Invalid rates, weights, burst allowances, oversized packets, reservations
and unsupported EALR/protected combinations are rejected. CLI tests exercise the actual JSON
configuration and exported queue counters. No tests or compiler warnings were disabled.

Read `scope_priority_comparison.csv` for loss causes and scope/class outcomes, `queue_stats.csv`
within each run for admission/service observations, and raw `packets.csv` for actual paths and
times. Queue waits cover service starts only; queued-at-cutoff/rerouted samples are censored.
Packet traces are the end-to-end measurement. Bytes started are not delivered throughput.

## Next step toward completion

Keep this policy experimental. Before selecting a default, evaluate admission rate/burst
sensitivity and per-port critical-traffic envelopes, together with endpoint/uplink capacity.
Use a declared sweep and additional/held-out seeds, retaining the current settings as reference.
Compare an always-on ceiling against a separately specified congestion-conditional policy
if appropriate; do not silently reinterpret this implementation after seeing the results.

Actual warehouse traffic/loss/availability requirements still need calibration. ns-3/NR
build/runtime integration and genuine radio/user-plane validation remain separate unfinished
work. The current evidence is a wired abstract-link model, not a private-5G performance claim.
