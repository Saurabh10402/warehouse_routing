# ns-3 and 5G-LENA integration

## Current verification boundary

The primary stationary experiment executable is `ealr-fabric`; its outputs remain independent
abstract-link measurements. `ealr-ns3-smoke` is a separate real wired IPv4/UDP prerequisite
fixture. The new `ealr-ns3-fabric` consumes a strict stationary wired schema-v2 subset with
FIFO/strict-priority queues and size-aware Dijkstra/ECMP forwarding; see [its contract and workflow](ns3-wired-warehouse.md).
Native node/link availability and ordered service failover use immediate centralized knowledge
and receive-time outage gates; see [the declaration](ns3-resilience-study.md). Protected queues
and radio remain outside that subset.
The preserved legacy robot/NetAnim adapter has a different configuration and scope.

On 31 October 2026, the wired ns-3.46.1 Release build and all 150 upstream UNIT QUICK
suites passed on Windows 11 / MinGW GCC 14.2. The installed package passed all 22 project
tests, including actual routed delay and receive-error loss checks. The two-hop fixture
delivered 3/3 packets at 4.416 ms mean delay with byte-identical replay. See
[the runtime validation report](runtime-validation.md) and checkpoint_008 for raw evidence.
Those checkpoint_008 results are small prerequisite measurements. The new schema-v2 slice
has separate validation and output batches; see [the policy comparison](ns3-policy-study.md); NR remains pending.

## Reproducible wired prerequisites

Follow [the first-run guide](first-run.md) for host prerequisites, then run:

```sh
python scripts/setup_ns3.py --jobs 4
python scripts/run_ns3_smoke.py
```

Python 3.12+ is required for safe archive extraction. The setup script pins official ns-3.46.1
archive SHA256 `fd3255df035a52008638fb5d81f77685260fd607be107eaf8e5116bcf1fd2c03`, verifies
existing source against the archive, and installs locally under `.tools/ns3-wired-install`.
It enables core, network, mobility, flow-monitor, applications, internet and point-to-point,
plus their required dependencies. NetAnim and radio modules are not required by this fixture.
Only two documentation figure symlink aliases are omitted during extraction.

The Release build retains warnings and warnings-as-errors. Upstream UNIT QUICK suites run
before installation; a nonzero exit or zero passing suites prevents a success receipt. This
is not the system/extensive test suite. Release defaults leave assertions/logging disabled.
The test executable lives under the source tree because this release discovers reference
data by walking up from its executable. Previous build attempts and diagnostics are retained.

The project then requires the explicit installed prefix, compiles and runs its tests, and
executes a two-hop UDP fixture plus byte-identical replay. It checks three 100-byte UDP
packets, 384 received IP bytes, fixed positions and a 4.416 ms mean delay across two 5 Mbps,
2 ms links. Delay includes UDP, IPv4 and PPP headers. Tests additionally cover one-hop delay,
receive-error loss after FlowMonitor's timeout, and agreement with the application receiver.
These analytical checks validate a small fixture, not warehouse performance or deadlines.

Explicit `scripts/verify.py --ns3-prefix PREFIX` requires the wired modules and fails if they
are missing. It cannot fall back to a different global package or a core-only result. The
ordinary default preset retains an explicit core-only fallback warning for legacy workflows.
The core remains C++17; the optional ns-3 target uses C++23 for this pinned release.

## Preserved external failure and legacy adapter

The earlier full ns-3/NetAnim Release build failed in Wi-Fi `mgt-headers.cc` with GCC 14.2
`-Werror=maybe-uninitialized`, involving `std::optional<NonInheritance>` in
`wifi-mgt-header.h`. NetAnim pulls radio dependencies into that build. The underlying diagnostic
is unresolved and has not been declared harmless or disabled. Evidence remains in
`checkpoints/evidence/ns3-configure.txt` and `ns3-build-failure.txt`. Separating optional NetAnim
allows a wired build; it does not repair or validate the excluded Wi-Fi/NetAnim build.

`Ns3Private5gScenario` retains the legacy `ExperimentConfiguration`, robot nodes, a gNB node,
static/grid or random-waypoint mobility, and NetAnim output. It is built only when NetAnim is
available. Random movement is not part of the primary stationary runner.

The optional `nr` branch constructs NR/EPC/beamforming helpers and devices when compatible
NR and NetAnim targets are available. This scaffolding is not a verified application user
plane. NR is not installed or runtime-tested here. Neither attachment nor animation establishes
5G warehouse packet delivery. Keep these APIs/examples distinct from schema-v2.

## Remaining integration

Use [the schema-v2 implementation contract](ns3-warehouse-contract.md) for the next staged
backend: explicit stationary topology, application packet observations, queue accounting,
forwarding policies, then failures/services and protected QoS. Unsupported features must fail
clearly until implemented. Preserve core regressions and label every new backend result.

NR requires a separately pinned compatible pair using the
[5G-LENA compatibility documentation](https://cttc-lena.gitlab.io/nr/html/), explicit EPC/UPF
paths and real receiver traffic tests. Do not attach an arbitrary latest NR release to the
wired pin. Refer to the [ns-3 release installation guide](https://www.nsnam.org/docs/release/3.46/installation/singlehtml/index.html)
for platform prerequisites. Mac M4 and Ubuntu ARM64 commands are prepared but have not been
executed on this Windows host; target-platform validation remains required.
