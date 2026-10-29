#pragma once

#include "ealr/routing/routing_algorithm.hpp"

namespace ealr {

/** Baseline shortest-path routing with distance as its only edge cost. */
class DijkstraRouting final : public RoutingAlgorithm {
public:
    const char* name() const noexcept override;
    RouteResult computeRoute(const RoutingRequest& request) const override;
};

}  // namespace ealr
