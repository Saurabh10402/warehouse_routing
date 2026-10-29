#pragma once

#include "ealr/robot/robot.hpp"

#include <string>
#include <vector>

namespace ealr {

/** Read-only routing data captured from a robot at a topology update. */
struct RoutingNode {
    RobotId id;
    Position2D position;
    double batteryPercent;
    std::uint32_t queueLength;
    double communicationRadiusMeters;
    RobotState state;
    std::vector<RobotId> neighbours;

    static RoutingNode fromRobot(const Robot& robot);

    bool isAvailableForRouting() const;
};

/**
 * Immutable graph supplied to an algorithm after neighbour discovery.
 *
 * A neighbour relationship describes a permitted forwarding hop. The topology
 * deliberately does not derive links from positions: future obstacle-aware or
 * radio-aware modules may provide a more accurate adjacency view.
 */
class RoutingTopology {
public:
    explicit RoutingTopology(std::vector<RoutingNode> nodes);

    const std::vector<RoutingNode>& nodes() const;
    const RoutingNode* findNode(RobotId id) const;

private:
    std::vector<RoutingNode> nodes_;
};

struct EalrCostWeights {
    double alpha;
    double beta;
    double gamma;

    void validate() const;
};

enum class RouteStatus {
    Found,
    InvalidSource,
    InvalidDestination,
    SourceUnavailable,
    DestinationUnavailable,
    NoRoute
};

struct RouteResult {
    RouteStatus status = RouteStatus::NoRoute;
    std::vector<RobotId> hops;
    double totalCost = 0.0;

    bool found() const;
};

struct RoutingRequest {
    const RoutingTopology& topology;
    RobotId source;
    RobotId destination;
};

const char* toString(RouteStatus status);

}  // namespace ealr
