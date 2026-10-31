# checkpoint_009 - initial native wired schema-v2 warehouse validated

The project now has a separate `ealr-ns3-fabric` backend for stationary wired schema-v2
FIFO/Dijkstra configurations. Read docs/ns3-wired-warehouse.md and docs/ns3-warehouse-results.md.
The independent core and its primary study are unchanged; native priority/ECMP/NR are pending.

- Tested code: 49d4f4538873ec2619b52b726e70d1c7693f8ccd. Source manifest: manifests/checkpoint_009.json.
- Main and independently rebuilt source: 24/24 CTest PASS each; all seven deterministic native
  cases, raw artifact audit and replay PASS; all 35 measurement artifacts match byte-for-byte.
- Native batch: ns3-warehouse-20261031T183840Z-7256a26e.
- Fresh source batch: ns3-warehouse-20261031T185300Z-b07383e3, copied unchanged for recovery.
- Same-zone/cross-zone/edge-service packets followed actual observed routes at 4.416/8.832/6.624 ms.
- Native queue loss, FIFO conservation, duplex independence, no-route and cutoff accounting,
  mixed-size route selection, endpoint transit exclusion and unsupported-input guards PASS.
- Core: 20/20 CTest and full 132 exact baseline comparisons, replay and 55 charts PASS.
  Batch experiments-20261031T173020Z-a8e5dbed, reference experiments-20261031T094745Z-723bc3c8.

One CLI test timed out during an interrupted invocation; retained log, unchanged targeted
retry (3.66 s), full retry and fresh build PASS. No timeout/test/warning was relaxed.
The native cases are deterministic acceptance fixtures, NOT a multiple-seed radio study.
Queue drops and late arrivals are retained. Cutoff is censored, not proven permanent loss.

Next exact action: implement native strict priority and per-flow ECMP with packet-level
controls and explicit transport/queue semantics. Preserve these FIFO cases. Then add dynamic
failures, backup services, protected queues and a compatible NR user plane. Mac M4/Ubuntu
ARM64 commands are prepared but not executed; calibration and full product readiness remain open.

Recovery archives separately preserve the main native batch, fresh-source batch and complete
core regression, each with source/checkpoints/Git history. See recovery_packages/
receipt-checkpoint_009.json after packaging. Original archives, prior manifests and results remain.
