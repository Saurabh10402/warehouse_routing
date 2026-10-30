#pragma once
#include "ealr/config/fabric_configuration.hpp"
#include <cstdint>

namespace ealr::fabric {
struct PacketRecord {
    std::size_t id = 0;
    std::string flow, source, destination, status = "pending";
    unsigned priority = 0, bytes = 0;
    double createdMs = 0, finishedMs = 0;
    std::vector<std::string> path;
};
struct SafetyRecord {
    std::string zone, state;
    double readingMs = 0, responseMs = 0;
    bool controllerAvailable = true;
};
struct EventRecord { double timeMs; std::string type, target; bool available; };
struct QueueStatistics {
    std::uint64_t attempts=0, admitted=0, serviceStarts=0, bytesStarted=0;
    std::uint64_t overflowDrops=0, reservationDrops=0, policedDrops=0, rerouted=0, queuedAtCutoff=0, peakWaiting=0;
    double totalWaitMs=0, maxWaitMs=0;
};
struct QueueRecord { std::string link, from, to; unsigned priority; QueueStatistics statistics; };
struct SimulationResult {
    std::vector<PacketRecord> packets;
    std::vector<SafetyRecord> safetyEvents;
    std::vector<EventRecord> events;
    std::vector<Node> finalNodes;
    std::vector<QueueRecord> queues;
    std::uint64_t routeComputations = 0, latencyGuardFallbacks = 0;
    bool stationary = false;
    // First constrained device to reach critical threshold; -1 means censored/not observed.
    double firstEnergyExclusionMs = -1;
};
SimulationResult simulate(const Configuration& configuration);
} // namespace ealr::fabric
