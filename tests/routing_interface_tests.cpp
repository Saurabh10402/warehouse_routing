#include "ealr/routing/routing_algorithm.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Failure: " << message << '\n';
        ++failures;
    }
}

class DirectLinkAlgorithm final : public ealr::RoutingAlgorithm {
public:
    const char* name() const noexcept override {
        return "direct-link-test";
    }

    ealr::RouteResult computeRoute(const ealr::RoutingRequest& request) const override {
        if (const auto invalidRoute = validateRequest(request)) {
            return *invalidRoute;
        }

        const ealr::RoutingNode* source = request.topology.findNode(request.source);
        for (const ealr::RobotId neighbour : source->neighbours) {
            if (neighbour == request.destination) {
                return {ealr::RouteStatus::Found, {request.source, request.destination}, 1.0};
            }
        }
        return {ealr::RouteStatus::NoRoute, {}, 0.0};
    }
};

}  // namespace

int main() {
    ealr::Robot source(1, {0.0, 0.0}, 100.0, 20.0, {25.0, 10.0});
    ealr::Robot destination(2, {10.0, 0.0}, 100.0, 20.0, {25.0, 10.0});
    source.setNeighbours({2});
    destination.setNeighbours({1});

    const ealr::RoutingTopology topology({
        ealr::RoutingNode::fromRobot(source),
        ealr::RoutingNode::fromRobot(destination)
    });
    const DirectLinkAlgorithm algorithm;
    const ealr::RouteResult route = algorithm.computeRoute({topology, 1, 2});
    expect(std::string(algorithm.name()) == "direct-link-test", "algorithm identity is polymorphic");
    expect(route.found(), "derived algorithm can return a route through the common interface");
    expect(route.hops == std::vector<ealr::RobotId>({1, 2}), "route contains ordered source and destination hops");

    const ealr::RouteResult unknownSource = algorithm.computeRoute({topology, 99, 2});
    expect(unknownSource.status == ealr::RouteStatus::InvalidSource, "unknown source has a dedicated status");

    destination.consumeBattery(95.0);
    const ealr::RoutingTopology unavailableDestination({
        ealr::RoutingNode::fromRobot(source),
        ealr::RoutingNode::fromRobot(destination)
    });
    const ealr::RouteResult unavailableRoute = algorithm.computeRoute({unavailableDestination, 1, 2});
    expect(unavailableRoute.status == ealr::RouteStatus::DestinationUnavailable, "returning robot cannot be a destination");

    try {
        ealr::RoutingTopology invalidTopology({{{1, {0.0, 0.0}, 50.0, 0, 10.0, ealr::RobotState::Idle, {9}}}});
        expect(false, "unknown topology neighbours are rejected");
    } catch (const std::invalid_argument&) {
    }

    try {
        ealr::EalrCostWeights{-1.0, 1.0, 1.0}.validate();
        expect(false, "negative EALR cost weights are rejected");
    } catch (const std::invalid_argument&) {
    }

    return failures == 0 ? 0 : 1;
}
