# Resume after checkpoint_010

Read CURRENT_STATUS, PROJECT_STATE, sessions/session_007.md and native policy study/results.
Inspect Git status and verify checkpoint_010 before changes.

Next exact action: implement bounded, logged subprocess supervision for deterministic native
cases, with a controlled hung-child/partial-log regression test. Preserve timeout outputs
without declaring PASS. Then declare/implement native link/node failure detection, recovery,
queue/in-flight fate and backup destinations. No invented convergence or service replication.
Protected queues/safety/NR remain rejected until implemented and tested. Read traffic and
capacity findings before queue changes. Actual Mac M4/Ubuntu ARM64 builds, compatible NR/UPF
tests and workload calibration remain required; whole product readiness is still open.

Retain all 48 native cases/replays, identical policy arrivals, hash sensitivity and original
seven-case FIFO controls. Run the full 132-run core baseline regression. Keep models distinct
and all raw/provenance/history artifacts; no random movement in the primary workflow.

Completed:
Tested implementation: 3e97cd064d8e8fe102b928b7fb03a147cb9b1611. Later source edits are reports only.
Main: ns3-warehouse-20261031T215600Z-dd1a4b9a. Fresh: ns3-warehouse-20261031T215445Z-85e4fb40.
Each passed 24/24 CTest, 12 acceptance controls + 36 native policy runs, independent audits
and every replay. All 48 inputs, 240 measurement artifacts and aggregate tables match
byte-for-byte across builds. Original seven FIFO controls match every pre-existing field.
Core: experiments-20261031T215045Z-6af82532, 20 tests and 132 exact baseline input/packet comparisons
versus experiments-20261031T173020Z-a8e5dbed, replay and 55 charts PASS.
