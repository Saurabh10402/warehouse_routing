#include "ealr/config/configuration.hpp"

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

template <typename Function>
void expectConfigurationError(Function function, const std::string& expectedText) {
    try {
        function();
        expect(false, "Expected ConfigurationError containing: " + expectedText);
    } catch (const ealr::ConfigurationError& error) {
        expect(std::string(error.what()).find(expectedText) != std::string::npos,
               "ConfigurationError should contain: " + expectedText);
    }
}

std::filesystem::path sourcePath(const std::string& relativePath) {
    return std::filesystem::path(EALR_SOURCE_DIRECTORY) / relativePath;
}

}  // namespace

int main() {
    const ealr::ExperimentConfiguration baseline = ealr::ConfigurationLoader::load(sourcePath("configs/baseline.json"));
    expect(baseline.routing.algorithm == ealr::RoutingAlgorithm::Dijkstra, "baseline uses Dijkstra");
    expect(baseline.robots.count == 16, "baseline robot count is read");
    expect(std::abs(baseline.battery.idleCostPerSecond - 0.001) < 0.00001, "baseline battery cost is read");
    expect(std::abs(baseline.private5g.centralFrequencyHz - 3500000000.0) < 0.00001, "private 5G carrier frequency is read");
    expect(baseline.private5g.numerology == 1, "private 5G numerology is read");

    const ealr::ExperimentConfiguration lowBattery = ealr::ConfigurationLoader::load(sourcePath("configs/low_battery.json"));
    expect(lowBattery.routing.algorithm == ealr::RoutingAlgorithm::Ealr, "derived configuration overrides routing algorithm");
    expect(lowBattery.robots.count == 16, "derived configuration inherits unmodified objects");
    expect(std::abs(lowBattery.battery.initialPercent - 35.0) < 0.00001, "derived configuration overrides battery values");

    expectConfigurationError(
        [] { ealr::ConfigurationLoader::load(sourcePath("tests/fixtures/missing_section.json")); },
        "Missing required property 'routing'");
    expectConfigurationError(
        [] { ealr::ConfigurationLoader::load(sourcePath("tests/fixtures/cycle_a.json")); },
        "Circular configuration inheritance");
    expectConfigurationError(
        [] { ealr::ConfigurationLoader::load(sourcePath("tests/fixtures/malformed.json")); },
        "Invalid JSON");
    expectConfigurationError(
        [] { ealr::ConfigurationLoader::load(sourcePath("tests/fixtures/invalid_battery_thresholds.json")); },
        "Battery thresholds must satisfy");

    return failures == 0 ? 0 : 1;
}
