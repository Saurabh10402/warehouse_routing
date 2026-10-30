#include "ealr/routing/ecmp_routing.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <stdexcept>

namespace ealr {
RouteResult EcmpRouting::computeRoute(const RoutingRequest& request) const {
    if (const auto invalid = validateRequest(request)) return *invalid;
    if (request.source == request.destination) return {RouteStatus::Found, {request.source}, 0};

    using Edge = std::pair<RobotId, double>;
    std::map<RobotId, std::vector<Edge>> incoming, outgoing;
    std::map<RobotId, double> distance;
    for (const auto& node : request.topology.nodes()) {
        distance[node.id] = std::numeric_limits<double>::infinity();
        if (!node.isAvailableForRouting()) continue;
        for (auto id : node.neighbours) {
            const auto& next = *request.topology.findNode(id);
            if (!next.isAvailableForRouting()) continue;
            double cost = checkedRoutingCost(request.linkCost ? request.linkCost(node, next) : node.position.distanceTo(next.position));
            if (cost == 0) throw std::invalid_argument("ECMP requires strictly positive link costs");
            incoming[id].push_back({node.id, cost});
            outgoing[node.id].push_back({id, cost});
        }
    }
    using Entry = std::pair<double, RobotId>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
    distance[request.destination] = 0;
    queue.push({0, request.destination});
    while (!queue.empty()) {
        const auto [cost, id] = queue.top(); queue.pop();
        if (cost != distance[id]) continue;
        for (const auto& [previous, edgeCost] : incoming[id]) {
            double candidate = checkedRoutingCost(cost + edgeCost);
            if (candidate < distance[previous]) {
                distance[previous] = candidate;
                queue.push({candidate, previous});
            }
        }
    }
    if (!std::isfinite(distance[request.source])) return {};
    RouteResult result{RouteStatus::Found, {request.source}, 0};
    auto current = request.source;
    while (current != request.destination) {
        std::vector<Edge> candidates;
        for (const auto& edge : outgoing[current]) {
            const auto [next, cost] = edge;
            double total = cost + distance[next];
            // Strictly decreasing remaining cost prevents loops, including roundoff ties.
            if (distance[next] < distance[current] &&
                std::abs(total - distance[current]) <= 1e-12 * std::max(1.0, distance[current]))
                candidates.push_back(edge);
        }
        if (candidates.empty()) return {};
        std::sort(candidates.begin(), candidates.end());
        // SplitMix64 finalizer: stable across platforms, no per-packet random spraying.
        std::uint64_t hash = flowHash_ + static_cast<std::uint64_t>(current) + UINT64_C(0x9e3779b97f4a7c15);
        hash = (hash ^ (hash >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
        hash ^= hash >> 31;
        const auto& [next, cost] = candidates[static_cast<std::size_t>(hash % candidates.size())];
        result.totalCost = checkedRoutingCost(result.totalCost + cost);
        result.hops.push_back(next); current = next;
    }
    return result;
}
} // namespace ealr
