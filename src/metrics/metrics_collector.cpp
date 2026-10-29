#include "ealr/metrics/metrics_collector.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>

using namespace std;

namespace ealr {
namespace {

void validateFiniteNonNegative(double value, const char* metricName) {
    if (!isfinite(value) || value < 0.0) {
        throw invalid_argument(string(metricName) + " must be finite and non-negative");
    }
}

ofstream openOutputFile(const filesystem::path& outputPath) {
    if (outputPath.empty()) {
        throw invalid_argument("Metrics output path must not be empty");
    }
    if (!outputPath.parent_path().empty()) {
        filesystem::create_directories(outputPath.parent_path());
    }
    ofstream output(outputPath);
    if (!output) {
        throw runtime_error("Unable to write metrics file '" + outputPath.string() + "'");
    }
    output << fixed << setprecision(6);
    return output;
}

}  // namespace

void MetricsCollector::recordFlowStatistics(const FlowStatistics& statistics) {
    validateFiniteNonNegative(statistics.totalDelaySeconds, "Flow delay");
    flowStatistics_.transmittedPackets += statistics.transmittedPackets;
    flowStatistics_.transmittedBytes += statistics.transmittedBytes;
    flowStatistics_.receivedPackets += statistics.receivedPackets;
    flowStatistics_.receivedBytes += statistics.receivedBytes;
    flowStatistics_.lostPackets += statistics.lostPackets;
    flowStatistics_.totalDelaySeconds += statistics.totalDelaySeconds;
}

void MetricsCollector::recordBatteryLevel(double batteryPercent) {
    if (!isfinite(batteryPercent) || batteryPercent < 0.0 || batteryPercent > 100.0) {
        throw invalid_argument("Battery level must be finite and between 0 and 100");
    }
    batteryTotal_ += batteryPercent;
    if (batterySampleCount_ == 0 || batteryPercent < minimumBatteryPercent_) {
        minimumBatteryPercent_ = batteryPercent;
    }
    ++batterySampleCount_;
}

void MetricsCollector::recordChargingEvent() {
    ++chargingEvents_;
}

void MetricsCollector::recordRobotRemovedFromRouting(double simulationTimeSeconds) {
    validateFiniteNonNegative(simulationTimeSeconds, "Robot removal time");
    ++robotsRemovedFromRouting_;
    if (firstRobotRemovalTimeSeconds_ < 0.0) {
        firstRobotRemovalTimeSeconds_ = simulationTimeSeconds;
    }
}

void MetricsCollector::recordRouteRecomputation() {
    ++routeRecomputations_;
}

SimulationMetrics MetricsCollector::summarize(double simulationDurationSeconds) const {
    validateFiniteNonNegative(simulationDurationSeconds, "Simulation duration");
    if (simulationDurationSeconds == 0.0) {
        throw invalid_argument("Simulation duration must be greater than zero");
    }

    SimulationMetrics metrics;
    metrics.transmittedPackets = flowStatistics_.transmittedPackets;
    metrics.receivedPackets = flowStatistics_.receivedPackets;
    metrics.lostPackets = flowStatistics_.lostPackets;
    if (flowStatistics_.receivedPackets > 0) {
        metrics.averageEndToEndDelayMilliseconds =
            (flowStatistics_.totalDelaySeconds / static_cast<double>(flowStatistics_.receivedPackets)) * 1000.0;
    }
    if (flowStatistics_.transmittedPackets > 0) {
        metrics.packetDeliveryRatioPercent =
            (static_cast<double>(flowStatistics_.receivedPackets) / static_cast<double>(flowStatistics_.transmittedPackets)) * 100.0;
        metrics.packetLossPercent =
            (static_cast<double>(flowStatistics_.lostPackets) / static_cast<double>(flowStatistics_.transmittedPackets)) * 100.0;
    }
    metrics.throughputMbps =
        (static_cast<double>(flowStatistics_.receivedBytes) * 8.0) / (simulationDurationSeconds * 1000000.0);
    if (batterySampleCount_ > 0) {
        metrics.averageBatteryRemainingPercent = batteryTotal_ / static_cast<double>(batterySampleCount_);
        metrics.minimumBatteryRemainingPercent = minimumBatteryPercent_;
    }
    metrics.chargingEvents = chargingEvents_;
    metrics.robotsRemovedFromRouting = robotsRemovedFromRouting_;
    metrics.networkLifetimeSeconds =
        firstRobotRemovalTimeSeconds_ < 0.0 ? simulationDurationSeconds : firstRobotRemovalTimeSeconds_;
    metrics.routeRecomputations = routeRecomputations_;
    return metrics;
}

void MetricsCollector::writeCsv(const filesystem::path& outputPath, double simulationDurationSeconds) const {
    const SimulationMetrics metrics = summarize(simulationDurationSeconds);
    ofstream output = openOutputFile(outputPath);
    output << "metric,value\n";
    output << "average_end_to_end_delay_ms," << metrics.averageEndToEndDelayMilliseconds << '\n';
    output << "throughput_mbps," << metrics.throughputMbps << '\n';
    output << "packet_delivery_ratio_percent," << metrics.packetDeliveryRatioPercent << '\n';
    output << "packet_loss_percent," << metrics.packetLossPercent << '\n';
    output << "transmitted_packets," << metrics.transmittedPackets << '\n';
    output << "received_packets," << metrics.receivedPackets << '\n';
    output << "lost_packets," << metrics.lostPackets << '\n';
    output << "average_battery_remaining_percent," << metrics.averageBatteryRemainingPercent << '\n';
    output << "minimum_battery_remaining_percent," << metrics.minimumBatteryRemainingPercent << '\n';
    output << "charging_events," << metrics.chargingEvents << '\n';
    output << "robots_removed_from_routing," << metrics.robotsRemovedFromRouting << '\n';
    output << "network_lifetime_seconds," << metrics.networkLifetimeSeconds << '\n';
    output << "route_recomputations," << metrics.routeRecomputations << '\n';
}

void MetricsCollector::writeSummary(const filesystem::path& outputPath, double simulationDurationSeconds) const {
    const SimulationMetrics metrics = summarize(simulationDurationSeconds);
    ofstream output = openOutputFile(outputPath);
    output << "Simulation Summary\n";
    output << "Average End-to-End Delay (ms): " << metrics.averageEndToEndDelayMilliseconds << '\n';
    output << "Throughput (Mbps): " << metrics.throughputMbps << '\n';
    output << "Packet Delivery Ratio (%): " << metrics.packetDeliveryRatioPercent << '\n';
    output << "Packet Loss (%): " << metrics.packetLossPercent << '\n';
    output << "Packets Sent: " << metrics.transmittedPackets << '\n';
    output << "Packets Delivered: " << metrics.receivedPackets << '\n';
    output << "Packets Lost: " << metrics.lostPackets << '\n';
    output << "Average Battery Remaining (%): " << metrics.averageBatteryRemainingPercent << '\n';
    output << "Minimum Battery Remaining (%): " << metrics.minimumBatteryRemainingPercent << '\n';
    output << "Charging Events: " << metrics.chargingEvents << '\n';
    output << "Robots Removed From Routing: " << metrics.robotsRemovedFromRouting << '\n';
    output << "Network Lifetime (s): " << metrics.networkLifetimeSeconds << '\n';
    output << "Route Recomputation Count: " << metrics.routeRecomputations << '\n';
}

}  // namespace ealr
