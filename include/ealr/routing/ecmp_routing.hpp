#pragma once
#include "ealr/routing/routing_algorithm.hpp"
#include <cstdint>

namespace ealr {
// Per-flow equal-cost next-hop selection. Costs must be strictly positive.
// This is a centralized model of ECMP, not a distributed routing protocol.
class EcmpRouting final : public RoutingAlgorithm {
public:
    explicit EcmpRouting(std::uint64_t flowHash) : flowHash_(flowHash) {}
    const char* name() const noexcept override { return "ecmp"; }
    RouteResult computeRoute(const RoutingRequest& request) const override;
private:
    std::uint64_t flowHash_;
};
} // namespace ealr
