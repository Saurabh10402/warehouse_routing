#include "ealr/config/configuration.hpp"
#include "ealr/simulation/ns3_private_5g_scenario.hpp"

#include <filesystem>
#include <iostream>

int main() {
    const std::filesystem::path sourceDirectory(EALR_SOURCE_DIRECTORY);
    ealr::ExperimentConfiguration configuration = ealr::ConfigurationLoader::load(sourceDirectory / "configs/baseline.json");
    configuration.simulation.durationSeconds = 0.1;

    const std::filesystem::path animationPath = std::filesystem::temp_directory_path() / "ealr-ns3-scenario-test.xml";
    const ealr::Ns3Private5gScenario scenario(configuration);
    const ealr::Ns3ScenarioResult result = scenario.run(animationPath);

    if (result.robotNodeCount != configuration.robots.count || result.animationPath != animationPath || !std::filesystem::exists(animationPath)) {
        std::cerr << "ns-3 scenario did not create the expected NetAnim output\n";
        return 1;
    }
    return 0;
}
