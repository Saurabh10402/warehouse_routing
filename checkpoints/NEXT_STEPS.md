# Resume after checkpoint_004

The primary stationary prototype and full matrix are validated. Read CURRENT_STATUS,
PROJECT_STATE and docs/validation-report.md, inspect Git status, then run:

```sh
python scripts/checkpoint.py checkpoint_004 --verify
```

1. On the Mac/Ubuntu return, reproduce the core with `python scripts/run_all_experiments.py`
   using docs/reproducibility.md. Preserve the new host results separately.
2. Optional ns-3: resolve the external ns-3.46.1 Wi-Fi `-Werror=maybe-uninitialized` failure
   on an appropriate toolchain. See evidence/ns3-build-failure.txt and docs/ns3-integration.md.
   After a successful build/install, run `python scripts/verify.py --preset default
   --build-dir build/verified-ns3 --ns3-prefix PATH`. Syntax checks are not runtime evidence.
3. After adapter runtime tests pass, map schema-v2 topology, traffic, routing and priority
   queues into actual ns-3 packet forwarding; add a matched 5G-LENA module and verify NR
   traffic. Existing NR scaffolding does not establish this capability.
4. Research continuation needs the finalized warehouse layout and calibrated traffic,
   access delays and energy data. Configure these inputs and run additional seeds. Preserve
   negative results and report delivery alongside delivered-only latency.

Recovery ZIPs and Git bundles are in recovery_packages/. The ZIP manifest verifies each
included file. Toolchains/build caches are excluded; use setup_portable.py on Windows or
native prerequisites. Keep .recovery/original.zip separately.
