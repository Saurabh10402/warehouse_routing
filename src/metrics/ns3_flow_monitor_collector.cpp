#include "ealr/metrics/ns3_flow_monitor_collector.hpp"

#include <stdexcept>

using namespace std;

namespace ealr {

FlowMonitorCollectionResult Ns3FlowMonitorCollector::collect(const ns3::Ptr<ns3::FlowMonitor>& flowMonitor,
                                                              MetricsCollector& metricsCollector) {
    if (flowMonitor == nullptr) {
        throw invalid_argument("FlowMonitor must not be null");
    }

    flowMonitor->CheckForLostPackets();
    FlowMonitorCollectionResult result;
    for (const auto& [flowId, flow] : flowMonitor->GetFlowStats()) {
        static_cast<void>(flowId);
        ++result.monitoredFlowCount;
        result.statistics.transmittedPackets += flow.txPackets;
        result.statistics.transmittedBytes += flow.txBytes;
        result.statistics.receivedPackets += flow.rxPackets;
        result.statistics.receivedBytes += flow.rxBytes;
        result.statistics.lostPackets += flow.lostPackets;
        result.statistics.totalDelaySeconds += flow.delaySum.GetSeconds();
    }
    metricsCollector.recordFlowStatistics(result.statistics);
    return result;
}

}  // namespace ealr
