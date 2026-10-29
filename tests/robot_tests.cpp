#include "ealr/robot/robot.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Failure: " << message << '\n';
        ++failures;
    }
}

template <typename Function>
void expectInvalidArgument(Function function, const std::string& message) {
    try {
        function();
        expect(false, message);
    } catch (const std::invalid_argument&) {
    }
}

}  // namespace

int main() {
    ealr::Robot robot(7, {3.0, 4.0}, 100.0, 25.0, {25.0, 10.0});
    expect(robot.id() == 7, "robot ID is preserved");
    expect(robot.state() == ealr::RobotState::Idle, "new robot begins idle");
    expect(robot.isAvailableForRouting(), "normal robot is available for routing");
    expect(std::abs(robot.position().distanceTo({0.0, 0.0}) - 5.0) < 0.00001, "distance uses two-dimensional geometry");

    robot.setWorking(true);
    expect(robot.state() == ealr::RobotState::Working, "working state is represented");
    expect(robot.consumeBattery(75.0) == ealr::BatteryTransition::LowBatteryWarning, "low battery threshold emits one warning");
    expect(robot.state() == ealr::RobotState::LowBattery, "low battery robot remains a distinct state");
    expect(robot.isAvailableForRouting(), "low battery robot remains routable");
    expect(robot.consumeBattery(0.1) == ealr::BatteryTransition::None, "remaining low battery does not repeat warnings");
    expect(robot.consumeBattery(15.0) == ealr::BatteryTransition::ReturningToCharge, "critical threshold starts return-to-charge");
    expect(robot.state() == ealr::RobotState::ReturningToCharge, "critical robot enters terminal state");
    expect(!robot.isAvailableForRouting(), "critical robot is removed from routing");
    expect(robot.consumeBattery(1.0) == ealr::BatteryTransition::None, "returning state does not repeat transition events");

    ealr::Robot topologyRobot(8, {0.0, 0.0}, 80.0, 10.0, {25.0, 10.0});
    topologyRobot.setNeighbours({2, 4, 6});
    expect(topologyRobot.neighbours().size() == 3, "neighbour view is retained");
    expectInvalidArgument([&topologyRobot] { topologyRobot.setNeighbours({2, 2}); }, "duplicate neighbours are rejected");
    expectInvalidArgument([&topologyRobot] { topologyRobot.setNeighbours({8}); }, "self neighbours are rejected");

    expect(topologyRobot.queueLength() == 0, "queue begins empty");
    expect(!topologyRobot.dequeuePacket(), "dequeue reports an empty queue");
    topologyRobot.enqueuePacket();
    topologyRobot.enqueuePacket();
    expect(topologyRobot.queueLength() == 2, "enqueue updates queue length");
    expect(topologyRobot.dequeuePacket() && topologyRobot.queueLength() == 1, "dequeue updates queue length");

    expectInvalidArgument(
        [] { ealr::Robot(1, {0.0, 0.0}, 80.0, 10.0, {10.0, 20.0}); },
        "invalid battery thresholds are rejected");
    expectInvalidArgument([&topologyRobot] { topologyRobot.consumeBattery(-1.0); }, "negative battery use is rejected");

    return failures == 0 ? 0 : 1;
}
