#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace ealr {

enum class RoutingAlgorithm {
    Dijkstra,
    Greedy,
    Ealr
};

enum class MobilityModel {
    Static,
    Grid,
    RandomWaypoint
};

struct SimulationConfiguration {
    double durationSeconds;
    std::uint32_t seed;
};

struct RoutingConfiguration {
    RoutingAlgorithm algorithm;
    double alpha;
    double beta;
    double gamma;
};

struct BatteryConfiguration {
    double initialPercent;
    double lowBatteryThreshold;
    double criticalBatteryThreshold;
    double transmitCost;
    double receiveCost;
    double relayCost;
    double idleCostPerSecond;
};

struct TrafficConfiguration {
    std::uint32_t packetsPerSecond;
    std::uint32_t packetSizeBytes;
};

struct MobilityConfiguration {
    MobilityModel model;
    double factoryWidthMeters;
    double factoryHeightMeters;
    double randomWaypointMinSpeedMetersPerSecond;
    double randomWaypointMaxSpeedMetersPerSecond;
    double randomWaypointPauseSeconds;
};

struct RobotConfiguration {
    std::uint32_t count;
    double communicationRadiusMeters;
};

struct ChargingStationConfiguration {
    double x;
    double y;
};

struct MetricsConfiguration {
    double sampleIntervalSeconds;
};

struct Private5gConfiguration {
    double gnbX;
    double gnbY;
    double centralFrequencyHz;
    double channelBandwidthHz;
    std::uint32_t numerology;
    double gnbTxPowerDbm;
    double ueTxPowerDbm;
    std::string channelScenario;
    std::string channelCondition;
    std::string channelModel;
};

struct OutputConfiguration {
    std::filesystem::path directory;
};

struct ExperimentConfiguration {
    SimulationConfiguration simulation;
    RoutingConfiguration routing;
    BatteryConfiguration battery;
    TrafficConfiguration traffic;
    MobilityConfiguration mobility;
    RobotConfiguration robots;
    ChargingStationConfiguration chargingStation;
    MetricsConfiguration metrics;
    Private5gConfiguration private5g;
    OutputConfiguration output;
};

/** Identifies an invalid JSON document or an invalid experiment value. */
class ConfigurationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/** Loads, inherits, validates, and converts JSON scenario files to domain data. */
class ConfigurationLoader {
public:
    static ExperimentConfiguration load(const std::filesystem::path& configurationPath);
};

const char* toString(RoutingAlgorithm algorithm);
const char* toString(MobilityModel model);

}  // namespace ealr
