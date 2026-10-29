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
