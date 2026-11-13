# checkpoint_043 — Configurable radio randomness and matched comparison

Optional radioRandomness {seed,run} added to access contract; strict integers
1..2147483647, default1/1. Native CLI sets RNG before construction, emits effective
RADIO_RNG values. Runner checks native evidence and records it in provenance,
audit and status. ECMP hash unchanged. No topology/task randomization added.

New scripts/run_access_seed_comparison.py declares and writes eight inputs before
execution: seeds1/2/3 at run1, seed1/run2, both shortest-path and ECMP per setting.
Unique batch, per-case original/replay, JSON/CSV summary and explicit failure/
interruption states. Failed cases retain offered count and error, not omitted.
Subprocess failures handled as failed cases. No unbounded/full matrix claim.

Actual batch results/access-seeds-20261107T062330Z-43014f26 PASS8/8 cases.
All288 offered tasks delivered and met100ms deadlines; every replay byte-identical.
Each pair differs only by routing policy and has identical per-task results/times.
Median/max ms (both policies):
- seed1/run1:1.2494385 /45.002193
- seed2/run1:1.250202 /41.177193
- seed3/run1:1.2476645 /47.095054
- seed1/run2:1.249192 /44.975193
Radio variation observed, no ECMP latency advantage in this small workload.
Replays are not extra independent samples. Three seeds plus a run check, not
statistical reliability or industrial deadline guarantees.

Default-omission regressions:
- results/access-auth-20261107T062609Z-4595d670:ECMP36/36 PASS.
- results/access-auth-20261107T062629Z-b36612da:boundary43/46,2 auth timeouts,
  1 observed payload drop,3 deadline misses, loss-accounting PASS.
Traces equal checkpoint042 references byte-for-byte except the new RADIO_RNG line;
CSV inventories identical. Receipts .recovery/access-seeds/regression-receipt.json.

Validation:40/40 access tests incl invalid RNG, mismatched native settings,
paired inputs, failed-case retention, subprocess failure and interruption;
required scripts/verify.py21/21 PASS (.recovery/verify-radio-seeds.log).
Failure/interruption test outputs are deliberate temporary fixtures, not failed
native experiments. No upstream-suite rerun. No background process active.

Docs: docs/access-seed-comparison.md plus links from authorization/ECMP guides.
Source checkpoint_043; separate checkpoint_043-radio-seeds.zip recovery package.
No Git commit/push; previous data/manifests preserved. Small19-local+1-external
stationary NR/Wi-Fi model remains distinct from500-device wired demonstration.

Limits: fixed ECMP L3 hash, GTP grouping, immediate centralized failure knowledge,
uncalibrated traffic, no mobility/real upper-network/crypto/large-scale capacity.
ECMP-specific and inner-NR transition-boundary loss still pending.

Next exact action: expand to four active zones with explicit inventory/traffic,
about49 local+1 external devices (32 robots,12 zone devices,2 spines,3 EPC).
Keep the64-local-device guard; ensure all zones and both access technologies carry
traffic. Validate authorization, route paths/outage recovery and replay before
increasing toward500. Preserve current small baseline and its targeted loss case.
