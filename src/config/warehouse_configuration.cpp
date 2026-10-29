#include "ealr/config/warehouse_configuration.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace ealr {
namespace {

namespace json = boost::json;

[[noreturn]] void fail(const std::string& message) {
    throw WarehouseConfigurationError(message);
}

const json::value& requiredValue(const json::object& object, const char* key, const std::string& path) {
    const auto value = object.find(key);
    if (value == object.end()) {
        fail("Missing required property '" + std::string(key) + "' at " + path);
    }
    return value->value();
}

const json::object& requiredObject(const json::object& object, const char* key, const std::string& path) {
    const json::value& value = requiredValue(object, key, path);
    if (!value.is_object()) {
        fail("Property '" + std::string(key) + "' at " + path + " must be an object");
    }
    return value.as_object();
}

void validateKeys(const json::object& object, std::initializer_list<const char*> allowed, const std::string& path) {
    for (const auto& entry : object) {
        const bool known = std::any_of(allowed.begin(), allowed.end(), [&entry](const char* key) {
            return entry.key() == key;
        });
        if (!known) {
            fail("Unknown property '" + std::string(entry.key()) + "' at " + path);
        }
    }
}

double requiredNumber(const json::object& object, const char* key, const std::string& path) {
    const json::value& value = requiredValue(object, key, path);
    double number = 0.0;
    if (value.is_double()) {
        number = value.as_double();
    } else if (value.is_int64()) {
        number = static_cast<double>(value.as_int64());
    } else if (value.is_uint64()) {
        number = static_cast<double>(value.as_uint64());
    } else {
        fail("Property '" + std::string(key) + "' at " + path + " must be a number");
    }
    if (!std::isfinite(number)) {
        fail("Property '" + std::string(key) + "' at " + path + " must be finite");
    }
    return number;
}

std::uint32_t requiredPositiveInteger(const json::object& object, const char* key, const std::string& path) {
    const double number = requiredNumber(object, key, path);
    if (number < 1.0 || std::floor(number) != number || number > std::numeric_limits<std::uint32_t>::max()) {
        fail("Property '" + std::string(key) + "' at " + path + " must be a positive integer");
    }
    return static_cast<std::uint32_t>(number);
}

double requiredPositiveNumber(const json::object& object, const char* key, const std::string& path) {
    const double number = requiredNumber(object, key, path);
    if (number <= 0.0) {
        fail("Property '" + std::string(key) + "' at " + path + " must be greater than zero");
    }
    return number;
}

bool requiredBoolean(const json::object& object, const char* key, const std::string& path) {
    const json::value& value = requiredValue(object, key, path);
    if (!value.is_bool()) {
        fail("Property '" + std::string(key) + "' at " + path + " must be a boolean");
    }
    return value.as_bool();
}

std::string requiredString(const json::object& object, const char* key, const std::string& path) {
    const json::value& value = requiredValue(object, key, path);
    if (!value.is_string() || value.as_string().empty()) {
        fail("Property '" + std::string(key) + "' at " + path + " must be a non-empty string");
    }
    return std::string(value.as_string());
}

AccessTechnology parseAccessTechnology(const std::string& technology, const std::string& path) {
    if (technology == "wired") {
        return AccessTechnology::Wired;
    }
    if (technology == "wifi") {
        return AccessTechnology::Wifi;
    }
    if (technology == "private5g") {
        return AccessTechnology::Private5g;
    }
    fail("Property 'accessTechnology' at " + path + " must be one of: wired, wifi, private5g");
}

TrafficPriority parseTrafficPriority(const std::string& priority, const std::string& path) {
    if (priority == "safety") {
        return TrafficPriority::Safety;
    }
    if (priority == "control") {
        return TrafficPriority::Control;
    }
    if (priority == "normal") {
        return TrafficPriority::Normal;
    }
    if (priority == "background") {
        return TrafficPriority::Background;
    }
    fail("Property 'priority' at " + path + " must be one of: safety, control, normal, background");
}

std::vector<DeviceProfile> parseDeviceProfiles(const json::object& root) {
    const json::value& profilesValue = requiredValue(root, "deviceProfiles", "root");
    if (!profilesValue.is_array() || profilesValue.as_array().empty()) {
        fail("Property 'deviceProfiles' at root must be a non-empty array");
    }

    std::vector<DeviceProfile> profiles;
    for (std::size_t index = 0; index < profilesValue.as_array().size(); ++index) {
        const json::value& value = profilesValue.as_array()[index];
        const std::string path = "deviceProfiles[" + std::to_string(index) + "]";
        if (!value.is_object()) {
            fail("Property '" + path + "' must be an object");
        }
        const json::object& profile = value.as_object();
        validateKeys(profile, {"name", "count", "accessTechnology", "priority"}, path);
        DeviceProfile parsed{
            requiredString(profile, "name", path),
            requiredPositiveInteger(profile, "count", path),
            parseAccessTechnology(requiredString(profile, "accessTechnology", path), path),
            parseTrafficPriority(requiredString(profile, "priority", path), path)};
        const bool duplicateName = std::any_of(profiles.begin(), profiles.end(), [&parsed](const DeviceProfile& existing) {
            return existing.name == parsed.name;
        });
        if (duplicateName) {
            fail("Duplicate device profile name '" + parsed.name + "'");
        }
        profiles.push_back(std::move(parsed));
    }
    return profiles;
}

WarehouseNetworkConfiguration validate(const json::object& root) {
    validateKeys(root, {"name", "topology", "radioPolicy", "latencyTargets", "temperatureSafety", "deviceProfiles"}, "root");
    const json::object& topology = requiredObject(root, "topology", "root");
    validateKeys(topology, {"zoneCount", "endpointsPerZone", "leafSwitchesPerZone", "spineSwitchCount", "headNodeCount", "leafUplinkGbps", "spineUplinkGbps"}, "topology");
    const json::object& radio = requiredObject(root, "radioPolicy", "root");
    validateKeys(radio, {"allowWiredCriticalDevices", "allowWifiDevices", "allowPrivate5gDevices", "requireDualHomedLeafUplinks"}, "radioPolicy");
    const json::object& latency = requiredObject(root, "latencyTargets", "root");
    validateKeys(latency, {"safetyMaximumLatencyMs", "controlMaximumLatencyMs", "normalMaximumLatencyMs", "backgroundMaximumLatencyMs"}, "latencyTargets");
    const json::object& temperature = requiredObject(root, "temperatureSafety", "root");
    validateKeys(temperature, {"warningCelsius", "stopCelsius", "broadcastEmergencyStop", "processAtLocalEdge", "requireManualResetAfterStop"}, "temperatureSafety");

    const std::vector<DeviceProfile> profiles = parseDeviceProfiles(root);
    WarehouseNetworkConfiguration configuration{
        requiredString(root, "name", "root"),
        {requiredPositiveInteger(topology, "zoneCount", "topology"), requiredPositiveInteger(topology, "endpointsPerZone", "topology"), requiredPositiveInteger(topology, "leafSwitchesPerZone", "topology"), requiredPositiveInteger(topology, "spineSwitchCount", "topology"), requiredPositiveInteger(topology, "headNodeCount", "topology"), requiredPositiveInteger(topology, "leafUplinkGbps", "topology"), requiredPositiveInteger(topology, "spineUplinkGbps", "topology")},
        {requiredBoolean(radio, "allowWiredCriticalDevices", "radioPolicy"), requiredBoolean(radio, "allowWifiDevices", "radioPolicy"), requiredBoolean(radio, "allowPrivate5gDevices", "radioPolicy"), requiredBoolean(radio, "requireDualHomedLeafUplinks", "radioPolicy")},
        {requiredPositiveNumber(latency, "safetyMaximumLatencyMs", "latencyTargets"), requiredPositiveNumber(latency, "controlMaximumLatencyMs", "latencyTargets"), requiredPositiveNumber(latency, "normalMaximumLatencyMs", "latencyTargets"), requiredPositiveNumber(latency, "backgroundMaximumLatencyMs", "latencyTargets")},
        {requiredPositiveNumber(temperature, "warningCelsius", "temperatureSafety"), requiredPositiveNumber(temperature, "stopCelsius", "temperatureSafety"), requiredBoolean(temperature, "broadcastEmergencyStop", "temperatureSafety"), requiredBoolean(temperature, "processAtLocalEdge", "temperatureSafety"), requiredBoolean(temperature, "requireManualResetAfterStop", "temperatureSafety")},
        profiles
    };

    if (configuration.topology.spineSwitchCount < 2) {
        fail("Property 'spineSwitchCount' at topology must be at least 2 for a redundant leaf-spine network");
    }
    if (configuration.topology.headNodeCount < 2) {
        fail("Property 'headNodeCount' at topology must be at least 2 for redundant edge services");
    }
    if (configuration.radioPolicy.requireDualHomedLeafUplinks && configuration.topology.spineSwitchCount < 2) {
        fail("Dual-homed leaf uplinks require at least two spine switches");
    }
    if (configuration.latencyTargets.safetyMaximumLatencyMs > configuration.latencyTargets.controlMaximumLatencyMs ||
        configuration.latencyTargets.controlMaximumLatencyMs > configuration.latencyTargets.normalMaximumLatencyMs ||
        configuration.latencyTargets.normalMaximumLatencyMs > configuration.latencyTargets.backgroundMaximumLatencyMs) {
        fail("Latency targets must satisfy safety <= control <= normal <= background");
    }
    if (configuration.temperatureSafety.warningCelsius >= configuration.temperatureSafety.stopCelsius) {
        fail("Temperature thresholds must satisfy warningCelsius < stopCelsius");
    }
    if (!configuration.temperatureSafety.processAtLocalEdge) {
        fail("Temperature safety events must be processed at the local edge");
    }
    std::uint32_t profileEndpointCount = 0;
    for (const DeviceProfile& profile : configuration.deviceProfiles) {
        profileEndpointCount += profile.count;
        if (profile.accessTechnology == AccessTechnology::Wired && !configuration.radioPolicy.allowWiredCriticalDevices) {
            fail("Wired device profiles require allowWiredCriticalDevices to be true");
        }
        if (profile.accessTechnology == AccessTechnology::Wifi && !configuration.radioPolicy.allowWifiDevices) {
            fail("Wi-Fi device profiles require allowWifiDevices to be true");
        }
        if (profile.accessTechnology == AccessTechnology::Private5g && !configuration.radioPolicy.allowPrivate5gDevices) {
            fail("Private-5G device profiles require allowPrivate5gDevices to be true");
        }
    }
    const std::uint32_t plannedEndpointCount = configuration.topology.zoneCount * configuration.topology.endpointsPerZone;
    if (profileEndpointCount != plannedEndpointCount) {
        fail("Device profile counts must equal zoneCount * endpointsPerZone");
    }
    return configuration;
}

}  // namespace

WarehouseNetworkConfiguration WarehouseConfigurationLoader::load(const std::filesystem::path& configurationPath) {
    std::ifstream input(configurationPath);
    if (!input) {
        fail("Unable to open warehouse configuration file '" + configurationPath.string() + "'");
    }
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    boost::system::error_code parseError;
    const json::value parsed = json::parse(contents, parseError);
    if (parseError || !parsed.is_object()) {
        fail("Invalid JSON warehouse configuration file '" + configurationPath.string() + "'");
    }
    return validate(parsed.as_object());
}

const char* toString(AccessTechnology technology) {
    switch (technology) {
        case AccessTechnology::Wired:
            return "wired";
        case AccessTechnology::Wifi:
            return "wifi";
        case AccessTechnology::Private5g:
            return "private5g";
    }
    return "unknown";
}

const char* toString(TrafficPriority priority) {
    switch (priority) {
        case TrafficPriority::Safety:
            return "safety";
        case TrafficPriority::Control:
            return "control";
        case TrafficPriority::Normal:
            return "normal";
        case TrafficPriority::Background:
            return "background";
    }
    return "unknown";
}

}  // namespace ealr
