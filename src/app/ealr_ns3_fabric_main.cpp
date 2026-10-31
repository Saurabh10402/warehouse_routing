#include "ealr/simulation/ns3_fabric.hpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        std::filesystem::path config, output;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--config" && config.empty() && i + 1 < argc) config = argv[++i];
            else if (argument == "--output" && output.empty() && i + 1 < argc) output = argv[++i];
            else throw std::invalid_argument("Usage: ealr-ns3-fabric --config scenario.json --output NEW_DIRECTORY");
        }
        if (config.empty() || output.empty() || std::filesystem::exists(output))
            throw std::invalid_argument("Provide a configuration and a new output directory; existing results are never overwritten");
        auto configuration = ealr::fabric::load(config);
        auto result = ealr::fabric::simulateNs3Fabric(configuration);
        ealr::fabric::writeNs3FabricResults(configuration, result, output);
        std::filesystem::copy_file(config, output / "config.json");
        std::cout << "Actual ns-3 stationary wired fabric: " << result.ipReceived << '/' << result.packets.size()
                  << " delivered; output " << output << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ns-3 fabric failed: " << error.what() << '\n'; return 1;
    }
}
