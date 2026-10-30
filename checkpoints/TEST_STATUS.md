# Test status — checkpoint_007

| Check | Result | Evidence |
|---|---|---|
| Bootstrap / default core CTest | 19/19 PASS each | checkpoint007-bootstrap/default-tests.txt |
| Capacity/admission study | 288 PASS, seeds 4/5/6 | checkpoint007-capacity-status.json |
| Protected regression | 144 exact inputs and packet traces PASS | checkpoint007-protected-status.json |
| Warehouse regression | 132 exact inputs and packet traces PASS | checkpoint007-baseline-status.json |
| Operating table raw audit | 96 rows / all 288 traces PASS | checkpoint007-table-audit.json |
| Replay / charts | PASS all three, 60/40/55 PNGs | Batch outputs; flood-c5 scope figure visually inspected |
| Stationarity, queue conservation, loss denominators, paired offered traffic | PASS all runs | Runner artifact validation |
| ns-3 / NR runtime | BLOCKED / NOT RUN | Previous compiler failure unchanged |
| Linux / Mac execution | NOT RUN | Windows host only |

Evidence files are under checkpoints/evidence. Initial sandbox temp-file permission failures
are not simulator failures; authorized rerun passed without test/warning suppression.
The stale build LastTestsFailed.log may retain the initial sandbox failures: the saved final
LastTest.log records 19 Test Passed entries for each preset. Experimental deadline failures
remain in results and do not become performance guarantees merely because validation passes.
