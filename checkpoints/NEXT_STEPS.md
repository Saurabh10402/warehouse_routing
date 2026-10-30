# Resume after checkpoint_006

1. Read CURRENT_STATUS, PROJECT_STATE, sessions/session_003.md and docs/protected-queue-results.md.
   Inspect Git status; run `python scripts/checkpoint.py checkpoint_006 --verify`.
2. Define the next sweep before inspecting results: endpoint/uplink capacity, P0 rate/burst
   budgets and reservation sizes. Keep current parameters as reference and use additional or
   held-out seeds. Derive per-port critical-traffic envelopes from declared flows rather than
   blindly applying 20% everywhere. Report admission losses as deadline misses.
3. If considering a congestion-conditional policer or per-link policy overrides, specify the
   new semantics separately, test token/queue accounting and admitted lower-class service,
   and compare against the current always-on ceiling. Do not silently retune the old batch.
4. Existing commands:
   `python scripts/run_all_experiments.py --suite protected-qos` (144 runs);
   `python scripts/run_all_experiments.py --suite warehouse --baseline results/experiments-20261030T140941Z-dd164497`
   (132 exact baseline comparisons on this host). `--quick` remains a smoke check only.
5. Obtain actual warehouse traffic, layout, loss/availability requirements and the application
   versus network deadline boundary before calibration. EALR remains legacy scope.
6. Resolve the documented external ns-3 dependency build failure on an appropriate toolchain,
   then run adapter runtime tests and implement real schema-v2/QoS/NR forwarding with explicit
   user-plane paths. Syntax success is not runtime proof. Reproduce core on Mac/Linux too.
7. Final completion needs validated simulator fidelity, calibrated evaluation and a research
   report that distinguishes ordinary QoS mechanisms from any substantiated novel contribution.

Recovery packages for this milestone preserve the protected study and baseline regression
separately; each contains source/checkpoints and a complete Git bundle. Keep both and the
original archive. Toolchains/build caches are excluded.
