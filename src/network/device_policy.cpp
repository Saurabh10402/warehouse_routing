#include "ealr/network/device_policy.hpp"

namespace ealr {

QualityOfServicePolicy qualityOfServiceFor(
    TrafficPriority priority,
    const LatencyTargetConfiguration& latencyTargets) {
    switch (priority) {
        case TrafficPriority::Safety:
            return {48, true, true, latencyTargets.safetyMaximumLatencyMs};  // CS6
        case TrafficPriority::Control:
            return {46, true, false, latencyTargets.controlMaximumLatencyMs};  // EF
        case TrafficPriority::Normal:
            return {18, false, false, latencyTargets.normalMaximumLatencyMs};  // AF21
        case TrafficPriority::Background:
            return {8, false, false, latencyTargets.backgroundMaximumLatencyMs};  // CS1
    }
    return {0, false, false, latencyTargets.backgroundMaximumLatencyMs};
}

}  // namespace ealr
