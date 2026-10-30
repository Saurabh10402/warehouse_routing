#include "ealr/config/configuration.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <vector>

using namespace std;

namespace ealr {
namespace {

namespace json = boost::json;

[[noreturn]] void fail(const string& message) {
    throw ConfigurationError(message);
}

string quotedPath(const filesystem::path& path) {
    return "'" + path.string() + "'";
}

filesystem::path normalizedPath(const filesystem::path& path) {
    error_code error;
    filesystem::path absolute = filesystem::absolute(path, error);
    if (error) {
        fail("Unable to resolve configuration path " + quotedPath(path) + ": " + error.message());
    }
    return absolute.lexically_normal();
}

json::object mergeObjects(json::object base, const json::object& overrides) {
    for (const auto& entry : overrides) {
        auto existing = base.find(entry.key());
        if (existing != base.end() && existing->value().is_object() && entry.value().is_object()) {
            existing->value() = mergeObjects(existing->value().as_object(), entry.value().as_object());
        } else {
            base[entry.key()] = entry.value();
        }
    }
    return base;
}

json::object loadDocument(const filesystem::path& filePath, vector<filesystem::path>& ancestry) {
    const filesystem::path resolvedPath = normalizedPath(filePath);
    if (find(ancestry.begin(), ancestry.end(), resolvedPath) != ancestry.end()) {
        fail("Circular configuration inheritance detected at " + quotedPath(resolvedPath));
    }

    ifstream input(resolvedPath);
    if (!input) {
        fail("Unable to open configuration file " + quotedPath(resolvedPath));
    }

    const string contents((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    boost::system::error_code parseError;
    json::value parsed = json::parse(contents, parseError);
    if (parseError) {
        fail("Invalid JSON in " + quotedPath(resolvedPath) + ": " + parseError.message());
    }
    if (!parsed.is_object()) {
        fail("Configuration root in " + quotedPath(resolvedPath) + " must be a JSON object");
    }

    json::object document = parsed.as_object();
    const auto extends = document.find("extends");
    if (extends == document.end()) {
        return document;
    }
    if (!extends->value().is_string()) {
        fail("Property 'extends' in " + quotedPath(resolvedPath) + " must be a string");
    }

    const filesystem::path parentPath = resolvedPath.parent_path() / string(extends->value().as_string());
    document.erase(extends);
    ancestry.push_back(resolvedPath);
    json::object parent = loadDocument(parentPath, ancestry);
    ancestry.pop_back();
    return mergeObjects(std::move(parent), document);
}

const json::value& requiredValue(const json::object& object, const char* key, std::string_view path) {
    const auto value = object.find(key);
    if (value == object.end()) {
        fail("Missing required property '" + string(key) + "' at " + std::string(path));
    }
    return value->value();
}

const json::object& requiredObject(const json::object& object, const char* key, std::string_view path) {
    const json::value& value = requiredValue(object, key, path);
    if (!value.is_object()) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be an object");
    }
    return value.as_object();
}

void validateKeys(const json::object& object, initializer_list<const char*> allowed, std::string_view path) {
    for (const auto& entry : object) {
        const bool known = any_of(allowed.begin(), allowed.end(), [&entry](const char* key) {
            return entry.key() == key;
        });
        if (!known) {
            fail("Unknown property '" + string(entry.key()) + "' at " + std::string(path));
        }
    }
}

double requiredNumber(const json::object& object, const char* key, std::string_view path) {
    const json::value& value = requiredValue(object, key, path);
    double number = 0.0;
    if (value.is_double()) {
        number = value.as_double();
    } else if (value.is_int64()) {
        number = static_cast<double>(value.as_int64());
    } else if (value.is_uint64()) {
        number = static_cast<double>(value.as_uint64());
    } else {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be a number");
    }
    if (!isfinite(number)) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be finite");
    }
    return number;
}

uint32_t requiredPositiveInteger(const json::object& object, const char* key, std::string_view path) {
    const double number = requiredNumber(object, key, path);
    if (number < 1.0 || floor(number) != number || number > numeric_limits<uint32_t>::max()) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be a positive integer");
    }
    return static_cast<uint32_t>(number);
}

uint32_t requiredNonNegativeInteger(const json::object& object, const char* key, std::string_view path) {
    const double number = requiredNumber(object, key, path);
    if (number < 0.0 || floor(number) != number || number > numeric_limits<uint32_t>::max()) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be a non-negative integer");
    }
    return static_cast<uint32_t>(number);
}

double requiredPositiveNumber(const json::object& object, const char* key, std::string_view path) {
    const double number = requiredNumber(object, key, path);
    if (number <= 0.0) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be greater than zero");
    }
    return number;
}

double requiredNonNegativeNumber(const json::object& object, const char* key, std::string_view path) {
    const double number = requiredNumber(object, key, path);
    if (number < 0.0) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must not be negative");
    }
    return number;
}

string requiredString(const json::object& object, const char* key, std::string_view path) {
    const json::value& value = requiredValue(object, key, path);
    if (!value.is_string()) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must be a string");
    }
    const string result(value.as_string());
    if (result.empty()) {
        fail("Property '" + string(key) + "' at " + std::string(path) + " must not be empty");
    }
    return result;
}

RoutingAlgorithmKind parseRoutingAlgorithmKind(const string& algorithm, std::string_view path) {
    if (algorithm == "dijkstra") {
        return RoutingAlgorithmKind::Dijkstra;
    }
    if (algorithm == "greedy") {
        return RoutingAlgorithmKind::Greedy;
    }
    if (algorithm == "ealr") {
        return RoutingAlgorithmKind::Ealr;
    }
    fail("Property 'algorithm' at " + std::string(path) + " must be one of: dijkstra, greedy, ealr");
}

MobilityModel parseMobilityModel(const string& model, std::string_view path) {
    if (model == "static") {
        return MobilityModel::Static;
    }
    if (model == "grid") {
        return MobilityModel::Grid;
    }
    if (model == "randomWaypoint") {
        return MobilityModel::RandomWaypoint;
    }
    fail("Property 'model' at " + std::string(path) + " must be one of: static, grid, randomWaypoint");
}

ExperimentConfiguration validate(const json::object& root) {
    validateKeys(root, {"simulation", "routing", "battery", "traffic", "mobility", "robots", "chargingStation", "metrics", "private5g", "output"}, "root");

    const json::object& simulation = requiredObject(root, "simulation", "root");
    validateKeys(simulation, {"durationSeconds", "seed"}, "simulation");

    const json::object& routing = requiredObject(root, "routing", "root");
    validateKeys(routing, {"algorithm", "alpha", "beta", "gamma"}, "routing");

    const json::object& battery = requiredObject(root, "battery", "root");
    validateKeys(battery, {"initialPercent", "lowBatteryThreshold", "criticalBatteryThreshold", "transmitCost", "receiveCost", "relayCost", "idleCostPerSecond"}, "battery");

    const json::object& traffic = requiredObject(root, "traffic", "root");
    validateKeys(traffic, {"packetsPerSecond", "packetSizeBytes"}, "traffic");

    const json::object& mobility = requiredObject(root, "mobility", "root");
    validateKeys(mobility, {"model", "factoryWidthMeters", "factoryHeightMeters", "randomWaypointMinSpeedMetersPerSecond", "randomWaypointMaxSpeedMetersPerSecond", "randomWaypointPauseSeconds"}, "mobility");

    const json::object& robots = requiredObject(root, "robots", "root");
    validateKeys(robots, {"count", "communicationRadiusMeters"}, "robots");

    const json::object& chargingStation = requiredObject(root, "chargingStation", "root");
    validateKeys(chargingStation, {"x", "y"}, "chargingStation");

    const json::object& metrics = requiredObject(root, "metrics", "root");
    validateKeys(metrics, {"sampleIntervalSeconds"}, "metrics");

    const json::object& private5g = requiredObject(root, "private5g", "root");
    validateKeys(private5g, {"gnbX", "gnbY", "centralFrequencyHz", "channelBandwidthHz", "numerology", "gnbTxPowerDbm", "ueTxPowerDbm", "channelScenario", "channelCondition", "channelModel"}, "private5g");

    const json::object& output = requiredObject(root, "output", "root");
    validateKeys(output, {"directory"}, "output");

    ExperimentConfiguration configuration{
        {requiredPositiveNumber(simulation, "durationSeconds", "simulation"), requiredPositiveInteger(simulation, "seed", "simulation")},
        {parseRoutingAlgorithmKind(requiredString(routing, "algorithm", "routing"), "routing"), requiredNonNegativeNumber(routing, "alpha", "routing"), requiredNonNegativeNumber(routing, "beta", "routing"), requiredNonNegativeNumber(routing, "gamma", "routing")},
        {requiredNumber(battery, "initialPercent", "battery"), requiredNumber(battery, "lowBatteryThreshold", "battery"), requiredNumber(battery, "criticalBatteryThreshold", "battery"), requiredNonNegativeNumber(battery, "transmitCost", "battery"), requiredNonNegativeNumber(battery, "receiveCost", "battery"), requiredNonNegativeNumber(battery, "relayCost", "battery"), requiredNonNegativeNumber(battery, "idleCostPerSecond", "battery")},
        {requiredPositiveInteger(traffic, "packetsPerSecond", "traffic"), requiredPositiveInteger(traffic, "packetSizeBytes", "traffic")},
        {parseMobilityModel(requiredString(mobility, "model", "mobility"), "mobility"), requiredPositiveNumber(mobility, "factoryWidthMeters", "mobility"), requiredPositiveNumber(mobility, "factoryHeightMeters", "mobility"), requiredPositiveNumber(mobility, "randomWaypointMinSpeedMetersPerSecond", "mobility"), requiredPositiveNumber(mobility, "randomWaypointMaxSpeedMetersPerSecond", "mobility"), requiredNonNegativeNumber(mobility, "randomWaypointPauseSeconds", "mobility")},
        {requiredPositiveInteger(robots, "count", "robots"), requiredPositiveNumber(robots, "communicationRadiusMeters", "robots")},
        {requiredNumber(chargingStation, "x", "chargingStation"), requiredNumber(chargingStation, "y", "chargingStation")},
        {requiredPositiveNumber(metrics, "sampleIntervalSeconds", "metrics")},
        {requiredNumber(private5g, "gnbX", "private5g"), requiredNumber(private5g, "gnbY", "private5g"), requiredPositiveNumber(private5g, "centralFrequencyHz", "private5g"), requiredPositiveNumber(private5g, "channelBandwidthHz", "private5g"), requiredNonNegativeInteger(private5g, "numerology", "private5g"), requiredNumber(private5g, "gnbTxPowerDbm", "private5g"), requiredNumber(private5g, "ueTxPowerDbm", "private5g"), requiredString(private5g, "channelScenario", "private5g"), requiredString(private5g, "channelCondition", "private5g"), requiredString(private5g, "channelModel", "private5g")},
        {filesystem::path(requiredString(output, "directory", "output"))}
    };

    if (configuration.battery.initialPercent < 0.0 || configuration.battery.initialPercent > 100.0) {
        fail("Property 'initialPercent' at battery must be between 0 and 100");
    }
    if (configuration.battery.criticalBatteryThreshold < 0.0 || configuration.battery.criticalBatteryThreshold >= configuration.battery.lowBatteryThreshold || configuration.battery.lowBatteryThreshold > 100.0) {
        fail("Battery thresholds must satisfy 0 <= criticalBatteryThreshold < lowBatteryThreshold <= 100");
    }
    if (configuration.mobility.randomWaypointMinSpeedMetersPerSecond > configuration.mobility.randomWaypointMaxSpeedMetersPerSecond) {
        fail("Random waypoint speed bounds must satisfy min <= max");
    }
    if (configuration.private5g.numerology > 5) {
        fail("Property 'numerology' at private5g must be between 0 and 5");
    }

    return configuration;
}

}  // namespace

ExperimentConfiguration ConfigurationLoader::load(const filesystem::path& configurationPath) {
    vector<filesystem::path> ancestry;
    return validate(loadDocument(configurationPath, ancestry));
}

const char* toString(RoutingAlgorithmKind algorithm) {
    switch (algorithm) {
        case RoutingAlgorithmKind::Dijkstra:
            return "dijkstra";
        case RoutingAlgorithmKind::Greedy:
            return "greedy";
        case RoutingAlgorithmKind::Ealr:
            return "ealr";
    }
    return "unknown";
}

const char* toString(MobilityModel model) {
    switch (model) {
        case MobilityModel::Static:
            return "static";
        case MobilityModel::Grid:
            return "grid";
        case MobilityModel::RandomWaypoint:
            return "randomWaypoint";
    }
    return "unknown";
}

}  // namespace ealr
