# Resume after checkpoint_008

1. Read CURRENT_STATUS, PROJECT_STATE, sessions/session_005.md and docs/runtime-validation.md.
   Inspect Git status; verify `python scripts/checkpoint.py checkpoint_008 --verify`.
2. Implement the first supported wired schema-v2 ns-3 slice from docs/ns3-warehouse-contract.md.
   Reuse validated configuration; use stationary nodes and actual packet observations for
   same-zone, cross-zone and edge-service paths. Reject unsupported features explicitly.
   Keep core outputs/model labels and all known-good comparisons unchanged.
3. Extend finite queue accounting/FIFO/priority and per-flow ECMP with packet-level tests;
   specify payload versus on-wire bytes, loss/deadline denominators and routing semantics.
4. Add failures/recovery/backup services, then protected queues and local safety fan-out.
   Do not substitute immediate core topology knowledge for measured protocol convergence.
5. Select and pin a compatible ns-3/5G-LENA pair separately. Validate EPC/UPF and receiver
   traffic before radio comparisons. Old Wi-Fi/NetAnim compiler diagnostic remains recorded.
6. Execute clean builds on MacBook Air M4 and Ubuntu 24.04 ARM64/UTM when those hosts exist.
   docs/first-run.md is the prepared workflow, not evidence those commands ran there.
7. Calibrate actual warehouse traffic, access technology and deadline/loss boundaries before
   deployment claims. Preserve the capacity study's adverse burst/flood/lower-class results.

Completed this milestone:
- `python scripts/setup_ns3.py --offline --jobs 4`: wired build/install, 150 upstream suites PASS.
- `python scripts/run_ns3_smoke.py`: strict linked build, 22 project tests, UDP runtime/replay PASS.
- `python scripts/first_run.py` in isolated source path: 20 tests, 180/180 delivery PASS.
- `python scripts/run_all_experiments.py --suite warehouse --preset bootstrap --baseline results/experiments-20261030T180019Z-2441a69a`: full 132-run exact regression PASS.

Results and source have separate recovery packages; inspect receipt-checkpoint_008.json.
No random movement, assumed ten equal zones or radio claims belong in the primary core runner.
