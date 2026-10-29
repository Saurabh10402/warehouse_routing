#include "ealr/routing/ealr_routing.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Failure: " << message << '\n';
        ++failures;
    }
}

ealr::RoutingNode node(ealr::RobotId id,
                       ealr::Position2D position,
                       double batteryPercent,
                       std::uint32_t queueLength,
                       std::vector<ealr::RobotId> neighbours) {
    return {id, position, batteryPercent, queueLength, 50.0, ealr::RobotState::Idle, std::move(neighbours)};
}

ealr::RoutingTopology buildTopology() {
    return ealr::RoutingTopology(std::vector<ealr::RoutingNode>{
        node(1, {0.0, 0.0}, 100.0, 0, {2, 3}),
        node(2, {1.0, 0.0}, 5.0, 10, {1, 4}),
        node(3, {0.0, 3.0}, 100.0, 0, {1, 4}),
        node(4, {2.0, 0.0}, 100.0, 0, {2, 3})
    });
}

}  // namespace

int main() {
    const ealr::RoutingTopology topology = buildTopology();

    const ealr::EalrRouting energyAware({1.0, 10.0, 1.0});
    const ealr::RouteResult energyAwareRoute = energyAware.computeRoute({topology, 1, 4});
    expect(std::string(energyAware.name()) == "ealr", "EALR has a stable algorithm name");
    expect(energyAwareRoute.found(), "EALR finds a connected route");
    expect(energyAwareRoute.hops == std::vector<ealr::RobotId>({1, 3, 4}), "EALR avoids the low-battery, congested shorter route");
    expect(energyAwareRoute.totalCost > 6.0 && energyAwareRoute.totalCost < 7.0, "EALR reports composite routing cost");

    const ealr::EalrRouting distanceOnly({1.0, 0.0, 0.0});
    const ealr::RouteResult distanceOnlyRoute = distanceOnly.computeRoute({topology, 1, 4});
    expect(distanceOnlyRoute.hops == std::vector<ealr::RobotId>({1, 2, 4}), "zero penalty weights reduce EALR to distance-only routing");
    expect(std::abs(distanceOnlyRoute.totalCost - 2.0) < 0.00001, "distance-only EALR reports distance cost");

    const ealr::EalrRouting queueAware({0.0, 0.0, 2.0});
    const ealr::RouteResult queueAwareRoute = queueAware.computeRoute({topology, 1, 4});
    expect(queueAwareRoute.hops == std::vector<ealr::RobotId>({1, 3, 4}), "congestion weight avoids a queued relay");

    try {
        ealr::EalrRouting invalidWeights({1.0, -1.0, 1.0});
        expect(false, "negative EALR weights are rejected");
    } catch (const std::invalid_argument&) {
    }

    return failures == 0 ? 0 : 1;
}
