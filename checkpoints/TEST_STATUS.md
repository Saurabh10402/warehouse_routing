# Test status - checkpoint_009

| Check | Actual result | Evidence under checkpoints/evidence |
|---|---|---|
| Linked native project | 24/24 CTest PASS | checkpoint009-ns3-tests.txt |
| Fresh project source/build | 24/24 CTest PASS | checkpoint009-fresh-ns3-tests.txt; fresh-ns3-build.txt |
| Native schema-v2 acceptance | All seven deterministic cases, audit and replay PASS | checkpoint009-ns3-status.json; native-comparison.csv |
| Across source roots | All 35 measurement files byte-identical | checkpoint009-fresh-comparison.json; fresh-source.json |
| Core bootstrap | 20/20 CTest PASS | checkpoint009-bootstrap-tests.txt |
| Full core regression | 132 exact inputs/traces, replay and 55 charts PASS | checkpoint009-core-status.json |
| CLI/unsupported/corrupt evidence checks | PASS in linked/fresh CTest | Native CLI tests; no required guards disabled |
| Initial interrupted CLI invocation | Timeout retained; unchanged retry PASS in 3.66 s | checkpoint009-ns3-timeout.txt; cli-retry.txt |
| Native strict priority/ECMP/failures/NR | NOT IMPLEMENTED/VALIDATED | Capability rejection and next-step contract |
| Mac / Ubuntu execution | NOT RUN | Windows-only fresh-build evidence |

Tested implementation is 49d4f45. Later edits are documentation/checkpoint evidence only.
Dependency source/runtime unchanged from checkpoint_008: 150 upstream UNIT QUICK suites
previously passed; system/extensive suites were not run. No warning or required test was
suppressed. Native and core model results remain separate. Validation passes preserve adverse
queue/deadline outcomes; they do not imply that the fixtures or a real deployment meet all targets.
