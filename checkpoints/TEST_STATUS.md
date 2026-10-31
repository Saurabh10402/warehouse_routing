# Test status - checkpoint_010

| Check | Actual result | Evidence under checkpoints/evidence |
|---|---|---|
| Main and fresh native | 24/24 tests each PASS | checkpoint010-native-tests.txt; fresh-tests.txt; fresh-build.txt |
| Native controls and policies | All 48 cases, audits and every replay PASS | checkpoint010-native-status.json; native-comparison.csv |
| Fresh reproduction | 48 exact inputs, 240 identical measurements and tables | checkpoint010-fresh-comparison.json |
| Core | 20 tests, 132 exact comparisons/replay/55 charts PASS | checkpoint010-core-tests.txt; core-status.json |
| Class/scope | All 432 rows, including losses, retained | checkpoint010-scope-priority.csv |
| Initial failures | Temp permissions and corrected guard fixture retained | checkpoint010-initial-validation.txt |
| Delayed process exit | Interrupted; unchanged retry 0.297 s and full main/fresh PASS; cause unknown | checkpoint010-exit-retry.json; native-exit-diagnostic.txt; interrupted-native-log.txt |
| Native failures/protected queues/NR | NOT IMPLEMENTED/VALIDATED | Explicit capability checks |
| Mac M4 / Ubuntu ARM64 | NOT RUN | Windows-only fresh source evidence |

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

Existing upstream 150 UNIT QUICK results remain historical; not system/extensive tests.
