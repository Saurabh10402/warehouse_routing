#pragma once

#include "ealr/config/configuration.hpp"

#include <cstdint>
#include <filesystem>

namespace ealr {

struct Ns3ScenarioResult {
    std::uint32_t robotNodeCount;
    bool uses5gLenaNr;
    std::filesystem::path animationPath;
};

/**
 * Creates an ns-3 factory-floor scenario from a validated experiment config.
 *
 * The runner always materializes robot and gNB nodes, configured mobility, and
 * a NetAnim trace. When the build has an `ns3::nr` target, it additionally
 * installs 5G-LENA NR gNB and UE devices using the `private5g` configuration.
 */
class Ns3Private5gScenario {
public:
    explicit Ns3Private5gScenario(ExperimentConfiguration configuration);

    Ns3ScenarioResult run(const std::filesystem::path& animationPath) const;

    static bool has5gLenaNrSupport();

private:
    ExperimentConfiguration configuration_;
};

}  // namespace ealr
