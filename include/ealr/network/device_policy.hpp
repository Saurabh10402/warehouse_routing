#pragma once

#include "ealr/config/warehouse_configuration.hpp"

#include <cstdint>

namespace ealr {

/** Vendor-neutral packet treatment derived from a device's configured priority. */
struct QualityOfServicePolicy {
    std::uint8_t dscp;
    bool strictPriorityQueue;
    bool allowPreemption;
    double maximumLatencyMs;
};

/** Maps device profiles to a concrete QoS policy without vendor-specific commands. */
QualityOfServicePolicy qualityOfServiceFor(
    TrafficPriority priority,
    const LatencyTargetConfiguration& latencyTargets);

}  // namespace ealr
