#pragma once

#include "ealr/config/warehouse_configuration.hpp"

#include <string>
#include <unordered_map>

namespace ealr {

enum class TemperatureSafetyState {
    Normal,
    Warning,
    EmergencyStop
};

struct TemperatureSafetyDecision {
    TemperatureSafetyState state;
    TrafficPriority priority;
    bool notifyLocalSafetyController;
    bool broadcastEmergencyStop;
};

/**
 * Maintains independent temperature-safety state per warehouse zone.
 * Emergency stops are deliberately latched until acknowledged when the plan
 * requires a manual reset.
 */
class TemperatureSafetyController {
public:
    explicit TemperatureSafetyController(TemperatureSafetyConfiguration configuration);

    TemperatureSafetyDecision processReading(const std::string& zoneId, double temperatureCelsius);
    bool acknowledgeEmergencyStop(const std::string& zoneId, double currentTemperatureCelsius);
    TemperatureSafetyState stateFor(const std::string& zoneId) const;

private:
    TemperatureSafetyConfiguration configuration_;
    std::unordered_map<std::string, TemperatureSafetyState> zoneStates_;
};

const char* toString(TemperatureSafetyState state);

}  // namespace ealr
