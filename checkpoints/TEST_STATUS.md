# Test status - checkpoint_011

| Check | Actual result | Evidence under checkpoints/evidence |
|---|---|---|
| Native project | 25/25 CTest PASS | checkpoint011-native-tests.txt |
| Core-only project | 21/21 CTest PASS | checkpoint011-core-tests.txt |
| Process failure controls | Real hung child reaped; logs/artifact retained; failures never PASS | process_control_tests in both CTest logs |
| Native suite | 48 cases, all audits/every replay; 96 supervised exits PASS | checkpoint011-native-status.json |
| Measurement regression | All 240 artifacts, 48 inputs and tables identical | checkpoint011-native-comparison.json |
| Unchanged model | 83 source/config/root CMake files identical | checkpoint011-unchanged-simulator.json |
| Core 132-run / fresh build | Historical checkpoint_010; not rerun here | checkpoint010 evidence retained |
| Native failures/NR, Mac/Ubuntu | NOT IMPLEMENTED/VALIDATED | Pending work |

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
