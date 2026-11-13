# Matched multi-seed stationary radio comparison

> **Historical ECMP source-address caveat:** the ECMP results below predate the
> correction described in [the eight-zone guide](access-eight-zone-control.md).
> Some outer NR tunnel packets used ns-3's uninitialized source-address sentinel.
> Their delivery records remain preserved, but they do not satisfy the stronger
> source-validity audit. Use the corrected controls for current ECMP validation.
> The shortest-path results and documented cold-ARP delay are not invalidated.


The small native NR/Wi-Fi access study now accepts an optional JSON field:

```json
"radioRandomness": {"seed": 1, "run": 1}
```

Both values must be integers in1–2147483647. Omission retains seed1/run1.
They configure ns-3 random streams; `run` chooses the stream run/substream.
The ECMP flow hash is unchanged and is not reseeded. Tasks, topology, policies,
positions and outage schedule remain explicit configuration, not randomly changed.
The native process sets RNG values before node/radio construction and emits
`RADIO_RNG seed run`. The maintained runner checks that record against the JSON
and records the effective values in provenance, audit and status output.

## Reproduction

From the project directory in Linux/WSL with the existing pinned ns-3/NR build:

```sh
python3 scripts/run_access_seed_comparison.py configs/access_ecmp_control.json --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

This bounded comparison declares eight cases before executing any simulation:
seeds1/2/3 at run1, plus seed1/run2; shortest-path and ECMP for every setting.
It overrides only the base input's radio randomness and routing policy. Each
policy pair has identical tasks, starts, deadlines, topology and failure events.
The extra run2 case checks the separate run setting; it is not a fourth seed.
Every case invokes the existing build/run/replay/authorization/path workflow.

A unique `results/access-seeds-*` directory contains `plan.json`, all eight input
JSON files, `status.json`, `summary.json`, `summary.csv` and complete per-case
result directories. The latter retain source snapshots, dependency/binary/library
hashes, actual device inventories, original/replay logs and audits. Every native
case remains bounded by the existing build and execution timeouts.

Failed cases remain explicit rows with their offered count and error; they are
not omitted from the comparison. Recoverable case failures allow remaining cases
to run but make the batch FAILED. Interruption makes it INTERRUPTED; power loss
may leave RUNNING, which is not success. Replays verify determinism and are not
counted as additional independent observations. The command is deliberately
small and fixed, not a full traffic/failure/capacity experiment matrix.

## Executed comparison

Batch: `access-seeds-20261107T062330Z-43014f26`, using the actual pinned ns-3.46 /
NR v4.1.1 runtime through Ubuntu WSL on this Windows PC. There are19 local devices
plus the external main-server model, eight stationary wireless robots and36 tasks
per case. A spine outage/restoration occurs within each run. Both policies retain
all authorization and actual-path checks, including ECMP flow consistency.

| Radio seed | Run | Shortest-path delivery | ECMP delivery | Median task latency, both policies (ms) | Maximum, both policies (ms) |
|---:|---:|---:|---:|---:|---:|
| 1 | 1 | 36/36 | 36/36 | 1.249439 | 45.002193 |
| 2 | 1 | 36/36 | 36/36 | 1.250202 | 41.177193 |
| 3 | 1 | 36/36 | 36/36 | 1.247664 | 47.095054 |
| 1 | 2 | 36/36 | 36/36 | 1.249192 | 44.975193 |

All288 offered tasks across the eight cases delivered within the configured100ms
deadlines. Every case had a byte-identical replay. For each setting, per-task
outcomes and times were identical between routing policies, not just the summary
statistics. The timing differences across settings show that the radio RNG controls
are active. This small workload does not establish an ECMP latency advantage.

Three seed values plus the run-setting check provide a bounded multi-seed control,
not a statistical reliability guarantee, deployment calibration or proof of
industrial deadlines. Fixed topology/traffic and small sample sizes remain important
limits. No confidence intervals or claims about rare-event loss are inferred.
The ECMP hash remains fixed; this is radio randomness variation, not a hash-seed sweep.

## Defaults and regressions

Configurations without `radioRandomness` were rerun separately:

- ECMP: `access-auth-20261107T062609Z-4595d670`,36/36 delivered.
- Targeted shortest-path boundary loss: `access-auth-20261107T062629Z-b36612da`,
  43/46 delivered, two authorization timeouts, one observed payload drop.

Both retain their previous default traces byte for byte after removing only the
new `RADIO_RNG` metadata line; device inventories are byte-identical. The boundary
losses remain deadline misses and are not erased by the successful comparison.
The eight main cases use explicit seed settings; these two extra regressions test
omission/default behavior and are not counted among the288 comparison tasks.

All40 access tests and21 standard project checks passed. Tests reject invalid
seed/run values, missing or duplicate native RNG evidence, silently discarded
failed cases and false success after interruption; they also verify matched inputs.
No full upstream suite repeat was needed for this project-only extension.

## Remaining scope

This remains the small stationary real NR/Wi-Fi model: no movement, calibrated
traffic, large-scale radio capacity result or real upper TSN/shim/5G implementation.
ECMP uses Layer-3 flow grouping, with GTP tunnels kept together. Recovery assumes
immediate centralized knowledge. ECMP-specific/inner-NR boundary loss is pending.

Next extend the explicit inventory and traffic to four active zones (about50 total
modeled devices), within the current64-local-device guard, and validate routing,
authorization, outage recovery and reproducibility before moving toward500 devices.
The existing500-device wired demonstration is a separate result.
