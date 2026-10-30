# Resume after checkpoint_005

1. Read CURRENT_STATUS, PROJECT_STATE, sessions/session_002.md and docs/warehouse-results.md.
   Inspect Git status and run `python scripts/checkpoint.py checkpoint_005 --verify`.
2. Next controlled extension: define reserved emergency buffer space, bounded high-priority
   admission, and lower-class service allocation. Add tests for a full buffer, overload,
   competing traffic and starvation; compare against the unchanged 132-run baseline.
   Do not claim capacity or deadlines can be guaranteed simply by queue priority.
3. Run `python scripts/verify.py`, then `python scripts/run_all_experiments.py` for full
   three-seed primary evaluation. `--suite legacy-ealr` selects the preserved 114-run legacy
   study; `--quick` is only a smoke check. Keep each new result directory and provenance.
4. Request actual warehouse traffic/layout/access inputs when preparing calibrated scenarios.
   Current baseline is wired; real private-5G peer forwarding requires explicit UPF paths.
5. Optional ns-3: preserve the external Wi-Fi diagnostic in evidence/ns3-build-failure.txt.
   Resolve it on an appropriate toolchain, install only after success, and run
   `python scripts/verify.py --preset default --build-dir build/verified-ns3 --ns3-prefix PATH`.
   Then implement/test schema-v2 forwarding/QoS and matched NR. Syntax is not runtime proof.
6. Reproduce on the Mac/Ubuntu host using docs/reproducibility.md. Those platforms remain
   unexecuted here. Verify package hashes and manifests before rebuilding.

EALR is legacy scope. Do not reintroduce energy-aware relay optimization into powered
infrastructure experiments without a relevant architecture and explicit user direction.
