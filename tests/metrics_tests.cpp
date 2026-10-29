#include "ealr/metrics/metrics_collector.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Failure: " << message << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    ealr::MetricsCollector collector;
    collector.recordFlowStatistics({4, 400, 3, 300, 1, 0.3});
    collector.recordBatteryLevel(80.0);
    collector.recordBatteryLevel(60.0);
    collector.recordChargingEvent();
    collector.recordRobotRemovedFromRouting(5.0);
    collector.recordRouteRecomputation();
    collector.recordRouteRecomputation();

    const ealr::SimulationMetrics metrics = collector.summarize(10.0);
    expect(std::abs(metrics.averageEndToEndDelayMilliseconds - 100.0) < 0.00001, "average delay is calculated from delivered packets");
    expect(std::abs(metrics.throughputMbps - 0.00024) < 0.0000001, "throughput is calculated from received bytes and duration");
    expect(std::abs(metrics.packetDeliveryRatioPercent - 75.0) < 0.00001, "packet delivery ratio is calculated");
    expect(std::abs(metrics.packetLossPercent - 25.0) < 0.00001, "packet loss percentage is calculated");
    expect(metrics.transmittedPackets == 4 && metrics.receivedPackets == 3 && metrics.lostPackets == 1, "packet counters are retained");
    expect(std::abs(metrics.averageBatteryRemainingPercent - 70.0) < 0.00001, "average battery level is calculated");
    expect(std::abs(metrics.minimumBatteryRemainingPercent - 60.0) < 0.00001, "minimum battery level is calculated");
    expect(metrics.chargingEvents == 1 && metrics.robotsRemovedFromRouting == 1, "battery events are counted");
    expect(std::abs(metrics.networkLifetimeSeconds - 5.0) < 0.00001, "first robot removal defines network lifetime");
    expect(metrics.routeRecomputations == 2, "route recomputations are counted");

    const std::filesystem::path outputDirectory = std::filesystem::temp_directory_path() / "ealr-metrics-test";
    const std::filesystem::path csvPath = outputDirectory / "metrics.csv";
    const std::filesystem::path summaryPath = outputDirectory / "summary.txt";
    collector.writeCsv(csvPath, 10.0);
    collector.writeSummary(summaryPath, 10.0);
    std::ifstream csv(csvPath);
    std::ifstream summary(summaryPath);
    std::string csvContents((std::istreambuf_iterator<char>(csv)), std::istreambuf_iterator<char>());
    std::string summaryContents((std::istreambuf_iterator<char>(summary)), std::istreambuf_iterator<char>());
    expect(csvContents.find("packet_delivery_ratio_percent,75.000000") != std::string::npos, "CSV export contains computed PDR");
    expect(summaryContents.find("Route Recomputation Count: 2") != std::string::npos, "summary export contains route recomputations");

    return failures == 0 ? 0 : 1;
}
