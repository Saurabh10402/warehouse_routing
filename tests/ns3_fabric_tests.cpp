#include "ealr/simulation/ns3_fabric.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <set>

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
        auto priority = pair(); priority.scheduler = "strictPriority";
        priority.durationMs = 6; priority.drainMs = 1000; priority.links[0].queuePackets = 8;
        priority.traffic.clear();
        for (unsigned i = 0; i < 5; ++i) {
            const unsigned classes[] = {3,3,0,0,1};
            priority.traffic.push_back({"p"+std::to_string(i), "a", {"b"}, classes[i], 100,
                                       static_cast<double>(i), static_cast<double>(i+1), 10, 0});
        }
        auto ordered = simulateNs3Fabric(priority); std::vector<std::size_t> order;
        for (const auto& h : ordered.hops) if (h.event == "tx") order.push_back(h.packet);
        check(order == std::vector<std::size_t>({0,2,3,4,1}), "Nonpreemption, strict priority, FIFO within class");
        check(ordered.ipReceived == 5 && ordered.queues[0].peak == 4, "Priority shared-buffer conservation");
        priority.links[0].queuePackets = 1;
        auto full = simulateNs3Fabric(priority);
        check(full.packets[2].status == "queue_overflow" && full.packets[1].status == "delivered",
              "P0 arrival cannot evict a lower-class packet from a full shared buffer");
        priority.links[0].queuePackets = 16; priority.durationMs = 400; priority.drainMs = 0;
        priority.traffic.resize(2);
        priority.traffic.push_back({"flood", "a", {"b"}, 0, 100, 2, 400, 100, 0});
        auto starved = simulateNs3Fabric(priority);
        check(starved.packets[1].status == "cutoff" && starved.packets[2].status == "delivered" &&
              starved.packets[3].status == "delivered", "Sustained high priority can starve admitted lower traffic");

        diamond.nodes[2].role = "relay"; diamond.nodes[2].zone.clear();
        diamond.routing.algorithm = "ecmp"; diamond.durationMs = 100; diamond.drainMs = 1000;
        for (auto& link : diamond.links) { link.capacityMbps = 1; link.delayMs = .1; link.queuePackets = 512; }
        diamond.traffic.clear();
        for (unsigned i = 0; i < 32; ++i)
            diamond.traffic.push_back({"f"+std::to_string(i), "a", {"b"}, i%4, 100, i*3., i*3.+3, 1, 0});
        auto balanced = simulateNs3Fabric(diamond); std::set<std::string> branches;
        for (std::size_t i = 0; i < balanced.packets.size(); i += 3) {
            const auto& path = balanced.packets[i].observedPath; branches.insert(path.at(1));
            check(path == balanced.packets[i+1].observedPath && path == balanced.packets[i+2].observedPath,
                  "Consecutive packets retain the transport flow path");
        }
        check(branches.size() == 2 && balanced.ipReceived == 96, "ECMP uses both eligible branches for the declared flows");
        diamond.seed = 2; auto reseeded = simulateNs3Fabric(diamond); bool changed = false;
        for (std::size_t i = 0; i < balanced.packets.size(); ++i)
            changed = changed || balanced.packets[i].observedPath != reseeded.packets[i].observedPath;
        check(changed, "ECMP seed participates in transport hash");
        auto replay = simulateNs3Fabric(diamond);
        for (std::size_t i = 0; i < replay.packets.size(); ++i)
            check(replay.packets[i].observedPath == reseeded.packets[i].observedPath &&
                  replay.packets[i].finishedMs == reseeded.packets[i].finishedMs, "ECMP deterministic replay");
        diamond.nodes[2].role = "endpoint"; diamond.nodes[2].zone = "A";
        for (const auto& packet : simulateNs3Fabric(diamond).packets)
            check(packet.observedPath == std::vector<std::string>({"a","fast","b"}), "ECMP excludes endpoint transit");
        diamond.nodes[2].role = "relay"; diamond.nodes[2].zone.clear(); diamond.links[0].delayMs = 10;
        for (const auto& packet : simulateNs3Fabric(diamond).packets)
            check(packet.observedPath == std::vector<std::string>({"a","fast","b"}), "ECMP excludes more expensive branches");
        auto outage = pair(); outage.durationMs = 300;
        outage.events = {{"link","ab","",100,20,false},{"link","ab","",200,20,true}};
        auto crossing = simulateNs3Fabric(outage);
        check(crossing.packets[0].status == "availability_drop" && crossing.packets[1].status == "delivered" &&
              crossing.packets[2].status == "queue_overflow", "Receive-time outage gate preserves queued frame after recovery");
        check(crossing.packets[0].finishedMs == 132 && crossing.packets[1].finishedMs == 262 && crossing.availability.size() == 2,
              "Native outage loss and post-recovery queue delay observations");
        auto down = pair(); down.nodes[0].available = false;
        down.events = {{"node","a","",2,20,true}}; auto sourceOut = simulateNs3Fabric(down);
        check(sourceOut.packets[0].status == "source_unavailable" && sourceOut.packets[1].status == "source_unavailable" &&
              sourceOut.packets[2].status == "delivered" && sourceOut.ipTransmitted == 1, "Source outage/recovery precedes equal-time generation");
        down = pair(); down.links[0].available = false;
        check(simulateNs3Fabric(down).ipTransmitted == 0, "Initially unavailable link excludes all offered traffic from IP");
        auto services = pair(); services.durationMs = 60; services.drainMs = 100;
        services.links[0].capacityMbps = 5; services.links[0].queuePackets = 8;
        services.nodes.push_back({"backup","edge","","wired"});
        services.links.push_back({"abackup","a","backup",5,2,8,true});
        services.traffic[0].destinations.push_back("backup"); services.traffic[0].stopMs = 51; services.traffic[0].intervalMs = 10;
        services.events = {{"node","b","",5,20,false},{"node","b","",25,20,true}};
        auto failback = simulateNs3Fabric(services);
        check(failback.ipReceived == 6 && failback.packets[0].destination == "b" && failback.packets[1].destination == "backup" &&
              failback.packets[2].destination == "backup" && failback.packets[3].destination == "b", "Ordered backup selection and failback per new packet");
        services.events.push_back({"node","backup","",5,20,false});
        auto allDown = simulateNs3Fabric(services);
        check(allDown.packets[1].status == "no_route" && allDown.packets[2].status == "no_route", "All service candidates unreachable");
        auto chain = pair(); chain.durationMs = 60; chain.drainMs = 100;
        chain.nodes.push_back({"relay","relay","","wired"});
        chain.links = {{"ar","a","relay",5,10,8,true},{"rb","relay","b",5,2,8,true}};
        chain.traffic[0].stopMs = 51; chain.traffic[0].intervalMs = 10;
        chain.events = {{"link","rb","",5,20,false},{"link","rb","",15,20,true}};
        auto routeLost = simulateNs3Fabric(chain);
        check(routeLost.packets[0].status == "ip_drop" && routeLost.ipDrops.size() == 1 &&
              routeLost.packets[1].status == "no_route" && routeLost.packets[2].status == "delivered", "In-flight route loss observed at IPv4, then recovery");
        rejected(c, [](auto& x) { x.scheduler = "protectedPriority"; x.protectedQueues.reservedEmergencyPackets = 1; }, "protected queues");
        rejected(c, [](auto& x) { x.routing.algorithm = "greedy"; }, "legacy routing");
        rejected(c, [](auto& x) { x.events.push_back({"cloud", "", "", 5, 20, false}); }, "events");
        rejected(c, [](auto& x) { x.topology = "graph"; x.nodes[0].access = "wifi"; }, "Wi-Fi/NR");
        rejected(c, [](auto& x) { x.nodes[0].energyConstrained = true; }, "energy-constrained");
        rejected(c, [](auto& x) { x.links[0].capacityMbps = .0000015; }, "whole number");
        rejected(c, [](auto& x) { x.traffic[0].phaseJitterMs = 1; }, "jitter");
        rejected(c, [](auto& x) { x.traffic[0].bytes = 1473; }, "fragmentation");
        rejected(c, [](auto& x) { x.seed = 0; }, "seed");
        rejected(c, [](auto& x) { x.durationMs = 1e100; }, "total simulated time");
        check(simulateNs3Fabric(c).ipReceived == 3, "Rejected runs and earlier simulations leave no stale ns-3 state");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    return failures ? 1 : 0;
}
