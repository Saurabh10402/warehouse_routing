# checkpoint_005 — powered warehouse communication baseline

Approved direction: powered infrastructure, same-zone/cross-zone peers and local edge
services, priority scheduling and resilience. EALR is preserved as legacy work.

- Bootstrap and default core-only CTest: **18/18 PASS each**.
- Primary batch: results/experiments-20261030T140941Z-dd164497 — **132 runs PASS**,
  11 scenarios, three seeds, four policies; raw/fairness checks, replay and **55 charts**.
- Legacy smoke: results/experiments-20261030T141204Z-9e04fe27 — **45 runs PASS**, one
  seed per case, replay and 75 charts. The earlier full legacy batches remain preserved.
- Tested code commit: 1da7f3fb2eb48e9941228e17ae3b607614c256a9.
- Source manifest: manifests/checkpoint_005.json. Post-run source edits are documentation only.

Read docs/warehouse-study.md and docs/warehouse-results.md. The primary study uses wired
abstract links. Two spines do not protect a failed zone leaf or remove a destination-link
bottleneck. With ECMP+priority, cross-zone P0 misses average 41.25% in the destination
bottleneck case. No admission control or reserved emergency buffer is implemented yet.

Optional ns-3.46.1 still fails in its Wi-Fi dependency with GCC maybe-uninitialized;
adapter runtime/NR remain unverified. No new ns-3 attempt was made in this milestone.
Original archive, prior checkpoints and results are retained. Recovery ZIPs and Git bundles
are in recovery_packages/; tools and build caches are excluded. See NEXT_STEPS.md.
