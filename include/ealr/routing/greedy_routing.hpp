#pragma once

#include "ealr/routing/routing_algorithm.hpp"

namespace ealr {

/**
 * Selects the neighbour geographically closest to the destination at each hop.
 *
 * Greedy routing intentionally does not backtrack: if its local choice reaches
 * a dead end, it returns NoRoute even when another first hop may have worked.
 */
class GreedyRouting final : public RoutingAlgorithm {
public:
    const char* name() const noexcept override;
    RouteResult computeRoute(const RoutingRequest& request) const override;
};

}  // namespace ealr
