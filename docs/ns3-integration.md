# ns-3 and 5G-LENA integration

## Current verification boundary

The primary stationary experiment executable is `ealr-fabric`; its data is not ns-3 data.
The adapter described below retains the legacy robot schema and does not yet consume the
schema-v2 fabric or forward its application packets through custom routes.

On 30 October 2026 the three adapter implementation files passed `g++ -std=c++23 -fsyntax-only`
checks against official ns-3.46.1 headers. A full native MinGW GCC 14.2 Release build of ns-3
was attempted with core, network, mobility, netanim, flow-monitor, applications, internet and
point-to-point enabled. NetAnim pulls additional radio-module dependencies into that build.
Configuration succeeded, but compilation stopped in Wi-Fi `mgt-headers.cc` with
`-Werror=maybe-uninitialized`, inlining `std::optional<NonInheritance>` destruction/emplacement
from `wifi-mgt-header.h` around line 896. The diagnostic's underlying cause was not resolved;
it has not been declared harmless. Warnings were not disabled. Evidence is preserved in
`checkpoints/evidence/ns3-configure.txt` and `ns3-build-failure.txt`.

Therefore **ns-3 link/runtime tests and 5G-LENA NR remain unverified**. No NR module was
installed. This does not block the independent core build or its 16 tests and full experiments.
The downloaded external source is under `.tools/ns-3.46.1`; archive SHA256 (recorded after
download) is `fd3255df035a52008638fb5d81f77685260fd607be107eaf8e5116bcf1fd2c03`.
Two documentation-directory symlink aliases failed extraction on Windows; source compilation
proceeded and reached the separate compiler diagnostic above.

This release's source build selects C++23. The project chooses C++23 for an installed
ns-3 >=3.46.1 and C++20 for older supported headers; `EALR_NS3_CXX_STANDARD` can be set explicitly
to match the installed release. The core target remains C++17. Required imported targets are
checked, including flow-monitor. MPI is needed only if that ns-3 package requires it. The
FlowMonitor traffic test separately checks applications/internet/point-to-point availability.

Reproducible external build attempt (with compiler/CMake/Ninja on PATH):

```sh
cmake -S .tools/ns-3.46.1 -B .tools/ns3-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DNS3_ENABLED_MODULES=core,network,mobility,netanim,flow-monitor,applications,internet,point-to-point \
  -DNS3_TESTS=OFF -DNS3_EXAMPLES=OFF -DNS3_PYTHON_BINDINGS=OFF \
  -DCMAKE_INSTALL_PREFIX=/absolute/path/to/project/.tools/ns3-install
cmake --build .tools/ns3-build -j 4
# Only after successful build:
cmake --install .tools/ns3-build
python scripts/verify.py --preset default --build-dir build/verified-ns3 --ns3-prefix .tools/ns3-install
```

ns-3's own test suite was not requested in this prerequisite build; project adapter tests
were intended to run after installation and are currently blocked. In PowerShell, quote
the comma-separated NS3_ENABLED_MODULES argument. On a Linux/UTM machine, use the matching
release's [official installation guide](https://www.nsnam.org/docs/installation/singlehtml/index.html)
and [release source](https://www.nsnam.org/releases/ns-3-46/). Match NR using the
[5G-LENA installation/compatibility documentation](https://cttc-lena.gitlab.io/nr/html/).

An unresolved compiler diagnostic in the external release is the exact next optional
integration task. After resolving it on a supported toolchain, run the adapter tests before
adding schema-v2 forwarding/QoS and NR comparisons. Do not infer NR functionality from
successful core-only default configuration or from available node animation.

## Preserved legacy adapter

`Ns3Private5gScenario` is the bridge from `ExperimentConfiguration` to ns-3. It creates a node for every robot plus one private gNB, installs static/grid or random-waypoint mobility, labels nodes for NetAnim, and runs for the configured simulation duration.

When CMake finds the `ns3::nr` target, the runner also creates `NrHelper`, EPC, and ideal-beamforming helpers. It configures the channel factory, carrier frequency, bandwidth, numerology, and UE/gNB transmit powers from `private5g`, installs the NR devices, completes device configuration, and attaches each robot UE to the closest gNB. The helper objects remain alive for the duration of the ns-3 run.

The local ns-3 integration is deliberately target-checked: a distribution that lacks `ns3::nr` still builds and can generate mobility/NetAnim scenarios, but it cannot claim to simulate 5G-LENA NR. Install a 5G-LENA-enabled ns-3 package and expose it through `CMAKE_PREFIX_PATH` to enable the NR branch.
