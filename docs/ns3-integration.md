# ns-3 and 5G-LENA integration

`Ns3Private5gScenario` is the bridge from `ExperimentConfiguration` to ns-3. It creates a node for every robot plus one private gNB, installs static/grid or random-waypoint mobility, labels nodes for NetAnim, and runs for the configured simulation duration.

When CMake finds the `ns3::nr` target, the runner also creates `NrHelper`, EPC, and ideal-beamforming helpers. It configures the channel factory, carrier frequency, bandwidth, numerology, and UE/gNB transmit powers from `private5g`, installs the NR devices, completes device configuration, and attaches each robot UE to the closest gNB. The helper objects remain alive for the duration of the ns-3 run.

The local ns-3 integration is deliberately target-checked: a distribution that lacks `ns3::nr` still builds and can generate mobility/NetAnim scenarios, but it cannot claim to simulate 5G-LENA NR. Install a 5G-LENA-enabled ns-3 package and expose it through `CMAKE_PREFIX_PATH` to enable the NR branch.
