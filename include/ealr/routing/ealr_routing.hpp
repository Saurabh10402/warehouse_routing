#pragma once

#include "ealr/routing/routing_algorithm.hpp"

namespace ealr {

/**
 * Energy-Aware Low-Latency Routing.
 *
 * Each forwarding edge is weighted as:
 * alpha * distance + beta * batteryPenalty + gamma * congestionPenalty.
 * Battery penalty is normalized to [0, 1] from the candidate forwarding
 * robot's remaining percentage. Congestion penalty is its queue length.
 */
class EalrRouting final : public RoutingAlgorithm {
public:
    explicit EalrRouting(EalrCostWeights weights);

    const char* name() const noexcept override;
    RouteResult computeRoute(const RoutingRequest& request) const override;
    const EalrCostWeights& weights() const;

private:
    double edgeCost(const RoutingNode& current, const RoutingNode& neighbour) const;

    EalrCostWeights weights_;
};

}  // namespace ealr
