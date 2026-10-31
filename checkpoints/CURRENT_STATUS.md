# checkpoint_008 — real wired ns-3 runtime and first-run workflow validated

The independent stationary warehouse core remains the primary model. Actual ns-3 wired
prerequisites now build, install and exchange measured packets. Read docs/runtime-validation.md,
docs/first-run.md and docs/ns3-integration.md. This is not full schema-v2/NR integration.

- Tested code: 47088f1 (after a2bf99f implementation and 39ff2af installer fix).
- Source manifest: manifests/checkpoint_008.json; previous manifests/results retained.
- ns-3.46.1 Release / GCC 14.2: 150/150 upstream UNIT QUICK suites PASS, warnings-as-errors ON.
- Linked project build: 22/22 CTest PASS, including actual one/two-hop UDP delay and loss.
- ns3-smoke-20261031T102542Z-fd5d9232: 3/3 packets, 4.416 ms, stationary, exact replay.
- Final extracted code: 20/20 tests and 180/180 first run, no Git/cache dependency.
  Batch first-run-20261031T101929Z-5481bba4; copied unchanged from isolated extraction.
- Warehouse regression experiments-20261031T094745Z-723bc3c8: 132 identical inputs/traces
  versus experiments-20261030T180019Z-2441a69a; replay and 55 charts PASS.
- Required dependency/prefix isolation and existing-output preservation checks PASS.

Upstream suites must run separately. Fixed the invocation and summary parser; retained every
failed attempt. The slow threaded suite passed naturally (275 s first run, 179 s final run).
Final installer completed successfully; no test or warning was suppressed. The previous
Wi-Fi/NetAnim diagnostic is still unresolved; separating optional modules is not its repair.

Next exact action: implement the first wired schema-v2 ns-3 slice described in
docs/ns3-warehouse-contract.md, with real same-zone/cross-zone/edge-service packets and
observed paths. Then add explicitly tested QoS/failures and compatible NR user-plane paths.
Mac M4 and Ubuntu 24.04 ARM64 commands are prepared but not executed. Calibration remains
open; neither abstract results nor this fixture guarantee deadlines or radio performance.

Recovery packages preserve the real ns-3 fixture, final extracted first run and 132-run
regression separately with source/checkpoints/Git history. See recovery_packages/
receipt-checkpoint_008.json after packaging. Keep previous archives and original.zip.
