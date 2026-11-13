# checkpoint_028 — Reproducible radio test correction and combined validation

Pinned ns-3.46 test-only patch is maintained in scripts/patches, with guarded
installer scripts/apply_radio_test_patch.py. Initial application and exact
idempotent reapplication PASS. Four test files only; model code unchanged.
Dependency rebuild PASS. Combined runner targeted QUICK suites PASS: wifi-mlo,
wifi-txop, wifi-emlsr-basic-exchanges, wifi-gcr.
Full upstream QUICK rerun STARTED; outcome PENDING. Active tool session 98166.
Linux batch: /home/warehousebuild/warehouse-radio/patched-validation-20261104T073031Z
Check full-status.txt, exit.txt and upstream-quick.log before restarting anything.
Original Wi-Fi test library preserved as original-wifi-test.so in this batch;
original failed upstream reports retained separately. Prior full run took ~95 min.

Project scripts/verify.py PASS 21/21 with normal Windows permissions. Initial
restricted run failed temporary-directory access; log retained in
.recovery/verification-patch-restricted.log. No core model changes.
Source manifest checkpoint_028; prior checkpoint_027 retained. New maintained
files: installer and patch; wireless integration documentation updated.

Next: inspect full validation outcome, resolve any actual failures; then promote
mixed access controls into configurable warehouse code with radio/path audits.
Full warehouse radio integration remains unimplemented. No Git push performed.

## Full patched dependency validation completed
Verified full-status.txt and exit.txt: COMPLETED exit=0.
729/731 passed, 2 intentionally skipped TAKES_FOREVER examples under QUICK,
0 failed, 0 crashed. Reports: Linux warehouse-radio/patched-validation-20261104T073031Z/upstream-quick.log, .txt and .xml.
No validation process remains active. Historical unmodified failed run retained.
Next: maintained configurable stationary NR/Wi-Fi warehouse runner, starting
with small cross-access/peer/service controls before eight zones/~500 devices.
This validates dependencies with local test-only fixes, not the full warehouse.
