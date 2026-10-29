#include "ealr/safety/temperature_safety_controller.hpp"

#include <cmath>
#include <stdexcept>

namespace ealr {
namespace {

void validateZoneAndTemperature(const std::string& zoneId, double temperatureCelsius) {
    if (zoneId.empty()) {
        throw std::invalid_argument("A temperature safety event requires a non-empty zone identifier");
    }
    if (!std::isfinite(temperatureCelsius)) {
        throw std::invalid_argument("Temperature safety readings must be finite");
    }
}

TemperatureSafetyDecision decisionFor(
    TemperatureSafetyState state,
    const TemperatureSafetyConfiguration& configuration) {
    switch (state) {
        case TemperatureSafetyState::Normal:
            return {state, TrafficPriority::Background, false, false};
        case TemperatureSafetyState::Warning:
            return {state, TrafficPriority::Control, true, false};
        case TemperatureSafetyState::EmergencyStop:
            return {state, TrafficPriority::Safety, true, configuration.broadcastEmergencyStop};
    }
    return {TemperatureSafetyState::EmergencyStop, TrafficPriority::Safety, true, configuration.broadcastEmergencyStop};
}

}  // namespace

TemperatureSafetyController::TemperatureSafetyController(TemperatureSafetyConfiguration configuration)
    : configuration_(configuration) {
    if (!std::isfinite(configuration_.warningCelsius) || !std::isfinite(configuration_.stopCelsius) ||
        configuration_.warningCelsius >= configuration_.stopCelsius) {
        throw std::invalid_argument("Temperature thresholds must satisfy warningCelsius < stopCelsius");
    }
    if (!configuration_.processAtLocalEdge) {
        throw std::invalid_argument("Temperature safety requires local processing");
    }
}

TemperatureSafetyDecision TemperatureSafetyController::processReading(
    const std::string& zoneId,
    double temperatureCelsius) {
    validateZoneAndTemperature(zoneId, temperatureCelsius);
    TemperatureSafetyState& state = zoneStates_[zoneId];

    if (temperatureCelsius >= configuration_.stopCelsius) {
        state = TemperatureSafetyState::EmergencyStop;
    } else if (state == TemperatureSafetyState::EmergencyStop && configuration_.requireManualResetAfterStop) {
        // A safety stop remains active until an operator acknowledges it.
    } else if (temperatureCelsius >= configuration_.warningCelsius) {
        state = TemperatureSafetyState::Warning;
    } else {
        state = TemperatureSafetyState::Normal;
    }
    return decisionFor(state, configuration_);
}

bool TemperatureSafetyController::acknowledgeEmergencyStop(
    const std::string& zoneId,
    double currentTemperatureCelsius) {
    validateZoneAndTemperature(zoneId, currentTemperatureCelsius);
    const auto state = zoneStates_.find(zoneId);
    if (state == zoneStates_.end() || state->second != TemperatureSafetyState::EmergencyStop ||
        currentTemperatureCelsius >= configuration_.warningCelsius) {
        return false;
    }
    state->second = TemperatureSafetyState::Normal;
    return true;
}

TemperatureSafetyState TemperatureSafetyController::stateFor(const std::string& zoneId) const {
    const auto state = zoneStates_.find(zoneId);
    return state == zoneStates_.end() ? TemperatureSafetyState::Normal : state->second;
}

const char* toString(TemperatureSafetyState state) {
    switch (state) {
        case TemperatureSafetyState::Normal:
            return "normal";
        case TemperatureSafetyState::Warning:
            return "warning";
        case TemperatureSafetyState::EmergencyStop:
            return "emergencyStop";
    }
    return "unknown";
}

}  // namespace ealr
