#pragma once

#include <cstdint>
#include <filesystem>

namespace ealr {

struct FlowStatistics {
    std::uint64_t transmittedPackets = 0;
    std::uint64_t transmittedBytes = 0;
    std::uint64_t receivedPackets = 0;
    std::uint64_t receivedBytes = 0;
    std::uint64_t lostPackets = 0;
    double totalDelaySeconds = 0.0;
};

struct SimulationMetrics {
    double averageEndToEndDelayMilliseconds = 0.0;
    double throughputMbps = 0.0;
    double packetDeliveryRatioPercent = 0.0;
    double packetLossPercent = 0.0;
    std::uint64_t transmittedPackets = 0;
    std::uint64_t receivedPackets = 0;
    std::uint64_t lostPackets = 0;
    double averageBatteryRemainingPercent = 0.0;
    double minimumBatteryRemainingPercent = 0.0;
    std::uint32_t chargingEvents = 0;
    std::uint32_t robotsRemovedFromRouting = 0;
    double networkLifetimeSeconds = 0.0;
    std::uint32_t routeRecomputations = 0;
};

/**
 * Accumulates simulator-independent metrics and exports stable experiment
 * reports. FlowMonitor data is converted to FlowStatistics by the ns-3 layer.
 */
class MetricsCollector {
public:
    void recordFlowStatistics(const FlowStatistics& statistics);
    void recordBatteryLevel(double batteryPercent);
    void recordChargingEvent();
    void recordRobotRemovedFromRouting(double simulationTimeSeconds);
    void recordRouteRecomputation();

    SimulationMetrics summarize(double simulationDurationSeconds) const;
    void writeCsv(const std::filesystem::path& outputPath, double simulationDurationSeconds) const;
    void writeSummary(const std::filesystem::path& outputPath, double simulationDurationSeconds) const;

private:
    FlowStatistics flowStatistics_;
    double batteryTotal_ = 0.0;
    double minimumBatteryPercent_ = 0.0;
    std::uint64_t batterySampleCount_ = 0;
    std::uint32_t chargingEvents_ = 0;
    std::uint32_t robotsRemovedFromRouting_ = 0;
    double firstRobotRemovalTimeSeconds_ = -1.0;
    std::uint32_t routeRecomputations_ = 0;
};

}  // namespace ealr
