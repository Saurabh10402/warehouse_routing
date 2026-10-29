#include "ealr/config/configuration.hpp"
#include "ealr/simulation/ns3_private_5g_scenario.hpp"

#include <exception>
#include <iostream>
#include <string>

using namespace std;

namespace {

void printUsage(const char* programName) {
    cerr << "Usage: " << programName << " --config <scenario.json> --animation <animation.xml>\n";
}

}  // namespace

int main(int argumentCount, char* arguments[]) {
    string configurationPath;
    string animationPath;
    for (int index = 1; index < argumentCount; ++index) {
        const string argument = arguments[index];
        if ((argument == "--config" || argument == "--animation") && index + 1 < argumentCount) {
            const string value = arguments[++index];
            if (argument == "--config") {
                configurationPath = value;
            } else {
                animationPath = value;
            }
        } else {
            printUsage(arguments[0]);
            return 2;
        }
    }
    if (configurationPath.empty() || animationPath.empty()) {
        printUsage(arguments[0]);
        return 2;
    }

    try {
        const ealr::ExperimentConfiguration configuration = ealr::ConfigurationLoader::load(configurationPath);
        const ealr::Ns3Private5gScenario scenario(configuration);
        const ealr::Ns3ScenarioResult result = scenario.run(animationPath);
        cout << "Generated NetAnim trace for " << result.robotNodeCount << " robots at " << result.animationPath << '\n';
        cout << "5G-LENA NR devices installed: " << (result.uses5gLenaNr ? "yes" : "no") << '\n';
        return 0;
    } catch (const exception& error) {
        cerr << "Simulation setup failed: " << error.what() << '\n';
        return 1;
    }
}
