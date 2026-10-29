#include "ealr/config/configuration.hpp"
#include "ealr/routing/ealr_routing.hpp"
#include "ealr/safety/temperature_safety_controller.hpp"
#include <limits>
#include <iostream>

int main() {
    int failures = 0;
    for (double value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        try { ealr::TemperatureSafetyController c({value, 80, true, true, true}); ++failures; }
        catch (const std::invalid_argument&) {}
        try { ealr::TemperatureSafetyController c({70, value, true, true, true}); ++failures; }
        catch (const std::invalid_argument&) {}
        try { ealr::EalrRouting r({1, value, 1}); ++failures; }
        catch (const std::invalid_argument&) {}
    }
    try { ealr::TemperatureSafetyController c({80, 80, true, true, true}); ++failures; }
    catch (const std::invalid_argument&) {}
    try { ealr::TemperatureSafetyController c({70, 80, true, false, true}); ++failures; }
    catch (const std::invalid_argument&) {}
    ealr::TemperatureSafetyController c({70, 80, true, true, true});
    if(c.processReading("a",79.999).state != ealr::TemperatureSafetyState::Warning) ++failures;
    c.processReading("a",80);
    if(c.acknowledgeEmergencyStop("a",70)) ++failures;
    if(!c.acknowledgeEmergencyStop("a",69.999)) ++failures;
    if (ealr::Position2D{1e200,0}.distanceTo({0,0}) != 1e200) ++failures;
    std::cerr << "regression failures: " << failures << '\n';
    return failures ? 1 : 0;
}
