#include "ealr/config/warehouse_configuration.hpp"
#include "ealr/network/device_policy.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Failure: " << message << '\n';
        ++failures;
    }
}

std::filesystem::path sourcePath(const std::string& relativePath) {
    return std::filesystem::path(EALR_SOURCE_DIRECTORY) / relativePath;
}

}  // namespace

int main() {
    const ealr::WarehouseNetworkConfiguration configuration =
        ealr::WarehouseConfigurationLoader::load(sourcePath("configs/warehouse_leaf_spine.json"));

    expect(configuration.topology.zoneCount == 10, "warehouse plan has ten zones");
    expect(configuration.topology.endpointsPerZone == 50, "warehouse plan has fifty endpoints per zone");
    expect(configuration.topology.spineSwitchCount == 2, "warehouse plan has two spines");
    expect(configuration.topology.headNodeCount == 2, "warehouse plan has two head nodes");
    expect(configuration.radioPolicy.requireDualHomedLeafUplinks, "leaf switches have redundant uplinks");
    expect(std::abs(configuration.temperatureSafety.warningCelsius - 70.0) < 0.00001, "warning threshold is configurable");
    expect(std::abs(configuration.temperatureSafety.stopCelsius - 80.0) < 0.00001, "stop threshold is configurable");
    expect(configuration.temperatureSafety.requireManualResetAfterStop, "emergency stop reset policy is configurable");
    expect(std::abs(configuration.latencyTargets.normalMaximumLatencyMs - 40.0) < 0.00001, "normal latency target is configurable");
    expect(configuration.deviceProfiles.size() == 4, "device mix is configuration driven");
    expect(configuration.deviceProfiles[0].accessTechnology == ealr::AccessTechnology::Wired, "critical temperature sensors use wired access by default");
    expect(configuration.deviceProfiles[1].priority == ealr::TrafficPriority::Control, "robot profile uses control priority");

    const ealr::QualityOfServicePolicy safetyPolicy =
        ealr::qualityOfServiceFor(ealr::TrafficPriority::Safety, configuration.latencyTargets);
    expect(safetyPolicy.strictPriorityQueue, "safety policy uses strict priority");
    expect(safetyPolicy.allowPreemption, "safety policy permits preemption");
    expect(safetyPolicy.dscp == 48, "safety policy marks CS6");
    expect(std::abs(safetyPolicy.maximumLatencyMs - 10.0) < 0.00001, "safety policy uses configurable target");

    return failures == 0 ? 0 : 1;
}
