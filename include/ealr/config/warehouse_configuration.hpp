#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace ealr {

/** A vendor-neutral description of the planned warehouse network. */
struct WarehouseTopologyConfiguration {
    std::uint32_t zoneCount;
    std::uint32_t endpointsPerZone;
    std::uint32_t leafSwitchesPerZone;
    std::uint32_t spineSwitchCount;
    std::uint32_t headNodeCount;
    std::uint32_t leafUplinkGbps;
    std::uint32_t spineUplinkGbps;
};

struct RadioPolicyConfiguration {
    bool allowWiredCriticalDevices;
    bool allowWifiDevices;
    bool allowPrivate5gDevices;
    bool requireDualHomedLeafUplinks;
};

struct LatencyTargetConfiguration {
    double safetyMaximumLatencyMs;
    double controlMaximumLatencyMs;
    double normalMaximumLatencyMs;
    double backgroundMaximumLatencyMs;
};

struct TemperatureSafetyConfiguration {
    double warningCelsius;
    double stopCelsius;
    bool broadcastEmergencyStop;
    bool processAtLocalEdge;
    bool requireManualResetAfterStop;
};

enum class AccessTechnology {
    Wired,
    Wifi,
    Private5g
};

enum class TrafficPriority {
    Safety,
    Control,
    Normal,
    Background
};

/** A configurable group of endpoints with the same access and QoS policy. */
struct DeviceProfile {
    std::string name;
    std::uint32_t count;
    AccessTechnology accessTechnology;
    TrafficPriority priority;
};

struct WarehouseNetworkConfiguration {
    std::string name;
    WarehouseTopologyConfiguration topology;
    RadioPolicyConfiguration radioPolicy;
    LatencyTargetConfiguration latencyTargets;
    TemperatureSafetyConfiguration temperatureSafety;
    std::vector<DeviceProfile> deviceProfiles;
};

/** Identifies an invalid warehouse-network JSON document or value. */
class WarehouseConfigurationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/** Loads and validates a standalone, editable warehouse-network plan. */
class WarehouseConfigurationLoader {
public:
    static WarehouseNetworkConfiguration load(const std::filesystem::path& configurationPath);
};

const char* toString(AccessTechnology technology);
const char* toString(TrafficPriority priority);

}  // namespace ealr
