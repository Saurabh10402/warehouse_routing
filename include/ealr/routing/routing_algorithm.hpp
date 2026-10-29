#pragma once

#include "ealr/routing/routing_types.hpp"

#include <optional>

namespace ealr {

/**
 * Common contract for all route-selection algorithms.
 *
 * Implementations must return a source-to-destination sequence of robot IDs
 * when successful. They return `NoRoute` when valid endpoints cannot be
 * connected through the supplied neighbour graph. Invalid or unavailable
 * endpoints are handled consistently through `validateRequest`.
 */
class RoutingAlgorithm {
public:
    virtual ~RoutingAlgorithm() = default;

    virtual const char* name() const noexcept = 0;
    virtual RouteResult computeRoute(const RoutingRequest& request) const = 0;

protected:
    static std::optional<RouteResult> validateRequest(const RoutingRequest& request);
};

}  // namespace ealr
