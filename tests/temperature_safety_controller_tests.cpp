#include "ealr/safety/temperature_safety_controller.hpp"

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
    const ealr::TemperatureSafetyConfiguration configuration{70.0, 80.0, true, true, true};
    ealr::TemperatureSafetyController controller(configuration);

    const ealr::TemperatureSafetyDecision normal = controller.processReading("zone-4", 45.0);
    expect(normal.state == ealr::TemperatureSafetyState::Normal, "normal reading remains normal");
    expect(normal.priority == ealr::TrafficPriority::Background, "normal reading uses background priority");

    const ealr::TemperatureSafetyDecision warning = controller.processReading("zone-4", 70.0);
    expect(warning.state == ealr::TemperatureSafetyState::Warning, "70C creates warning state");
    expect(warning.priority == ealr::TrafficPriority::Control, "warning is expedited");
    expect(warning.notifyLocalSafetyController, "warning reaches local edge controller");
    expect(!warning.broadcastEmergencyStop, "warning does not stop the warehouse");

    const ealr::TemperatureSafetyDecision emergency = controller.processReading("zone-4", 80.0);
    expect(emergency.state == ealr::TemperatureSafetyState::EmergencyStop, "80C creates emergency stop");
    expect(emergency.priority == ealr::TrafficPriority::Safety, "emergency uses safety priority");
    expect(emergency.broadcastEmergencyStop, "emergency broadcasts stop event");

    const ealr::TemperatureSafetyDecision latched = controller.processReading("zone-4", 60.0);
    expect(latched.state == ealr::TemperatureSafetyState::EmergencyStop, "emergency remains latched after temperature drops");
    expect(!controller.acknowledgeEmergencyStop("zone-4", 75.0), "cannot acknowledge an unsafe temperature");
    expect(controller.acknowledgeEmergencyStop("zone-4", 60.0), "operator can acknowledge below warning threshold");
    expect(controller.stateFor("zone-4") == ealr::TemperatureSafetyState::Normal, "acknowledgement resets zone state");

    return failures == 0 ? 0 : 1;
}
