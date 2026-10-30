#pragma once

#include "ealr/config/warehouse_configuration.hpp"
#include "ealr/robot/robot.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace ealr::fabric {

struct Node {
    std::string id, role, zone, access;
    Position2D position{};
    bool available = true;
    bool energyConstrained = false;
    double batteryPercent = 100, criticalPercent = 10, txPercentPerByte = 0, rxPercentPerByte = 0;
};
// Full duplex: each direction has an independent transmitter and finite waiting queue.
struct Link {
    std::string id, from, to;
    double capacityMbps = 100, delayMs = 0.1;
    unsigned queuePackets = 100;
    bool available = true;
};
struct Workload {
    std::string id, source;
    std::vector<std::string> destinations; // Ordered primary/backup service candidates.
    unsigned priority = 2, bytes = 512;
    double startMs = 0, stopMs = 100, intervalMs = 10, phaseJitterMs = 0;
};
struct Event {
    std::string type, target, zone;
    double timeMs = 0, temperatureCelsius = 20;
    bool available = false;
};
struct RoutingPolicy {
    std::string algorithm = "dijkstra";
    double alpha = 1, beta = 1, gamma = 1, latencyScaleMs = 1, maxStretch = 1.5;
};
struct ProtectedQueuePolicy {
    bool emergencyPolicingEnabled = true;
    unsigned reservedEmergencyPackets = 8, emergencyBurstBytes = 512;
    double emergencyRateFraction = 0.2;
    std::array<unsigned,3> lowerClassWeights{{4,2,1}};
};
struct Configuration {
    std::string name, topology = "graph", scheduler = "strictPriority";
    unsigned seed = 1, minimumSpinesPerLeaf = 0, minimumEdges = 0;
    double durationMs = 1000, drainMs = 100, safetyProcessingMs = 0.1;
    std::array<double,4> deadlinesMs{{10,20,40,100}};
    TemperatureSafetyConfiguration safety{70,80,true,true,true};
    RoutingPolicy routing;
    ProtectedQueuePolicy protectedQueues;
    std::vector<std::string> zones;
    std::vector<Node> nodes;
    std::vector<Link> links;
    std::vector<Workload> traffic;
    std::vector<Event> events;
};

Configuration load(const std::filesystem::path& path);
// Also validates programmatically created configurations, before any simulation side effects.
void validate(const Configuration& configuration);
} // namespace ealr::fabric
