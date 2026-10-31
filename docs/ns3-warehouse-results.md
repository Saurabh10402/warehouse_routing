# Initial native wired warehouse validation - checkpoint_009

Tested implementation: `49d4f4538873ec2619b52b726e70d1c7693f8ccd`. This milestone implements
the first stationary schema-v2 wired FIFO/Dijkstra slice. It preserves the independent core
study and does not implement native priority scheduling, ECMP, dynamic failures or NR.
See [the supported model and workflow](ns3-wired-warehouse.md) before interpreting results.

## Executed evidence

- Native linked build: **24/24 CTest PASS** on Windows 11, GCC 14.2, Boost 1.85 and the
  previously verified ns-3.46.1 wired Release installation.
- All **seven deterministic acceptance cases**, independent raw audits and replay PASS in
  `results/ns3-warehouse-20261031T183840Z-7256a26e`.
- A separate source copy with a path containing spaces, no `.git` and no previous project
  build compiled successfully, passed **24/24 tests**, and completed all seven cases/replay.
  Its batch is `results/ns3-warehouse-20261031T185300Z-b07383e3`, copied unchanged for recovery.
  All **35 measurement artifacts** match the main batch byte-for-byte. Compiler, Boost and
  ns-3 were supplied externally on the same Windows host; this is not a Mac/Ubuntu test.
- The complete core regression passed **132 exact input/packet-trace comparisons** against
  `experiments-20261031T094745Z-723bc3c8`, replay and 55 charts. The new batch is
  `results/experiments-20261031T173020Z-a8e5dbed`. The simulator and primary matrix were unchanged.

These are seven deterministic native cases, not seven seeds or a stochastic performance
study. The three-seed regression concerns the separate abstract-link core model.

## Observed native outcomes

| Case | Offered | Delivered | Deadline misses | Queue drops | No route | Censored at cutoff |
|---|---:|---:|---:|---:|---:|---:|
| Communication paths | 3 | 3 | 0 | 0 | 0 | 0 |
| FIFO overload | 3 | 2 | 3 | 1 | 0 | 0 |
| Short drain/cutoff | 3 | 0 | 3 | 1 | 0 | 2 |
| Disconnected destination | 3 | 0 | 3 | 0 | 3 | 0 |
| Full duplex overload | 6 | 4 | 6 | 2 | 0 | 0 |
| Size-sensitive routing | 2 | 2 | 0 | 0 | 0 | 0 |
| Endpoint transit exclusion | 2 | 2 | 0 | 0 | 0 | 0 |

The communication packets followed the observed physical paths A0->leafA->A1,
A0->leafA->spine->leafB->B0, and C2->leafC->spine->edge. Their delays were respectively
**4.416, 8.832 and 6.624 ms**, matching unloaded 5 Mbps / 2 ms links including UDP/IPv4/PPP
bytes. Node positions remained unchanged in all cases.

The overflow case shows why delivery alone is insufficient: two packets arrived but both
missed their 20 ms target; the dropped third packet also counts as a deadline miss. The
cutoff case distinguishes an observed drop from two packets still queued/in flight.
None of these observations establishes warehouse capacity or a guaranteed deadline.

The independent Python audit reconstructs paths and per-hop timing from native events;
checks FIFO order, per-direction serialization, finite capacity, admission/service/remaining
queue conservation and peak occupancy; and recomputes offered counts, deadlines, positions
and IP/application totals. CLI tests intentionally corrupt summary counts and wire bytes
to verify that those audits fail. Existing-output refusal and unsupported-input rejection
also passed. Unsupported inputs leave no result directory.

## Interrupted test and recovery record

One initial full invocation recorded a **3913.74 s wall-clock timeout** in the native CLI
test during an interruption. Its log is retained. The unchanged targeted retry passed in
3.66 s, followed by the full successful 24-test/native-case workflow and an independent fresh
build. No timeout was increased and no test or warning was disabled; a reproducible code
failure was not established by that interruption.

Raw data, source/executable/library hashes, exact inputs, dependency setup receipts and
successful status files remain with each batch. Checkpoint evidence contains the logs and
cross-source comparison receipt. Recovery packages preserve the main native batch, fresh
native batch and full core regression separately with source and Git history.

Next: native strict-priority service and per-flow ECMP, retaining these FIFO controls and
the existing core results. Then add explicitly tested failures/backup services, protected
queues and compatible NR user-plane paths. Mac M4 and Ubuntu ARM64 execution, workload
calibration and final deployment claims remain open.
