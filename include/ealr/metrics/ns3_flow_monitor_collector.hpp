#pragma once

#include "ealr/metrics/metrics_collector.hpp"

#include <ns3/flow-monitor.h>
#include <ns3/ptr.h>

#include <cstdint>

namespace ealr {

struct FlowMonitorCollectionResult {
    std::uint32_t monitoredFlowCount = 0;
    FlowStatistics statistics;
};

/** Converts ns-3 FlowMonitor counters into framework-independent metrics. */
class Ns3FlowMonitorCollector {
public:
    static FlowMonitorCollectionResult collect(const ns3::Ptr<ns3::FlowMonitor>& flowMonitor,
                                               MetricsCollector& metricsCollector);
};

}  // namespace ealr
