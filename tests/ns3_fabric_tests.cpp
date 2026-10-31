#include "ealr/simulation/ns3_fabric.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>

using namespace ealr::fabric;
namespace {
int failures = 0;
void check(bool valid, const std::string& message) {
    if (!valid) { ++failures; std::cerr << message << '\n'; }
}
Configuration pair() {
    Configuration c; c.name = "native queue and cutoff accounting"; c.zones = {"A"};
    c.scheduler = "fifo"; c.durationMs = 3; c.drainMs = 500;
    c.nodes = {{"a", "endpoint", "A", "wired"}, {"b", "edge", "", "wired"}};
    c.links = {{"ab", "a", "b", .008, 2, 1, true}};
    c.traffic = {{"flow", "a", {"b"}, 0, 100, 0, 3, 1, 0}};
    return c;
}
void rejected(Configuration c, const std::function<void(Configuration&)>& change, const std::string& text) {
    change(c);
    try { simulateNs3Fabric(c); check(false, "Unsupported input accepted: " + text); }
    catch (const std::invalid_argument& error) { check(std::string(error.what()).find(text) != std::string::npos, error.what()); }
}
}
int main() {
    try {
        auto c = load(std::filesystem::path(EALR_SOURCE_DIRECTORY) / "configs/warehouse_ns3_wired.json");
        auto result = simulateNs3Fabric(c);
        check(result.packets.size() == 3 && result.ipReceived == 3 && result.ipReceivedBytes == 384 && result.stationary, "Warehouse delivery/bytes/stationarity");
        const std::vector<std::vector<std::string>> paths{
            {"A0", "leafA", "A1"}, {"A0", "leafA", "spine", "leafB", "B0"}, {"C2", "leafC", "spine", "edge"}};
        for (std::size_t i = 0; i < paths.size(); ++i) {
            const auto& packet = result.packets.at(i);
            check(packet.observedPath == paths[i], "Actual same-zone/cross-zone/edge path");
            check(std::abs(packet.finishedMs - packet.createdMs - (paths[i].size() - 1) * 2.208) < 1e-6, "Analytical delay over every observed hop");
        }
        check(result.hops.size() == 36, "Every successful hop has attempt/enqueue/tx/rx observations");
        for (const auto& hop : result.hops) check(hop.wireBytes == 130, "PPP wire bytes include payload+30");

        auto overloaded = simulateNs3Fabric(pair());
        check(overloaded.packets[0].status == "delivered" && overloaded.packets[1].status == "delivered" &&
              overloaded.packets[2].status == "queue_overflow", "One waiting slot must admit exactly two overlapping packets including in-service packet");
        check(overloaded.ipTransmitted == 3 && overloaded.ipReceived == 2 && overloaded.ipUnreceivedAtCutoff == 1, "FlowMonitor must include native queue loss");
        const auto& q = overloaded.queues[0];
        check(q.attempts == 3 && q.admitted == 2 && q.started == 2 && q.dropped == 1 && q.waiting == 0, "Native queue conservation, no hidden buffer");
        check(std::abs(overloaded.packets[1].finishedMs - 262) < 1e-6, "FIFO service includes preceding frame serialization");
        auto shortRun = pair(); shortRun.drainMs = 0;
        auto cutoff = simulateNs3Fabric(shortRun);
        check(cutoff.packets[0].status == "cutoff" && cutoff.packets[1].status == "cutoff" && cutoff.packets[2].status == "queue_overflow", "Censored packets are distinct from observed drops");
        check(cutoff.ipReceived == 0 && cutoff.ipUnreceivedAtCutoff == 3 && cutoff.queues[0].waiting == 1, "Cutoff includes in-flight and queued packets");
        auto duplex = pair(); duplex.traffic.push_back({"reverse", "b", {"a"}, 3, 100, 0, 3, 1, 0});
        auto both = simulateNs3Fabric(duplex);
        check(both.ipReceived == 4 && both.ipUnreceivedAtCutoff == 2 && both.queues[1].dropped == 1, "Full duplex has independent queues");
        auto disconnected = pair(); disconnected.links.clear();
        auto lost = simulateNs3Fabric(disconnected);
        check(lost.packets.size() == 3 && lost.ipTransmitted == 0 && lost.ipReceived == 0 &&
              std::all_of(lost.packets.begin(), lost.packets.end(), [](const auto& p) { return p.status == "no_route"; }), "Unroutable offered packets remain observable before IP transmission");

        auto diamond = pair(); diamond.durationMs = 100; diamond.drainMs = 20;
        diamond.nodes.push_back({"slow", "relay", "", "wired"}); diamond.nodes.push_back({"fast", "relay", "", "wired"});
        diamond.links = {{"as", "a", "slow", 1, .1, 8, true}, {"sb", "slow", "b", 1, .1, 8, true},
                         {"af", "a", "fast", 100, .6, 8, true}, {"fb", "fast", "b", 100, .6, 8, true}};
        diamond.traffic = {{"small", "a", {"b"}, 0, 1, 10, 11, 10, 0}, {"large", "a", {"b"}, 3, 1400, 50, 51, 10, 0}};
        auto sizes = simulateNs3Fabric(diamond);
        check(sizes.packets[0].observedPath == std::vector<std::string>({"a", "slow", "b"}) &&
              sizes.packets[1].observedPath == std::vector<std::string>({"a", "fast", "b"}), "Mixed payload lengths choose different minimum wire-delay paths");
        check(std::abs(sizes.packets[0].finishedMs - 10.696) < 1e-6 &&
              std::abs(sizes.packets[1].finishedMs - 51.4288) < 1e-6, "Mixed-size analytical delay");
        diamond.nodes[2].role = "endpoint"; diamond.nodes[2].zone = "A";
        auto transit = simulateNs3Fabric(diamond);
        check(transit.packets[0].observedPath == std::vector<std::string>({"a", "fast", "b"}), "Endpoints never act as transit routers");
        rejected(c, [](auto& x) { x.scheduler = "strictPriority"; }, "scheduler=fifo");
        rejected(c, [](auto& x) { x.routing.algorithm = "ecmp"; }, "algorithm=dijkstra");
        rejected(c, [](auto& x) { x.events.push_back({"node", "spine", "", 5, 20, false}); }, "events");
        rejected(c, [](auto& x) { x.topology = "graph"; x.nodes[0].access = "wifi"; }, "Wi-Fi/NR");
        rejected(c, [](auto& x) { x.nodes[0].available = false; }, "unavailable");
        rejected(c, [](auto& x) { x.nodes[0].energyConstrained = true; }, "energy-constrained");
        rejected(c, [](auto& x) { x.links[0].available = false; }, "unavailable");
        rejected(c, [](auto& x) { x.links[0].capacityMbps = .0000015; }, "whole number");
        rejected(c, [](auto& x) { x.traffic[0].destinations.push_back("edge"); }, "backup");
        rejected(c, [](auto& x) { x.traffic[0].phaseJitterMs = 1; }, "jitter");
        rejected(c, [](auto& x) { x.traffic[0].bytes = 1473; }, "fragmentation");
        rejected(c, [](auto& x) { x.seed = 0; }, "seed");
        rejected(c, [](auto& x) { x.durationMs = 1e100; }, "total simulated time");
        check(simulateNs3Fabric(c).ipReceived == 3, "Rejected runs and earlier simulations leave no stale ns-3 state");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    return failures ? 1 : 0;
}
