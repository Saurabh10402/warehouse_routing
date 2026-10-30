# checkpoint_004 — validated core and recovery handoff

Core bootstrap and default (core-only) CTest: **16/16 PASS each**.
Final batch: results/experiments-20261030T104837Z-6863eef6 — **114 runs PASS**,
identical offered traffic, raw-packet metric checks, deterministic replay, **72 charts**.
Tested implementation commit: b8214a51edf075295aa0b00bf0b20cda2354d9df.
Source manifest: manifests/checkpoint_004.json. Later edits only document final evidence.

The primary model is a stationary abstract-link simulator; it is not ns-3/NR.
Optional ns-3.46.1 Release build **FAILED** in its Wi-Fi dependency with GCC
`-Werror=maybe-uninitialized` (wifi-mgt-header.h / optional<NonInheritance>).
Three adapter files pass syntax checks. Full adapter link/runtime is BLOCKED; NR is
not installed or verified. Schema-v2 ns-3 forwarding remains future integration work.

Start with NEXT_STEPS.md and docs/validation-report.md. Recovery ZIPs and bundles are
in recovery_packages/; each ZIP carries a verified PACKAGE_MANIFEST.json. Toolchains
and build caches are excluded. Original archive remains .recovery/original.zip,
SHA256 6e0e84ba7f075737da9299ec3520bb875404556a9199a9b92468b938e6da1838.
