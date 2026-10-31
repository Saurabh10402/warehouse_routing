# checkpoint_010 - native static priority / transport ECMP validated

Read docs/ns3-policy-study.md and docs/ns3-policy-results.md. Source manifest: manifests/checkpoint_010.json.

Tested implementation: 3e97cd064d8e8fe102b928b7fb03a147cb9b1611. Later source edits are reports only.
Main: ns3-warehouse-20261031T215600Z-dd1a4b9a. Fresh: ns3-warehouse-20261031T215445Z-85e4fb40.
Each passed 24/24 CTest, 12 acceptance controls + 36 native policy runs, independent audits
and every replay. All 48 inputs, 240 measurement artifacts and aggregate tables match
byte-for-byte across builds. Original seven FIFO controls match every pre-existing field.
Core: experiments-20261031T215045Z-6af82532, 20 tests and 132 exact baseline input/packet comparisons
versus experiments-20261031T173020Z-a8e5dbed, replay and 55 charts PASS.

Native seeds 1/2/3 vary ECMP hashes only: not stochastic traffic/radio replication. Preserve
adverse results: uplink ECMP/priority still misses 5-26/60 P0 and 15-16/24 P3 deadlines.
A full shared queue can reject P0; lower classes can starve. No deadline guarantee.
Initial sandbox temp-permission errors and a corrected guard fixture are retained. Initial
native batch ns3-warehouse-20261031T215052Z-d0f2eb11 delayed exit after a child wrote results;
unchanged exact-case retry passed in 0.297 s with identical outputs. Debugger attach failed
with Windows error 31. Full unchanged retry and fresh workflow passed; cause unestablished.
Interrupted batch is marked INTERRUPTED, never PASS. No test or warning was disabled.
Fresh evidence uses Windows and external verified dependencies, not Mac/Ubuntu execution.

Next exact action: implement bounded, logged subprocess supervision for deterministic native
cases, with a controlled hung-child/partial-log regression test. Preserve timeout outputs
without declaring PASS. Then declare/implement native link/node failure detection, recovery,
queue/in-flight fate and backup destinations. No invented convergence or service replication.
Protected queues/safety/NR remain rejected until implemented and tested. Read traffic and
capacity findings before queue changes. Actual Mac M4/Ubuntu ARM64 builds, compatible NR/UPF
tests and workload calibration remain required; whole product readiness is still open.

Source and separate main/fresh/core/diagnostic recovery packages are recorded in
recovery_packages/receipt-checkpoint_010.json after packaging. Preserve previous archives/results.
