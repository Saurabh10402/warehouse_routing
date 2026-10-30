# Resume after checkpoint_007

1. Read CURRENT_STATUS, PROJECT_STATE, sessions/session_004.md, docs/capacity-results.md
   and docs/ns3-integration.md. Inspect Git status and verify checkpoint_007.
2. Next executable milestone: inspect preserved ns-3 compiler diagnostics, determine a
   supported repair without suppressing warnings/tests, and validate a minimal packet
   example. A minimal runtime alone is not schema-v2 or NR integration.
3. Implement actual topology/traffic/QoS forwarding in the ns-3 adapter with packet-level
   tests before connecting NR and explicit user-plane paths. Preserve core comparisons.
4. Obtain real layout, access technology, flow periods/bursts, acceptable loss and network
   versus application deadline boundaries before claiming deployment suitability.
5. Further queue study must declare changes first: downstream/per-port arrival envelopes,
   port-specific budgets, reservation sizes and uplink capacities remain open. No tested
   setting eliminated burst/flood misses; do not silently select unpoliced or rate20 defaults.
6. Reproduce the core on Linux/Mac. Record actual tests, raw data and source provenance.

Completed commands (full matrices, not quick mode):
- python scripts/run_all_experiments.py --suite capacity-admission
- python scripts/run_all_experiments.py --suite protected-qos --preset default --baseline results/experiments-20261030T173430Z-db8fbaf2
- python scripts/run_all_experiments.py --suite warehouse --preset default --baseline results/experiments-20261030T173542Z-c74e4faf

Three separate recovery packages preserve the new 288-run batch and the 144/132 exact
regression batches, each with source/checkpoints/Git history. See the checkpoint_007 receipt
in recovery_packages. Original archives, old manifests and previous results are retained.
