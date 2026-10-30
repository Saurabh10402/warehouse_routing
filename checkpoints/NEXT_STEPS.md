# Next actions after checkpoint_003

1. Finish optional ns-3 build in .tools/ns3-build; inspect logs/ns3-build.log. Install to .tools/ns3-install and run scripts/verify.py --preset default --build-dir build/verified-ns3. Acceptance: compile/link/run actual adapter tests, or preserve exact blocker.
2. Final one-command experiment verification after strengthened raw-metric and offered-traffic checks. Preserve source/config/executable hashes and 114 runs with charts.
3. Update validation report and final checkpoint; commit source, verify manifest, create package via scripts/package_recovery.py --experiment results/SELECTED_BATCH. Verify zip hashes and Git bundle.
4. Future external work: integrate schema-v2 fabric with actual ns-3 packet/QoS forwarding and matched NR release. This does not block the independently tested core.
5. Research continuation: calibrate topology/traffic/energy from the finalized warehouse and run additional seeds; no algorithm hardcoding.
