#include "ealr/routing/greedy_routing.hpp"

#include <unordered_set>
#include <utility>

using namespace std;

namespace ealr {

const char* GreedyRouting::name() const noexcept {
    return "greedy";
}

RouteResult GreedyRouting::computeRoute(const RoutingRequest& request) const {
    if (const auto invalidRoute = validateRequest(request)) {
        return *invalidRoute;
    }
    if (request.source == request.destination) {
        return {RouteStatus::Found, {request.source}, 0.0};
    }

    const RoutingNode* destination = request.topology.findNode(request.destination);
    const RoutingNode* current = request.topology.findNode(request.source);
    unordered_set<RobotId> visited{request.source};
    vector<RobotId> hops{request.source};
    double totalDistance = 0.0;

    while (current->id != request.destination) {
        const RoutingNode* next = nullptr;
        double nextDestinationDistance = 0.0;

        for (const RobotId neighbourId : current->neighbours) {
            const RoutingNode* neighbour = request.topology.findNode(neighbourId);
            if (!neighbour->isAvailableForRouting() || visited.find(neighbourId) != visited.end()) {
                continue;
            }

            const double destinationDistance = neighbour->position.distanceTo(destination->position);
            if (next == nullptr || destinationDistance < nextDestinationDistance ||
                (destinationDistance == nextDestinationDistance && neighbour->id < next->id)) {
                next = neighbour;
                nextDestinationDistance = destinationDistance;
            }
        }

        if (next == nullptr) {
            return {RouteStatus::NoRoute, {}, 0.0};
        }

        const double cost = checkedRoutingCost(request.linkCost ? request.linkCost(*current,*next) : current->position.distanceTo(next->position));
        totalDistance = checkedRoutingCost(totalDistance + cost);
        current = next;
        visited.insert(current->id);
        hops.push_back(current->id);
    }

    return {RouteStatus::Found, std::move(hops), totalDistance};
}

}  // namespace ealr
