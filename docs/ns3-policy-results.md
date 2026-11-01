# Native wired priority and ECMP results - 1 November 2026

Implemented and tested source: commit 3e97cd0. The [predeclared comparison](ns3-policy-study.md)
defines the traffic, queue discipline, byte conventions, transport hash and model limits.
These are actual ns-3 wired IPv4/UDP measurements, not NR or deployment measurements.

## Evidence

- Main: results/ns3-warehouse-20261031T215600Z-dd1a4b9a.
- Independent fresh source: results/ns3-warehouse-20261031T215445Z-85e4fb40 (copied unchanged
  from .recovery/ns3 policy clean 43ecf17c).
- Each build passed 24/24 project tests, 12 acceptance controls and all 36 declared policy
  runs. Every one of the 48 cases was replayed and independently audited.
- All 48 exact inputs and 240 measurement artifacts match byte-for-byte between builds;
  aggregate tables also match. Fresh provenance correctly reports a source archive.
- The seven checkpoint_009 FIFO controls match on every pre-existing output field. New
  wire header columns additionally verify actual IP addresses, UDP ports and DSCP.
- Core: results/experiments-20261031T215045Z-6af82532 passed 20 tests and all 132 exact
  baseline input/packet comparisons versus checkpoint_009, replay and 55 charts. These
  remain abstract-link results.
- Identical offered traffic across policies and scheduler-independent paths passed.
  The full 432-row scope/class comparison retains drops and cutoff deadline failures.

Both builds ran on Windows with external verified GCC 14.2, Boost 1.85 and wired ns-3.46.1.
This is no-build-cache source reproduction, not a fresh dependency installation or actual
Mac M4/Ubuntu ARM64 validation. The existing 150-suite dependency results remain historical;
this milestone did not rerun or expand those upstream suites.

## Observed policy tradeoffs

Each matrix run offers 156 packets: 60 P0 and 24 P3, plus P1/P2. All mixed-load cases deliver
all packets without deadline misses. The following are min-max deadline-miss counts over
hash seeds 1/2/3, summed across same-zone, cross-zone and edge-service scopes. They are
finite-run counts, not confidence intervals or stochastic replications. P1/P2 and every
scope are preserved in scope_priority_comparison.csv.

| Scenario | Forwarding / service | P0 misses / 60 | P3 misses / 24 |
|---|---|---:|---:|
| Uplink contention | Dijkstra / FIFO | 38 | 14 |
| Uplink contention | Dijkstra / priority | 29 | 16 |
| Uplink contention | ECMP / FIFO | 18-38 | 12-14 |
| Uplink contention | ECMP / priority | 5-26 | 15-16 |
| Destination bottleneck | Dijkstra / FIFO | 19 | 7 |
| Destination bottleneck | Dijkstra / priority | 15 | 7 |
| Destination bottleneck | ECMP / FIFO | 0-19 | 6-7 |
| Destination bottleneck | ECMP / priority | 0-14 | 7-8 |

Priority improves P0 results in these comparisons at a cost to lower classes. ECMP can
spread flows across the two available spines, but the hash-dependent ranges show that
balanced use is not assured. Zero P0 misses for an individual destination-bottleneck seed
does not establish safety, admission guarantees, or success under uplink contention.
No default queue or workload was retuned to hide adverse outcomes.

The priority control transmits packet IDs 0,2,3,4,1: an in-service low-priority frame
finishes first; queued P0 frames retain admission order; P1 precedes the remaining P3.
The one-slot control rejects arriving P0 while keeping an already admitted P3. The flood
control leaves admitted P3 waiting at cutoff while P0 is served. These explicitly verify
nonpreemption, shared-buffer loss and possible lower-class starvation. C++ ECMP controls
verify stable consecutive packets, both eligible branches, seed sensitivity, expensive-path
rejection and endpoint transit exclusion. Independent audits reject forged headers and paths.

## Retained diagnostic limitations

The initial sandboxed tests were denied Windows temporary-file access; one rejection test
also reached schema validation before the intended native capability check. That fixture
was corrected and normal-temp-access tests passed; no test or compiler warning was disabled.
See checkpoints/evidence/checkpoint010-initial-validation.txt.

The first native batch, ns3-warehouse-20261031T215052Z-d0f2eb11, was interrupted after one
child wrote complete results but failed to exit promptly. The exact unchanged case retried
successfully in 0.297 seconds with all five artifacts identical. A debugger attach failed
with Windows error 31, so no cause was established. The full unchanged main retry and full
fresh build both passed. The interrupted batch remains marked INTERRUPTED, never PASS;
its raw data and diagnostics are retained. Intermittent Windows process-exit behavior remains
an unresolved handoff risk; successful retries do not prove it fixed.

## Remaining work

Native dynamic failures with explicit detection semantics, backup services, protected queues
and safety fan-out remain unsupported. NR requires compatible pinned ns-3/5G-LENA and actual
UPF user-plane tests. Mac/Ubuntu builds, workload calibration and final product readiness
remain open. Diagnose the exit-delay behavior or add bounded, logged subprocess supervision
before calling the unattended workflow fully reliable.

Checkpoint_011 update: [bounded native process supervision](ns3-run-control.md) is now
implemented and tested, preserving all 240 native measurements. The underlying earlier
Windows exit delay remains unexplained; this update does not claim a root-cause repair.
