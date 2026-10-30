# checkpoint_007 — capacity/admission sweep validated

Completed the declared 288-run study: three workloads, four attachment capacities,
eight ECMP queue settings, new seeds 4/5/6. Read docs/capacity-study.md and
docs/capacity-results.md. No parameters were retuned after execution.

- Tested source: 8fe3f315881f197b3af4a7d381913b04602e117b. Subsequent source edits are docs only.
- Manifest: manifests/checkpoint_007.json.
- Both core build presets: 19/19 CTest PASS.
- Capacity batch: experiments-20261030T175442Z-cb0b1ccc — 288 PASS, replay, 60 charts.
- Protected regression: experiments-20261030T175551Z-a5db7fd7 — 144 exact inputs/traces PASS, replay, 40 charts.
- Warehouse regression: experiments-20261030T180019Z-2441a69a — 132 exact inputs/traces PASS, replay, 55 charts.
- Independent raw-packet recomputation: all 96 operating-limit rows PASS.

At 1 Mbps mixed load, unpoliced reservation reduces P0 misses from strict's 15.278% to
1.944%. At 2 Mbps it has zero P0 misses in these seeds but 23.556% P3 misses. No tested
setting eliminates P0 misses in burst/flood workloads. At 5 Mbps flood, unpoliced P0
misses are 1.285%, versus 46.688% with rate20; unpoliced P3 delivery is only 79.5%.
Neither is promoted to a production default. All loss causes remain in denominators.

Next: repair the recorded ns-3 build and validate a minimal actual packet runtime before
schema-v2/QoS/NR integration. Calibration and cross-platform execution remain unfinished.
No new ns-3 build attempt occurred during this milestone; previous external failure remains.
Recovery packages preserve each completed batch separately under recovery_packages/;
receipt-checkpoint_007.json records exact archive hashes. Keep previous archives/manifests.
