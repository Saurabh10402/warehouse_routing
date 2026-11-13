# checkpoint_038 — Maintained authorization workflow

Added scripts/run_access_authorization.py: Linux/WSL pinned dependency checks,
build, immutable unique results, source/executable/linked-library hashes,
bounded process-group execution, original/replay and independent task audit.
Preserves changed scratch sources before replacement. New explicit static config
configs/access_authorization_control.json uses failureStudy:none. Validator allows
none while preserving desired single-spine-outage design configs. Runner rejects
ECMP, live outage request and >64 local nodes instead of silently downgrading.

Actual maintained-command run PASS: results/access-auth-20261106T185413Z-b9d0d304
(UTC identifier). Three tasks delivered, exact replay and behavioral audit PASS.
Native sources unchanged this milestone. Added capability tests4/4 PASS,
contract4/4 and path3/3 PASS; project verify21/21 PASS. Tiny follow-up preserves
previous scratch files when replacing; branch not exercised because sources match.
Documentation docs/run-access-authorization.md includes reproducible command,
results/status handling and explicit limitations. No fresh-install/Mac claim.
Source checkpoint_038. No process active. No Git upload.

Next implement live spine failure/restoration and deterministic route rebuild,
then authorize outage capability only after packet/path/task timing tests. ECMP,
full radio/S5 path audit, multiple seeds and500-node scaling remain pending.
Do not call static direct-link comparison dynamic failover. Preserve old ARP loss.
