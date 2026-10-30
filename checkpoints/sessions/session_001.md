# Session 001 — recovery and implementation

User requests a working, tested, reproducible WIPRO warehouse networking prototype.
Primary scope: fixed topology, variable/unequal zones, four priorities, local safety,
redundancy/failures, honest EALR comparisons, durable recovery. Do not claim NR results.
No architecture image was attached in this session; follow the detailed textual reference.

Audit: legacy algorithms exist but are not invoked by the ns-3 runner. NR setup installs
devices but no application traffic or custom forwarding. Warehouse plan is independently
parsed, with fixed equal zone sizes and unconditional minimum two spines/head nodes.
Safety constructor accepts NaN/infinity. Configuration enum and routing interface both use
the name ealr::RoutingAlgorithm (a latent header integration error). Build caches are from macOS.

Original archive and clean extraction retained under .recovery. Git initially has no commits.
The supplied Python runtime is available; native build prerequisites need portable installation.

Milestones: d632f2c preserved recovered source; d2f34e9 adds stationary model. Original 11 tests passed; expanded core/default suites reached 16/16. First full matrix completed 111 runs in results/experiments-20261029T210304Z-82f715a2. Charts later generated separately. Expanded 114-run matrix is being checked, adding energy/latency tradeoff and fairness checks. Optional ns-3.46.1 source downloaded and compiling in .tools/ns3-build; no NR module installed. See logs/ns3-*.log. Core code and ns-3 source are separate.

Final state: checkpoint_004 supersedes the in-progress notes above. Final batch
experiments-20261030T104837Z-6863eef6 (code b8214a5) passes all 114 runs, fairness/raw
checks, replay and 72 charts. Both core presets pass 16/16. External ns-3 build ended
in the Wi-Fi maybe-uninitialized diagnostic; evidence retained. Read NEXT_STEPS to resume.
