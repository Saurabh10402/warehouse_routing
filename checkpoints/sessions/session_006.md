# Session 006 - initial stationary native ns-3 warehouse backend

Started from clean checkpoint_008 / a25b936; source manifest verified. No changes to the
independent core simulator or its primary matrix. Implemented a separate schema-v2 wired
FIFO/Dijkstra backend, real packet/queue/position traces, strict unsupported-input checks,
a deterministic seven-case runner, and independent raw artifact auditing. Initial linked
23-test and then 24-test builds passed; final numerical bounds and audit hardening are now
ready for the full native suite and the full core regression. No stochastic ns-3 claim.

Next: run scripts/run_ns3_warehouse.py and the complete 132-run warehouse baseline regression
against experiments-20261031T094745Z-723bc3c8, inspect all results, then checkpoint_009 and
separate verified recovery packages. Full native priority/ECMP/failures/NR and target-host
Mac/Ubuntu execution remain unimplemented or unvalidated.

Native batch ns3-warehouse-20261031T183840Z-7256a26e completed all seven deterministic cases, independent packet/queue/position/deadline audit and replay PASS. 24/24 CTest PASS. Original long CLI timeout retained; unchanged targeted retry passed in 3.66 seconds, followed by full successful workflow. Core batch experiments-20261031T173020Z-a8e5dbed is still running. Isolated fresh source .recovery/ns3 warehouse clean 03477011 is compiling; do not claim fresh-source PASS until its recorded receipt exists.

## Final checkpoint_009 outcome

Tested implementation 49d4f45; no subsequent code changes. Native batch
ns3-warehouse-20261031T183840Z-7256a26e: all 24 tests, seven deterministic cases, raw audit and
replay PASS. Fresh source .recovery/ns3 warehouse clean 03477011: fresh compile, 24 tests and
seven cases/replay PASS; archived raw batch ns3-warehouse-20261031T185300Z-b07383e3. Provenance
correctly says source archive; all source hashes matched and all 35 native artifacts match
main results byte-for-byte. External Windows compiler/Boost/ns-3 were used; no Mac claim.
Full core batch experiments-20261031T173020Z-a8e5dbed: 20 tests, 132 exact baseline comparisons,
replay and 55 charts PASS. Preserved the initial 3913.74 s CLI timeout, unchanged 3.66 s
successful retry and subsequent full/fresh passes; no timeout/test/warning changed.

Same-zone/cross-zone/edge actual delays: 4.416/8.832/6.624 ms. Overload retains late deliveries
and drops; cutoff retains censored packets; no-route includes pre-IP offered traffic. Native
priority/ECMP/events/backup/energy/radio/jitter/fragmentation remain explicitly unsupported.
Next: native strict priority and per-flow ECMP with controlled comparisons, then failures,
protected queues and NR. Finish checkpoint manifest, source commit and separate recovery
packages for main native, fresh native and core regression; see receipt-checkpoint_009.json.
