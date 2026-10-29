#include "ealr/routing/dijkstra_routing.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_map>

using namespace std;

namespace ealr {

const char* DijkstraRouting::name() const noexcept {
    return "dijkstra";
}

RouteResult DijkstraRouting::computeRoute(const RoutingRequest& request) const {
    if (const auto invalidRoute = validateRequest(request)) {
        return *invalidRoute;
    }
    if (request.source == request.destination) {
        return {RouteStatus::Found, {request.source}, 0.0};
    }

    using QueueEntry = pair<double, RobotId>;
    priority_queue<QueueEntry, vector<QueueEntry>, greater<QueueEntry>> frontier;
    unordered_map<RobotId, double> distances;
    unordered_map<RobotId, RobotId> predecessors;

    distances[request.source] = 0.0;
    frontier.push({0.0, request.source});

    while (!frontier.empty()) {
        const auto [currentDistance, currentId] = frontier.top();
        frontier.pop();

        if (currentDistance > distances[currentId]) {
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

            const double edgeDistance = currentNode->position.distanceTo(neighbour->position);
            const double candidateDistance = currentDistance + edgeDistance;
            const auto knownDistance = distances.find(neighbourId);
            if (knownDistance == distances.end() || candidateDistance < knownDistance->second) {
                distances[neighbourId] = candidateDistance;
                predecessors[neighbourId] = currentId;
                frontier.push({candidateDistance, neighbourId});
            }
        }
    }

    const auto destinationDistance = distances.find(request.destination);
    if (destinationDistance == distances.end()) {
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

    return {RouteStatus::Found, std::move(reversedHops), destinationDistance->second};
}

}  // namespace ealr
