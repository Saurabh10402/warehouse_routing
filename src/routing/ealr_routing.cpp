#include "ealr/routing/ealr_routing.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_map>
#include <utility>

using namespace std;

namespace ealr {

EalrRouting::EalrRouting(EalrCostWeights weights) : weights_(weights) {
    weights_.validate();
}

const char* EalrRouting::name() const noexcept {
    return "ealr";
}

RouteResult EalrRouting::computeRoute(const RoutingRequest& request) const {
    if (const auto invalidRoute = validateRequest(request)) {
        return *invalidRoute;
    }
    if (request.source == request.destination) {
        return {RouteStatus::Found, {request.source}, 0.0};
    }

    using QueueEntry = pair<double, RobotId>;
    priority_queue<QueueEntry, vector<QueueEntry>, greater<QueueEntry>> frontier;
    unordered_map<RobotId, double> costs;
    unordered_map<RobotId, RobotId> predecessors;

    costs[request.source] = 0.0;
    frontier.push({0.0, request.source});

    while (!frontier.empty()) {
        const auto [currentCost, currentId] = frontier.top();
        frontier.pop();

        if (currentCost > costs[currentId]) {
            continue;
        }
        if (currentId == request.destination) {
            break;
        }

        const RoutingNode* currentNode = request.topology.findNode(currentId);
        for (const RobotId neighbourId : currentNode->neighbours) {
            const RoutingNode* neighbour = request.topology.findNode(neighbourId);
            if (!neighbour->isAvailableForRouting()) {
                continue;
            }

            const double cost = checkedRoutingCost(request.linkCost ? request.linkCost(*currentNode,*neighbour) : edgeCost(*currentNode, *neighbour));
            const double candidateCost = checkedRoutingCost(currentCost + cost);
            const auto knownCost = costs.find(neighbourId);
            if (knownCost == costs.end() || candidateCost < knownCost->second) {
                costs[neighbourId] = candidateCost;
                predecessors[neighbourId] = currentId;
                frontier.push({candidateCost, neighbourId});
            }
        }
    }

    const auto destinationCost = costs.find(request.destination);
    if (destinationCost == costs.end()) {
        return {RouteStatus::NoRoute, {}, 0.0};
    }

    vector<RobotId> reversedHops;
    for (RobotId currentId = request.destination;; currentId = predecessors.at(currentId)) {
        reversedHops.push_back(currentId);
        if (currentId == request.source) {
            break;
        }
    }
    reverse(reversedHops.begin(), reversedHops.end());

    return {RouteStatus::Found, std::move(reversedHops), destinationCost->second};
}

const EalrCostWeights& EalrRouting::weights() const {
    return weights_;
}

double EalrRouting::edgeCost(const RoutingNode& current, const RoutingNode& neighbour) const {
    const double distance = current.position.distanceTo(neighbour.position);
    const double batteryPenalty = neighbour.energyConstrained ? (100.0 - neighbour.batteryPercent) / 100.0 : 0.0;
    const double congestionPenalty = static_cast<double>(neighbour.queueLength);
    return (weights_.alpha * distance) + (weights_.beta * batteryPenalty) + (weights_.gamma * congestionPenalty);
}

}  // namespace ealr
