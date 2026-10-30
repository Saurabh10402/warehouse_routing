# checkpoint_006 — traffic contract and protected queues validated

Implemented experimental P0 reserved waiting space, per-direction byte-rate admission,
and byte-weighted service for admitted P1/P2/P3. Existing policies/default matrix preserved.

- Both core build presets: **19/19 CTest PASS**.
- Protected study: results/experiments-20261030T173430Z-db8fbaf2 — **144 runs PASS**, three
  seeds, six policies, raw/queue/fairness checks, replay, **40 charts**.
- Baseline regression: results/experiments-20261030T173542Z-c74e4faf — **132 runs PASS**;
  every input matches and packet trace is byte-identical to checkpoint_005's primary batch;
  replay and **55 charts PASS**.
- Tested code: 2d0dcede78ce551db9a4fdee21bd24bcc9919c42.
- Source manifest: manifests/checkpoint_006.json. Later changes are documentation only.

Read docs/traffic-requirements.md, protected-queues.md and protected-queue-results.md.
Cross-zone P0 misses at the destination bottleneck improve from 41.25% to 6.25% with ECMP
protected, but the emergency flood is worse: aggregate P0 misses rise from 1.5% to 25.5625%.
All policed/reservation losses remain in denominators. Do not adopt the 20% admission ceiling
as a production default; it can reject P0 with unused total link capacity.

Next: capacity/admission sensitivity and per-port critical traffic envelopes. Real warehouse
requirements, ns-3/NR runtime integration and cross-platform verification remain unfinished.
The external ns-3 Wi-Fi build failure is unchanged; no new attempt was made in this milestone.
Recovery ZIPs/bundles are under recovery_packages/. Prior archives and checkpoints are retained.
