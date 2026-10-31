#include "ealr/simulation/ns3_fabric.hpp"
#include "ealr/routing/dijkstra_routing.hpp"
#include "ealr/routing/ecmp_routing.hpp"
#include <boost/json.hpp>
#include <ns3/flow-monitor-helper.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-helper.h>
#include <ns3/mobility-model.h>
#include <ns3/point-to-point-module.h>
#include <ns3/position-allocator.h>
#include <ns3/rng-seed-manager.h>
#include <ns3/simulator.h>
#include <ns3/string.h>
#include <ns3/tag.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace ealr::fabric {
namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::invalid_argument("ns-3 wired fabric: " + message);
}
double nowMs() { return ns3::Simulator::Now().GetSeconds() * 1000; }
ns3::Time timeMs(double milliseconds) { return ns3::NanoSeconds(std::llround(milliseconds * 1e6)); }

// Observer metadata only: packet IDs add no on-wire bytes and do not select routes.
class ObservationTag : public ns3::Tag {
public:
    std::uint64_t id = 0;
    static ns3::TypeId GetTypeId() {
        static ns3::TypeId type = ns3::TypeId("ealr::FabricObservationTag")
            .SetParent<ns3::Tag>().AddConstructor<ObservationTag>();
        return type;
    }
    ns3::TypeId GetInstanceTypeId() const override { return GetTypeId(); }
    std::uint32_t GetSerializedSize() const override { return 8; }
    void Serialize(ns3::TagBuffer buffer) const override { buffer.WriteU64(id); }
    void Deserialize(ns3::TagBuffer buffer) override { id = buffer.ReadU64(); }
    void Print(std::ostream& out) const override { out << id; }
};

// Read real PPP/IPv4/UDP headers, never observer tags, for queue classification.
struct WireHeaders { ns3::Ipv4Header ip; ns3::UdpHeader udp; };
WireHeaders wireHeaders(ns3::Ptr<const ns3::Packet> packet) {
    auto copy = packet->Copy(); ns3::PppHeader ppp; WireHeaders headers;
    require(copy->RemoveHeader(ppp) == 2 && ppp.GetProtocol() == 0x0021,
            "queue requires PPP IPv4 frames");
    require(copy->RemoveHeader(headers.ip) == 20 && headers.ip.GetProtocol() == 17 &&
            copy->RemoveHeader(headers.udp) == 8, "queue requires unfragmented IPv4/UDP");
    return headers;
}
unsigned wirePriority(ns3::Ptr<const ns3::Packet> packet) {
    const auto tos = wireHeaders(packet).ip.GetTos();
    // Explicit local DSCP mapping P0..P3 -> CS7, CS5, CS3, CS0; ECN bits ignored.
    switch (tos >> 2) { case 56: return 0; case 40: return 1; case 24: return 2; case 0: return 3; }
    throw std::invalid_argument("ns-3 wired fabric: unknown priority DSCP");
}
class StrictPriorityQueue : public ns3::Queue<ns3::Packet> {
public:
    static ns3::TypeId GetTypeId() {
        static ns3::TypeId type = ns3::TypeId("ealr::FabricStrictPriorityQueue")
            .SetParent<ns3::Queue<ns3::Packet>>().AddConstructor<StrictPriorityQueue>();
        return type;
    }
    bool Enqueue(ns3::Ptr<ns3::Packet> packet) override {
        // Shared finite tail-drop buffer; no eviction or admission preference.
        return DoEnqueue(GetContainer().end(), packet);
    }
    ns3::Ptr<ns3::Packet> Dequeue() override { return DoDequeue(best()); }
    ns3::Ptr<ns3::Packet> Remove() override { return DoRemove(best()); }
    ns3::Ptr<const ns3::Packet> Peek() const override { return DoPeek(best()); }
private:
    ConstIterator best() const {
        // min_element keeps the first of equal priorities: FIFO within class.
        return std::min_element(GetContainer().begin(), GetContainer().end(),
            [](const auto& a, const auto& b) { return wirePriority(a) < wirePriority(b); });
    }
};
using FlowKey = std::tuple<std::uint32_t, std::uint32_t, unsigned, unsigned, unsigned>;
FlowKey flowKey(std::uint32_t source, std::uint32_t destination, unsigned sourcePort, unsigned destinationPort) {
    return {source, destination, 17, sourcePort, destinationPort};
}
std::uint64_t transportHash(const FlowKey& key, unsigned seed) {
    auto [source, destination, protocol, sourcePort, destinationPort] = key;
    std::uint64_t hash = UINT64_C(14695981039346656037);
    // Fixed-width big-endian fields: IPv4 addresses, protocol, UDP ports, seed.
    auto add = [&](std::uint32_t value, unsigned bytes) {
        for (unsigned i = bytes; i > 0; --i) {
            hash ^= (value >> ((i - 1) * 8)) & 255U; hash *= UINT64_C(1099511628211);
        }
    };
    add(source,4); add(destination,4); add(protocol,1); add(sourcePort,2); add(destinationPort,2); add(seed,4);
    return hash;
}
// Centrally installed static tables keyed by real five-tuple and payload size.
// Explicit source binding makes RouteOutput receive the UDP header at the IP layer.
class FabricRouting : public ns3::Ipv4RoutingProtocol {
public:
    using Key = std::pair<FlowKey, unsigned>;
    std::map<Key, ns3::Ptr<ns3::Ipv4Route>> routes;
    ns3::Ipv4Address address;
    ns3::Ptr<ns3::Ipv4> ipv4;
    static ns3::TypeId GetTypeId() {
        static ns3::TypeId type = ns3::TypeId("ealr::FabricRouting")
            .SetParent<ns3::Ipv4RoutingProtocol>().AddConstructor<FabricRouting>();
        return type;
    }
    ns3::Ptr<ns3::Ipv4Route> lookup(const FlowKey& key, unsigned payload) const {
        auto found = routes.find({key, payload});
        return found == routes.end() ? nullptr : found->second;
    }
    ns3::Ptr<ns3::Ipv4Route> packetRoute(ns3::Ptr<const ns3::Packet> packet, const ns3::Ipv4Header& ip) const {
        if (!packet || ip.GetProtocol() != 17 || packet->GetSize() < 8) return nullptr;
        ns3::UdpHeader udp; packet->PeekHeader(udp);
        return lookup(flowKey(ip.GetSource().Get(), ip.GetDestination().Get(), udp.GetSourcePort(), udp.GetDestinationPort()), packet->GetSize()-8);
    }
    ns3::Ptr<ns3::Ipv4Route> RouteOutput(ns3::Ptr<ns3::Packet> packet,
        const ns3::Ipv4Header& header, ns3::Ptr<ns3::NetDevice> device,
        ns3::Socket::SocketErrno& error) override {
        auto route = packetRoute(packet, header);
        if (route && device && route->GetOutputDevice() != device) route = nullptr;
        error = route ? ns3::Socket::ERROR_NOTERROR : ns3::Socket::ERROR_NOROUTETOHOST;
        return route;
    }
    bool RouteInput(ns3::Ptr<const ns3::Packet> packet, const ns3::Ipv4Header& header,
        ns3::Ptr<const ns3::NetDevice> device, const UnicastForwardCallback& forward,
        const MulticastForwardCallback&, const LocalDeliverCallback& local,
        const ErrorCallback& error) override {
        auto interface = ipv4->GetInterfaceForDevice(device);
        if (interface < 0) return false;
        if (header.GetDestination() == address) {
            local(packet, header, static_cast<unsigned>(interface));
            return true;
        }
        auto route = header.GetProtocol() == 17 && packet->GetSize() >= 8 &&
                     ipv4->IsForwarding(static_cast<unsigned>(interface))
            ? packetRoute(packet, header) : nullptr;
        if (route) forward(route, packet, header);
        else error(packet, header, ns3::Socket::ERROR_NOROUTETOHOST);
        return true;
    }
    void SetIpv4(ns3::Ptr<ns3::Ipv4> value) override { ipv4 = value; }
    void NotifyInterfaceUp(std::uint32_t) override {}
    void NotifyInterfaceDown(std::uint32_t) override {}
    void NotifyAddAddress(std::uint32_t, ns3::Ipv4InterfaceAddress) override {}
    void NotifyRemoveAddress(std::uint32_t, ns3::Ipv4InterfaceAddress) override {}
    void PrintRoutingTable(ns3::Ptr<ns3::OutputStreamWrapper> out, ns3::Time::Unit) const override {
        *out->GetStream() << "Static transport/size-aware wired fabric table: " << routes.size() << " entries\n";
    }
    void DoDispose() override { routes.clear(); ipv4 = nullptr; ns3::Ipv4RoutingProtocol::DoDispose(); }
};

struct Direction {
    std::size_t from, to;
    ns3::Ptr<ns3::PointToPointNetDevice> device;
    ns3::Ipv4Address gateway;
};
struct Observer {
    const Configuration& config;
    Ns3FabricResult result;
    std::vector<Direction> directions;
    std::vector<ns3::Ptr<ns3::Socket>> senders, receivers;
    std::vector<ns3::Ipv4Address> destinations;
    std::uint64_t socketAccepted = 0;

    void trace(std::size_t direction, unsigned kind, ns3::Ptr<const ns3::Packet> packet) {
        ObservationTag tag;
        if (!packet->PeekPacketTag(tag)) return;
        auto& record = result.packets.at(static_cast<std::size_t>(tag.id));
        auto& queue = result.queues.at(direction);
        if (kind == 0) ++queue.attempts;
        if (kind == 1) {
            ++queue.admitted;
            queue.peak = std::max(queue.peak, static_cast<std::uint64_t>(directions[direction].device->GetQueue()->GetNPackets()));
        }
        if (kind == 2) ++queue.started;
        if (kind == 3) {
            ++queue.dropped;
            record.status = "queue_overflow";
            record.finishedMs = nowMs();
        }
        if (kind == 4) record.observedPath.push_back(queue.from); // receiving-side direction
        {
            const bool receive = kind == 4;
            result.hops.push_back({record.id, queue.link, receive ? queue.to : queue.from,
                receive ? queue.from : queue.to, kind == 0 ? "attempt" : (kind == 1 ? "enqueue" : (kind == 2 ? "tx" : (receive ? "rx" : "drop"))),
                nowMs(), packet->GetSize()});
            const auto headers = wireHeaders(packet);
            auto& hop = result.hops.back(); hop.ipTos = headers.ip.GetTos();
            hop.sourcePort = headers.udp.GetSourcePort(); hop.destinationPort = headers.udp.GetDestinationPort();
            hop.sourceAddress = headers.ip.GetSource().Get(); hop.destinationAddress = headers.ip.GetDestination().Get();
        }
    }
    void receive(ns3::Ptr<ns3::Socket> socket) {
        while (auto packet = socket->Recv()) {
            ObservationTag tag;
            require(packet->PeekPacketTag(tag), "receiver lost observation metadata");
            auto& record = result.packets.at(static_cast<std::size_t>(tag.id));
            require(record.status == "pending", "duplicate or already-dropped packet delivered");
            require(packet->GetSize() == config.traffic.at(record.flowIndex).bytes, "UDP payload size changed");
            record.status = "delivered";
            record.finishedMs = nowMs();
        }
    }
    void send(std::size_t id) {
        auto& record = result.packets.at(id);
        const auto& flow = config.traffic.at(record.flowIndex);
        record.createdMs = nowMs();
        // Explicit-address UDP sockets report acceptance even without an IP route.
        // Check the installed static table before sending, preserving no-route accounting.
        if (record.expectedPath.empty()) {
            record.status = "no_route"; record.finishedMs = nowMs(); return;
        }
        auto packet = ns3::Create<ns3::Packet>(flow.bytes);
        ObservationTag tag; tag.id = id; packet->AddPacketTag(tag);
        const int sent = senders.at(record.flowIndex)->SendTo(packet, 0,
            ns3::InetSocketAddress(destinations.at(record.flowIndex), static_cast<std::uint16_t>(10000 + record.flowIndex)));
        if (sent < 0) {
            require(senders.at(record.flowIndex)->GetErrno() == ns3::Socket::ERROR_NOROUTETOHOST, "unexpected UDP send failure");
            record.status = "no_route"; record.finishedMs = nowMs();
        } else {
            require(sent == static_cast<int>(flow.bytes), "partial UDP send");
            ++socketAccepted;
        }
    }
};
void tracePacket(Observer* observer, std::size_t direction, unsigned kind, ns3::Ptr<const ns3::Packet> packet) {
    observer->trace(direction, kind, packet);
}
std::string pathText(const std::vector<std::string>& path) {
    std::string text;
    for (const auto& node : path) { if (!text.empty()) text += '>'; text += node; }
    return text;
}
} // namespace

void validateNs3Fabric(const Configuration& c) {
    validate(c);
    require(c.seed > 0 && c.seed <= 1000000000, "seed must be in [1,1000000000]");
    require(c.durationMs >= 1e-6 && c.durationMs + c.drainMs <= 1e9,
            "duration must be at least 1 ns and total simulated time at most 1e9 ms");
    require(c.scheduler == "fifo" || c.scheduler == "strictPriority", "protected queues are not implemented; use fifo/strictPriority");
    require(c.routing.algorithm == "dijkstra" || c.routing.algorithm == "ecmp", "legacy routing is not implemented; use dijkstra/ecmp");
    require(c.events.empty(), "availability/safety/cloud events are not implemented");
    require(c.nodes.size() <= 4096 && c.links.size() <= 16384 && c.traffic.size() <= 4096,
            "initial backend limits are 4096 nodes/flows and 16384 links");
    for (const auto& n : c.nodes) {
        require(n.access == "wired", n.id + ": Wi-Fi/NR access is not implemented");
        require(n.available && !n.energyConstrained, n.id + ": unavailable or energy-constrained nodes are not implemented");
    }
    for (const auto& link : c.links) {
        require(link.available, link.id + ": unavailable links are not implemented");
        const double bitsPerSecond = link.capacityMbps * 1e6;
        require(std::abs(bitsPerSecond - std::round(bitsPerSecond)) <= 1e-6,
                link.id + ": ns-3 requires a whole number of bits per second");
    }
    double offered = 0;
    for (const auto& flow : c.traffic) {
        require(flow.destinations.size() == 1, flow.id + ": backup destinations are not implemented");
        require(flow.source != flow.destinations.front(), flow.id + ": self-destination traffic is not implemented");
        require(flow.phaseJitterMs == 0, flow.id + ": stochastic phase jitter is not implemented");
        require(flow.bytes <= 1472, flow.id + ": UDP payload must be <=1472 bytes; fragmentation is not implemented");
        offered += std::ceil((flow.stopMs - flow.startMs) / flow.intervalMs);
    }
    require(offered <= 1000000, "initial trace limit is one million offered packets");
}

Ns3FabricResult simulateNs3Fabric(const Configuration& c) {
    validateNs3Fabric(c);
    struct Cleanup { ~Cleanup() { ns3::Simulator::Destroy(); } } cleanup;
    ns3::RngSeedManager::SetSeed(c.seed); ns3::RngSeedManager::SetRun(1);
    Observer observer{c, {}, {}, {}, {}, {}, 0};
    ns3::NodeContainer nodes; nodes.Create(static_cast<unsigned>(c.nodes.size()));
    std::map<std::string, std::size_t> ids;
    auto positions = ns3::CreateObject<ns3::ListPositionAllocator>();
    for (std::size_t i = 0; i < c.nodes.size(); ++i) {
        ids[c.nodes[i].id] = i;
        positions->Add(ns3::Vector(c.nodes[i].position.x, c.nodes[i].position.y, 0));
    }
    ns3::MobilityHelper mobility;
    mobility.SetPositionAllocator(positions); mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); mobility.Install(nodes);
    ns3::InternetStackHelper internet; internet.SetIpv6StackInstall(false); internet.Install(nodes);
    std::vector<ns3::Ptr<FabricRouting>> routing;
    for (std::size_t i = 0; i < c.nodes.size(); ++i) {
        auto ipv4 = nodes.Get(static_cast<unsigned>(i))->GetObject<ns3::Ipv4>();
        auto protocol = ns3::CreateObject<FabricRouting>();
        protocol->address = ns3::Ipv4Address(0xac100001U + static_cast<unsigned>(i));
        // Stable /32 node address, independent of which physical ingress link is used.
        ipv4->AddAddress(0, ns3::Ipv4InterfaceAddress(protocol->address, ns3::Ipv4Mask("255.255.255.255")));
        ipv4->SetRoutingProtocol(protocol); routing.push_back(protocol);
    }
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> directions;
    std::map<std::pair<std::size_t, std::size_t>, const Link*> links;
    ns3::Ipv4AddressHelper addresses; addresses.SetBase("10.0.0.0", "255.255.255.252");
    for (const auto& link : c.links) {
        auto a = ids.at(link.from), b = ids.at(link.to);
        ns3::PointToPointHelper helper;
        // Exactly one finite waiting queue per direction, with no hidden QueueDisc.
        helper.DisableFlowControl();
        helper.SetDeviceAttribute("DataRate", ns3::DataRateValue(ns3::DataRate(static_cast<std::uint64_t>(std::llround(link.capacityMbps * 1e6)))));
        helper.SetChannelAttribute("Delay", ns3::TimeValue(timeMs(link.delayMs)));
        helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", ns3::QueueSizeValue(ns3::QueueSize(std::to_string(link.queuePackets) + "p")));
        auto devices = helper.Install(nodes.Get(static_cast<unsigned>(a)), nodes.Get(static_cast<unsigned>(b)));
        auto interfaces = addresses.Assign(devices); addresses.NewNetwork();
        for (unsigned side = 0; side < 2; ++side) {
            auto from = side == 0 ? a : b, to = side == 0 ? b : a;
            auto device = ns3::DynamicCast<ns3::PointToPointNetDevice>(devices.Get(side));
            if (c.scheduler == "strictPriority") {
                auto queue = ns3::CreateObject<StrictPriorityQueue>();
                queue->SetMaxSize(ns3::QueueSize(std::to_string(link.queuePackets) + "p"));
                device->SetQueue(queue);
            }
            auto index = observer.directions.size();
            directions[{from, to}] = index; links[{from, to}] = &link;
            observer.directions.push_back({from, to, device, interfaces.GetAddress(1 - side)});
            observer.result.queues.push_back({link.id, c.nodes[from].id, c.nodes[to].id});
            auto ipv4 = nodes.Get(static_cast<unsigned>(from))->GetObject<ns3::Ipv4>();
            ipv4->SetForwarding(static_cast<unsigned>(ipv4->GetInterfaceForDevice(device)), c.nodes[from].role != "endpoint");
            for (const auto& event : std::vector<std::pair<std::string, unsigned>>{{"MacTx",0},{"PhyTxBegin",2},{"MacTxDrop",3},{"PhyRxEnd",4}})
                require(device->TraceConnectWithoutContext(event.first, ns3::MakeBoundCallback(&tracePacket, &observer, index, event.second)), "trace unavailable: " + event.first);
            require(device->GetQueue()->TraceConnectWithoutContext("Enqueue", ns3::MakeBoundCallback(&tracePacket, &observer, index, 1U)), "queue trace unavailable");
        }
    }
    for (std::size_t f = 0; f < c.traffic.size(); ++f) for (std::size_t source = 0; source < c.nodes.size(); ++source) {
        const auto& flow = c.traffic[f]; const auto target = ids.at(flow.destinations.front());
        const auto payload = flow.bytes;
        const auto key = flowKey(routing[ids.at(flow.source)]->address.Get(), routing[target]->address.Get(),
                                 static_cast<unsigned>(20000+f), static_cast<unsigned>(10000+f));
        if (source == target) continue;
        std::vector<RoutingNode> snapshot;
        for (std::size_t i = 0; i < c.nodes.size(); ++i)
            snapshot.push_back({static_cast<RobotId>(i), c.nodes[i].position, 100, 0, 1, RobotState::Idle, {}, false});
        for (const auto& [pair, link] : links) {
            static_cast<void>(link);
            if (c.nodes[pair.first].role != "endpoint" || pair.first == source)
                snapshot[pair.first].neighbours.push_back(static_cast<RobotId>(pair.second));
        }
        RoutingTopology topology(std::move(snapshot));
        const RoutingRequest request{topology, static_cast<RobotId>(source), static_cast<RobotId>(target),
            [&](const RoutingNode& from, const RoutingNode& to) {
                const auto& link = *links.at({from.id, to.id});
                return link.delayMs + (payload + 30.0) * 8 / (link.capacityMbps * 1000);
            }};
        auto path = c.routing.algorithm == "ecmp" ? EcmpRouting(transportHash(key, c.seed)).computeRoute(request)
                                                  : DijkstraRouting().computeRoute(request);
        if (!path.found()) continue;
        require(path.hops.size() <= 255, "route exceeds supported IPv4 TTL");
        const auto& direction = observer.directions.at(directions.at({source, path.hops.at(1)}));
        auto route = ns3::Create<ns3::Ipv4Route>();
        route->SetDestination(routing[target]->address); route->SetSource(routing[source]->address);
        route->SetGateway(direction.gateway); route->SetOutputDevice(direction.device);
        routing[source]->routes[{key, payload}] = route;
    }
    ns3::FlowMonitorHelper flowMonitor;
    flowMonitor.SetMonitorAttribute("MaxPerHopDelay", ns3::TimeValue(timeMs(c.durationMs + c.drainMs) + ns3::Seconds(1)));
    auto monitor = flowMonitor.InstallAll();
    for (std::size_t f = 0; f < c.traffic.size(); ++f) {
        const auto& flow = c.traffic[f]; auto source = ids.at(flow.source), target = ids.at(flow.destinations.front());
        auto receiver = ns3::Socket::CreateSocket(nodes.Get(static_cast<unsigned>(target)), ns3::UdpSocketFactory::GetTypeId());
        require(receiver->Bind(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), static_cast<std::uint16_t>(10000 + f))) == 0, "UDP receiver bind failed");
        receiver->SetRecvCallback(ns3::MakeCallback(&Observer::receive, &observer));
        auto sender = ns3::Socket::CreateSocket(nodes.Get(static_cast<unsigned>(source)), ns3::UdpSocketFactory::GetTypeId());
        require(sender->Bind(ns3::InetSocketAddress(routing[source]->address, static_cast<std::uint16_t>(20000+f))) == 0, "UDP sender bind failed");
        sender->SetIpTtl(255);
        const unsigned tos[] = {224,160,96,0}; sender->SetIpTos(static_cast<std::uint8_t>(tos[flow.priority]));
        const auto key = flowKey(routing[source]->address.Get(), routing[target]->address.Get(),
                                 static_cast<unsigned>(20000+f), static_cast<unsigned>(10000+f));
        observer.senders.push_back(sender); observer.receivers.push_back(receiver); observer.destinations.push_back(routing[target]->address);
        std::vector<std::string> expected{flow.source};
        auto node = source;
        while (node != target) {
            auto route = routing[node]->lookup(key, flow.bytes);
            if (!route) { expected.clear(); break; }
            auto direction = std::find_if(observer.directions.begin(), observer.directions.end(), [&](const auto& d) { return d.device == route->GetOutputDevice(); });
            require(direction != observer.directions.end(), "route has unknown output device");
            node = direction->to; expected.push_back(c.nodes[node].id);
            require(expected.size() <= c.nodes.size(), "forwarding table contains a loop");
        }
        for (std::uint64_t k = 0; ; ++k) {
            const double created = flow.startMs + static_cast<double>(k) * flow.intervalMs;
            if (created >= flow.stopMs) break;
            auto id = observer.result.packets.size();
            require(id < 1000000, "initial trace limit is one million offered packets");
            observer.result.packets.push_back({id, f, "pending", created, 0, {flow.source}, expected});
            ns3::Simulator::Schedule(timeMs(created), &Observer::send, &observer, id);
        }
    }
    ns3::Simulator::Stop(timeMs(c.durationMs + c.drainMs)); ns3::Simulator::Run();
    // Classify outstanding IP packets at the measurement cutoff, NOT as proven permanent loss.
    monitor->CheckForLostPackets(ns3::Seconds(0));
    for (const auto& [id, stats] : monitor->GetFlowStats()) {
        static_cast<void>(id);
        observer.result.ipTransmitted += stats.txPackets; observer.result.ipReceived += stats.rxPackets;
        observer.result.ipUnreceivedAtCutoff += stats.lostPackets; observer.result.ipReceivedBytes += stats.rxBytes;
    }
    std::uint64_t received = 0, bytes = 0;
    for (auto& packet : observer.result.packets) {
        if (packet.status == "pending") { packet.status = "cutoff"; packet.finishedMs = c.durationMs + c.drainMs; }
        if (packet.status == "delivered") {
            ++received; bytes += c.traffic[packet.flowIndex].bytes + 28;
            require(packet.observedPath == packet.expectedPath, "observed packet path differs from installed forwarding table");
        }
    }
    require(observer.socketAccepted == observer.result.ipTransmitted && received == observer.result.ipReceived &&
            bytes == observer.result.ipReceivedBytes && observer.result.ipTransmitted == received + observer.result.ipUnreceivedAtCutoff,
            "application observations and independent FlowMonitor totals disagree");
    observer.result.stationary = true;
    for (std::size_t i = 0; i < c.nodes.size(); ++i) {
        auto position = nodes.Get(static_cast<unsigned>(i))->GetObject<ns3::MobilityModel>()->GetPosition();
        observer.result.finalPositions.push_back({position.x, position.y, position.z});
        observer.result.stationary = observer.result.stationary && position.x == c.nodes[i].position.x && position.y == c.nodes[i].position.y && position.z == 0;
    }
    for (std::size_t i = 0; i < observer.directions.size(); ++i) {
        auto& queue = observer.result.queues[i]; queue.waiting = observer.directions[i].device->GetQueue()->GetNPackets();
        require(queue.attempts == queue.admitted + queue.dropped && queue.admitted == queue.started + queue.waiting,
                "native device queue conservation failed");
    }
    for (auto socket : observer.senders) socket->Close();
    for (auto socket : observer.receivers) socket->Close();
    return std::move(observer.result);
}

void writeNs3FabricResults(const Configuration& c, const Ns3FabricResult& r, const std::filesystem::path& output) {
    require(!output.empty() && !std::filesystem::exists(output), "output exists or is empty; choose a new directory");
    require(std::filesystem::create_directories(output), "could not create output directory");
    std::ofstream packets(output / "packets.csv"), hops(output / "hops.csv"), queues(output / "queues.csv"), nodes(output / "nodes.csv");
    packets << std::setprecision(17) << "id,flow,source,destination,priority,payload_bytes,status,created_ms,finished_ms,deadline_ms,deadline_met,observed_path,expected_path\n";
    std::uint64_t delivered = 0, missed = 0;
    boost::json::object statuses;
    for (const auto& p : r.packets) {
        const auto& f = c.traffic.at(p.flowIndex);
        const bool met = p.status == "delivered" && p.finishedMs - p.createdMs <= c.deadlinesMs[f.priority] + 1e-9;
        delivered += p.status == "delivered"; missed += !met;
        if (!statuses.contains(p.status)) statuses[p.status] = 0;
        statuses[p.status] = statuses[p.status].as_int64() + 1;
        packets << p.id << ',' << f.id << ',' << f.source << ',' << f.destinations.front() << ',' << f.priority << ',' << f.bytes << ',' << p.status << ','
                << p.createdMs << ',' << p.finishedMs << ',' << c.deadlinesMs[f.priority] << ',' << met << ',' << pathText(p.observedPath) << ',' << pathText(p.expectedPath) << '\n';
    }
    hops << std::setprecision(17) << "packet,link,from,to,event,time_ms,wire_bytes,ip_tos,source_port,destination_port,source_ipv4,destination_ipv4\n";
    for (const auto& h : r.hops) hops << h.packet << ',' << h.link << ',' << h.from << ',' << h.to << ',' << h.event << ',' << h.timeMs << ',' << h.wireBytes << ',' << h.ipTos << ',' << h.sourcePort << ',' << h.destinationPort << ',' << ns3::Ipv4Address(h.sourceAddress) << ',' << ns3::Ipv4Address(h.destinationAddress) << '\n';
    queues << "link,from,to,attempts,admitted,started,dropped,waiting_at_cutoff,peak_waiting\n";
    for (const auto& q : r.queues) queues << q.link << ',' << q.from << ',' << q.to << ',' << q.attempts << ',' << q.admitted << ',' << q.started << ',' << q.dropped << ',' << q.waiting << ',' << q.peak << '\n';
    nodes << std::setprecision(17) << "id,role,zone,x,y,z\n";
    for (std::size_t i = 0; i < c.nodes.size(); ++i) {
        const auto& position = r.finalPositions.at(i);
        nodes << c.nodes[i].id << ',' << c.nodes[i].role << ',' << c.nodes[i].zone << ',' << position[0] << ',' << position[1] << ',' << position[2] << '\n';
    }
    boost::json::object summary{{"model", "ns-3 stationary wired IPv4/UDP schema-v2 subset; not Ethernet switching or radio"},
        {"scheduler", c.scheduler}, {"routing", c.routing.algorithm == "dijkstra" ? "static size-aware Dijkstra; propagation plus payload/UDP/IPv4/PPP serialization" : "static five-tuple ECMP; propagation plus payload/UDP/IPv4/PPP serialization"},
        {"offered_packets", r.packets.size()}, {"delivered_packets", delivered}, {"deadline_missed_packets", missed},
        {"packet_statuses", statuses}, {"stationary", r.stationary}, {"cutoff_ms", c.durationMs + c.drainMs},
        {"ip_transmitted_packets", r.ipTransmitted}, {"ip_received_packets", r.ipReceived},
        {"ip_unreceived_at_cutoff", r.ipUnreceivedAtCutoff}, {"ip_received_bytes", r.ipReceivedBytes},
        {"byte_contract", "traffic.bytes = UDP payload; IPv4 bytes = payload+28; PPP wire bytes = payload+30"},
        {"loss_contract", "all offered packets in deadline denominator; cutoff is censored, not proven permanent loss"}};
    std::ofstream report(output / "summary.json"); report << boost::json::serialize(summary) << '\n';
    packets.close(); hops.close(); queues.close(); nodes.close(); report.close();
    require(bool(packets) && bool(hops) && bool(queues) && bool(nodes) && bool(report), "could not write result files");
}
} // namespace ealr::fabric
