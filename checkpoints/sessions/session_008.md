# Session 008 - bounded native process supervision

Started from clean checkpoint_010 / 1d684a9; source manifest verified. New helper supervises
one native process with direct file-backed stdout/stderr, positive finite timeout, bounded
kill/reap, atomic receipt/status writes, and preserved partial results. Batch provenance
moves before experiment execution; RUNNING/TIMEOUT/FAILED/INTERRUPTED remain distinct from
explicit post-audit PASS. No automatic retry and no native C++/traffic/queue changes.

Four real-process tests pass: hung-child cleanup/log/artifact retention, nonzero exit and
launch failure, artifact-check failure after successful exit, interruption/invalid timeout.
Next: run full native verification/48-case workflow and core bootstrap verification; compare
all native measurements to checkpoint_010. The 132-run core study from checkpoint_010 remains
historical because no core implementation changed. Record exact evidence, manifest, commit
and recovery package. Native dynamic failures/services/NR and actual Mac/Ubuntu remain open.

## Final checkpoint_011 validation
Tested supervision code: 1f12fdddafa3fe771d44f5acc20f1efa4b78c633; native model code remains 3e97cd0.
Batch: ns3-warehouse-20261101T152405Z-ac57d7cc. Native 25/25 CTest and bootstrap 21/21 CTest PASS.
All 48 native cases, independent raw audits and every replay PASS (96 supervised processes).
All processes exited zero and were reaped; maximum observed duration 0.781 seconds.
All 48 inputs, 240 measurement artifacts and both comparison tables match checkpoint_010
byte-for-byte. Executable and ns-3 library hashes also match. 83 simulator/configuration/root
CMake files are unchanged. This milestone changes supervision, tests and documentation only.

The supervisor defaults to 60 seconds per native case/replay after launch, with a separate
five-second kill/reap allowance. It logs cleanup failure and unreaped PID if termination fails.
Native executables do not spawn descendants; this is not process-tree supervision. Setup,
compilation and CTest retain their existing behavior. No automatic retry or promotion of
partial files to PASS. A hard kill can leave RUNNING, which is not successful validation.
The earlier Windows exit stall's underlying cause is unestablished; bounded supervision
mitigates indefinite native-case waiting but does not prove the cause fixed.
The 132-run core regression, fresh source rebuild and three native hash-seed study remain
checkpoint_010 evidence, not newly rerun core/fresh studies. Current native cases still use
three deterministic ECMP hash seeds, not stochastic radio/traffic replication.
Mac M4/Ubuntu ARM64 execution, NR and calibration remain unvalidated.

Next exact action: declare native link/node failure/recovery semantics, detection timing,
in-flight and queued packet fate, table updates and preferred/backup service reachability.
Then implement controlled native cases before widening accepted schema features. Preserve
same-zone paths, endpoint transit exclusion, shared-buffer losses and complete offered-traffic
denominators. Protected queues and local safety fan-out remain separate future work; review
prior protected/capacity findings before changing parameters. Compatible NR/UPF receiver tests
and actual target-host validation are required before describing the whole product as finished.

Finish final manifest, source/evidence commit and verified recovery archive; receipt-checkpoint_011.json.
