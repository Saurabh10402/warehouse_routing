#include "ealr/routing/dijkstra_routing.hpp"

#include <cmath>
#include <iostream>
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

ealr::Robot makeRobot(ealr::RobotId id, ealr::Position2D position) {
    return {id, position, 100.0, 50.0, {25.0, 10.0}};
}

ealr::RoutingTopology buildTopology(ealr::Robot& first,
                                    ealr::Robot& second,
                                    ealr::Robot& third,
                                    ealr::Robot& fourth) {
    return ealr::RoutingTopology(std::vector<ealr::RoutingNode>{
        ealr::RoutingNode::fromRobot(first),
        ealr::RoutingNode::fromRobot(second),
        ealr::RoutingNode::fromRobot(third),
        ealr::RoutingNode::fromRobot(fourth)
    });
}

}  // namespace

int main() {
    ealr::Robot first = makeRobot(1, {0.0, 0.0});
    ealr::Robot second = makeRobot(2, {1.0, 0.0});
    ealr::Robot third = makeRobot(3, {0.0, 3.0});
    ealr::Robot fourth = makeRobot(4, {2.0, 0.0});
    first.setNeighbours({2, 3});
    second.setNeighbours({1, 4});
    third.setNeighbours({1, 4});
    fourth.setNeighbours({2, 3});

    const ealr::DijkstraRouting routing;
    const ealr::RoutingTopology baseline = buildTopology(first, second, third, fourth);
    const ealr::RouteResult shortestRoute = routing.computeRoute({baseline, 1, 4});
    expect(std::string(routing.name()) == "dijkstra", "Dijkstra has a stable algorithm name");
    expect(shortestRoute.found(), "Dijkstra finds a connected route");
    expect(shortestRoute.hops == std::vector<ealr::RobotId>({1, 2, 4}), "Dijkstra selects the minimum-distance path");
    expect(std::abs(shortestRoute.totalCost - 2.0) < 0.00001, "Dijkstra route cost is geographic distance only");

    second.consumeBattery(95.0);
    const ealr::RoutingTopology removedRelay = buildTopology(first, second, third, fourth);
    const ealr::RouteResult alternateRoute = routing.computeRoute({removedRelay, 1, 4});
    expect(alternateRoute.found(), "Dijkstra can recompute after a relay leaves routing");
    expect(alternateRoute.hops == std::vector<ealr::RobotId>({1, 3, 4}), "Dijkstra does not select removed robots as relays");

    first.setNeighbours({});
    const ealr::RoutingTopology disconnected = buildTopology(first, second, third, fourth);
    const ealr::RouteResult noRoute = routing.computeRoute({disconnected, 1, 4});
    expect(noRoute.status == ealr::RouteStatus::NoRoute, "Dijkstra reports disconnected valid endpoints");

    const ealr::RouteResult localRoute = routing.computeRoute({baseline, 1, 1});
    expect(localRoute.found() && localRoute.hops == std::vector<ealr::RobotId>({1}), "source equals destination has a zero-hop route");
    expect(std::abs(localRoute.totalCost) < 0.00001, "local route costs zero");

    return failures == 0 ? 0 : 1;
}
