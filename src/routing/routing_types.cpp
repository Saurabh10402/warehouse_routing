#include "ealr/routing/routing_algorithm.hpp"

#include <cmath>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace ealr {
namespace {

void validateFinite(double value, const char* property) {
    if (!isfinite(value)) {
        throw invalid_argument(string(property) + " must be finite");
    }
}

void validateNode(const RoutingNode& node, const unordered_set<RobotId>& nodeIds) {
    validateFinite(node.position.x, "Routing node position x");
    validateFinite(node.position.y, "Routing node position y");
    validateFinite(node.batteryPercent, "Routing node battery percentage");
    validateFinite(node.communicationRadiusMeters, "Routing node communication radius");
    if (node.batteryPercent < 0.0 || node.batteryPercent > 100.0) {
        throw invalid_argument("Routing node battery percentage must be between 0 and 100");
    }
    if (node.communicationRadiusMeters <= 0.0) {
        throw invalid_argument("Routing node communication radius must be greater than zero");
    }

    unordered_set<RobotId> neighbourIds;
    for (const RobotId neighbour : node.neighbours) {
        if (neighbour == node.id) {
            throw invalid_argument("Routing topology cannot contain self-neighbour links");
        }
        if (nodeIds.find(neighbour) == nodeIds.end()) {
            throw invalid_argument("Routing topology neighbour does not identify a known node");
        }
        if (!neighbourIds.insert(neighbour).second) {
            throw invalid_argument("Routing topology cannot contain duplicate neighbour links");
        }
    }
}

}  // namespace

RoutingNode RoutingNode::fromRobot(const Robot& robot) {
    return {
        robot.id(),
        robot.position(),
        robot.batteryPercent(),
        robot.queueLength(),
        robot.communicationRadiusMeters(),
        robot.state(),
        robot.neighbours()
    };
}

bool RoutingNode::isAvailableForRouting() const {
    return state != RobotState::ReturningToCharge;
}

RoutingTopology::RoutingTopology(vector<RoutingNode> nodes) : nodes_(std::move(nodes)) {
    unordered_set<RobotId> nodeIds;
    for (const RoutingNode& node : nodes_) {
        if (!nodeIds.insert(node.id).second) {
            throw invalid_argument("Routing topology cannot contain duplicate robot IDs");
        }
    }
    for (const RoutingNode& node : nodes_) {
        validateNode(node, nodeIds);
    }
}

const vector<RoutingNode>& RoutingTopology::nodes() const {
    return nodes_;
}

const RoutingNode* RoutingTopology::findNode(RobotId id) const {
    for (const RoutingNode& node : nodes_) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

void EalrCostWeights::validate() const {
    validateFinite(alpha, "EALR alpha");
    validateFinite(beta, "EALR beta");
    validateFinite(gamma, "EALR gamma");
    if (alpha < 0.0 || beta < 0.0 || gamma < 0.0) {
        throw invalid_argument("EALR alpha, beta, and gamma must not be negative");
    }
}

bool RouteResult::found() const {
    return status == RouteStatus::Found;
}

optional<RouteResult> RoutingAlgorithm::validateRequest(const RoutingRequest& request) {
    const RoutingNode* source = request.topology.findNode(request.source);
    if (source == nullptr) {
        return RouteResult{RouteStatus::InvalidSource, {}, 0.0};
    }

    const RoutingNode* destination = request.topology.findNode(request.destination);
    if (destination == nullptr) {
        return RouteResult{RouteStatus::InvalidDestination, {}, 0.0};
    }

    if (!source->isAvailableForRouting()) {
        return RouteResult{RouteStatus::SourceUnavailable, {}, 0.0};
    }
    if (!destination->isAvailableForRouting()) {
        return RouteResult{RouteStatus::DestinationUnavailable, {}, 0.0};
    }

    return nullopt;
}

const char* toString(RouteStatus status) {
    switch (status) {
        case RouteStatus::Found:
            return "Found";
        case RouteStatus::InvalidSource:
            return "InvalidSource";
        case RouteStatus::InvalidDestination:
            return "InvalidDestination";
        case RouteStatus::SourceUnavailable:
            return "SourceUnavailable";
        case RouteStatus::DestinationUnavailable:
            return "DestinationUnavailable";
        case RouteStatus::NoRoute:
            return "NoRoute";
    }
    return "Unknown";
}

}  // namespace ealr
