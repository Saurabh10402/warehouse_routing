#pragma once
#include "ealr/config/fabric_configuration.hpp"
#include <cstdint>

namespace ealr::fabric {
struct Ns3PacketRecord {
    std::size_t id = 0, flowIndex = 0;
    std::string status = "pending";
    double createdMs = 0, finishedMs = 0;
    std::vector<std::string> observedPath, expectedPath;
};
struct Ns3HopRecord {
    std::size_t packet = 0;
    std::string link, from, to, event;
    double timeMs = 0;
    unsigned wireBytes = 0;
    unsigned ipTos = 0, sourcePort = 0, destinationPort = 0;
    std::uint32_t sourceAddress = 0, destinationAddress = 0;
};
struct Ns3QueueRecord {
    std::string link, from, to;
    std::uint64_t attempts = 0, admitted = 0, started = 0, dropped = 0, waiting = 0, peak = 0;
};
struct Ns3FabricResult {
    std::vector<Ns3PacketRecord> packets;
    std::vector<Ns3HopRecord> hops;
    std::vector<Ns3QueueRecord> queues;
    std::vector<std::array<double, 3>> finalPositions;
    std::uint64_t ipTransmitted = 0, ipReceived = 0, ipUnreceivedAtCutoff = 0, ipReceivedBytes = 0;
    bool stationary = false;
};
// Strict supported subset; throws before any ns-3 objects are created.
void validateNs3Fabric(const Configuration& configuration);
Ns3FabricResult simulateNs3Fabric(const Configuration& configuration);
void writeNs3FabricResults(const Configuration& configuration, const Ns3FabricResult& result,
                          const std::filesystem::path& output);
} // namespace ealr::fabric
