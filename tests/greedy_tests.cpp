#include "ealr/routing/greedy_routing.hpp"

#include <cmath>
#include <iostream>
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

ealr::RoutingNode node(ealr::RobotId id, ealr::Position2D position, std::vector<ealr::RobotId> neighbours) {
    return {id, position, 100.0, 0, 50.0, ealr::RobotState::Idle, std::move(neighbours)};
}

}  // namespace

int main() {
    const ealr::RoutingTopology topology(std::vector<ealr::RoutingNode>{
        node(1, {0.0, 0.0}, {2, 3}),
        node(2, {9.0, 0.0}, {1, 4}),
        node(3, {0.0, 1.0}, {1, 4}),
        node(4, {0.0, 2.0}, {2, 3, 5}),
        node(5, {10.0, 0.0}, {4})
    });

    const ealr::GreedyRouting routing;
    const ealr::RouteResult route = routing.computeRoute({topology, 1, 5});
    expect(std::string(routing.name()) == "greedy", "Greedy has a stable algorithm name");
    expect(route.found(), "Greedy finds an available route");
    expect(route.hops == std::vector<ealr::RobotId>({1, 2, 4, 5}), "Greedy chooses the neighbour closest to the destination at each hop");
    expect(route.totalCost > 20.0, "Greedy retains the resulting path distance even when locally suboptimal");

    const ealr::RoutingTopology deadEndTopology(std::vector<ealr::RoutingNode>{
        node(1, {0.0, 0.0}, {2, 3}),
        node(2, {9.0, 0.0}, {1}),
        node(3, {0.0, 3.0}, {1, 4}),
        node(4, {10.0, 0.0}, {3})
    });
    const ealr::RouteResult deadEndRoute = routing.computeRoute({deadEndTopology, 1, 4});
    expect(deadEndRoute.status == ealr::RouteStatus::NoRoute, "Greedy does not backtrack after a locally preferred dead end");

    ealr::RoutingNode unavailableNeighbour = node(2, {9.0, 0.0}, {4});
    unavailableNeighbour.state = ealr::RobotState::ReturningToCharge;
    const ealr::RoutingTopology unavailableRelayTopology(std::vector<ealr::RoutingNode>{
        node(1, {0.0, 0.0}, {2, 3}),
        unavailableNeighbour,
        node(3, {1.0, 0.0}, {4}),
        node(4, {10.0, 0.0}, {})
    });
    const ealr::RouteResult unavailableRelayRoute = routing.computeRoute({unavailableRelayTopology, 1, 4});
    expect(unavailableRelayRoute.found() && unavailableRelayRoute.hops == std::vector<ealr::RobotId>({1, 3, 4}), "Greedy skips unavailable neighbours even when they are geographically closer");

    const ealr::RoutingTopology tieTopology(std::vector<ealr::RoutingNode>{
        node(1, {0.0, 0.0}, {3, 2}),
        node(2, {1.0, 1.0}, {4}),
        node(3, {1.0, -1.0}, {4}),
        node(4, {2.0, 0.0}, {})
    });
    const ealr::RouteResult tieRoute = routing.computeRoute({tieTopology, 1, 4});
    expect(tieRoute.found() && tieRoute.hops == std::vector<ealr::RobotId>({1, 2, 4}), "equal-distance choices use the lower robot ID deterministically");
    expect(std::abs(tieRoute.totalCost - (2.0 * std::sqrt(2.0))) < 0.00001, "Greedy reports traversed distance");

    return failures == 0 ? 0 : 1;
}
