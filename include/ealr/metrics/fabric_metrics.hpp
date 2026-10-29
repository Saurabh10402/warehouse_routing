#pragma once
#include "ealr/simulation/fabric_simulator.hpp"
#include <optional>

namespace ealr::fabric {
struct PacketMetrics {
    std::size_t sent=0, delivered=0, lost=0, deadlineMisses=0;
    std::uint64_t receivedBytes=0;
    double meanMs=0, medianMs=0, deliveryPercent=0, deadlineMissPercent=0, throughputMbps=0;
    std::optional<double> p95Ms, p99Ms;
};
PacketMetrics summarize(const Configuration& c,const SimulationResult& r,int priority=-1);
// Output must not exist: prevents silent overwrite of research results.
void writeResults(const Configuration& c,const SimulationResult& r,const std::filesystem::path& output);
} // namespace ealr::fabric
