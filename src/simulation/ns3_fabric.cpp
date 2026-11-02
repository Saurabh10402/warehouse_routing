#include "ealr/simulation/ns3_fabric.hpp"
#include "ealr/routing/dijkstra_routing.hpp"
#include "ealr/routing/ecmp_routing.hpp"
#include <boost/json.hpp>
#include <ns3/flow-monitor-helper.h>
#include <ns3/error-model.h>
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
#include <functional>
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
        // Bound transient loops without generating unsupported ICMP control traffic.
        // The IPv4 error callback emits an observable DROP_ROUTE_ERROR with TTL=1.
        if (route && header.GetTtl() > 1) forward(route, packet, header);
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

// Declared receive-time outage gate: serialization/queues continue during outages.
class AvailabilityGate : public ns3::ErrorModel {
public:
    std::function<bool()> available;
    static ns3::TypeId GetTypeId() {
        static ns3::TypeId type = ns3::TypeId("ealr::FabricAvailabilityGate")
            .SetParent<ns3::ErrorModel>().AddConstructor<AvailabilityGate>();
        return type;
    }
private:
    bool DoCorrupt(ns3::Ptr<ns3::Packet>) override { return !available(); }
    void DoReset() override {}
};
struct Direction {
    std::size_t from, to;
    ns3::Ptr<ns3::PointToPointNetDevice> device;
    ns3::Ipv4Address gateway;
};
struct Observer {
    const Configuration& config;
    Configuration active;
    std::function<void(std::size_t)> plan;
    explicit Observer(const Configuration& c) : config(c), active(c) {}
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
        if (kind == 5) {
            require(record.status == "pending", "availability loss of an already finished packet");
            record.status = "availability_drop"; record.finishedMs = nowMs();
        }
        if (kind == 4) record.observedPath.push_back(queue.from); // receiving-side direction
        {
            const bool receive = kind >= 4;
            result.hops.push_back({record.id, queue.link, receive ? queue.to : queue.from,
                receive ? queue.from : queue.to, kind == 0 ? "attempt" : (kind == 1 ? "enqueue" : (kind == 2 ? "tx" : (kind == 5 ? "availability_drop" : (receive ? "rx" : "drop")))),
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
        plan(id);
        if (record.status != "pending") return;
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
void traceIpDrop(Observer* observer, std::size_t node, const ns3::Ipv4Header& header,
                 ns3::Ptr<const ns3::Packet> packet, ns3::Ipv4L3Protocol::DropReason reason,
                 ns3::Ptr<ns3::Ipv4>, std::uint32_t) {
    ObservationTag tag; if (!packet->PeekPacketTag(tag)) return;
    auto& record = observer->result.packets.at(static_cast<std::size_t>(tag.id));
    require(record.status == "pending", "duplicate native IPv4 drop");
    record.status = "ip_drop"; record.finishedMs = nowMs();
    observer->result.ipDrops.push_back({record.id, observer->config.nodes.at(node).id, nowMs(), static_cast<unsigned>(reason), header.GetTtl()});
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
    require(c.events.size() <= 1000, "at most 1000 native availability events are supported");
    for (const auto& event : c.events)
        require(event.type == "node" || event.type == "link", "safety/cloud events are not implemented");
    require(c.nodes.size() <= 4096 && c.links.size() <= 16384 && c.traffic.size() <= 4096,
            "initial backend limits are 4096 nodes/flows and 16384 links");
    for (const auto& n : c.nodes) {
        require(n.access == "wired", n.id + ": Wi-Fi/NR access is not implemented");
        require(!n.energyConstrained, n.id + ": energy-constrained nodes are not implemented");
    }
    for (const auto& link : c.links) {
        const double bitsPerSecond = link.capacityMbps * 1e6;
        require(std::abs(bitsPerSecond - std::round(bitsPerSecond)) <= 1e-6,
                link.id + ": ns-3 requires a whole number of bits per second");
    }
    double offered = 0;
    for (const auto& flow : c.traffic) {
        require(flow.destinations.size() <= 16, flow.id + ": at most 16 service candidates are supported");
        for (const auto& target : flow.destinations)
            require(flow.source != target, flow.id + ": self-destination traffic is not implemented");
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
    Observer observer(c);
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
        require(nodes.Get(static_cast<unsigned>(i))->GetObject<ns3::Ipv4L3Protocol>()->TraceConnectWithoutContext(
            "Drop", ns3::MakeBoundCallback(&traceIpDrop, &observer, i)), "IPv4 drop trace unavailable");
    }
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> directions;
    std::map<std::pair<std::size_t, std::size_t>, const Link*> links;
    ns3::Ipv4AddressHelper addresses; addresses.SetBase("10.0.0.0", "255.255.255.252");
    for (std::size_t linkIndex = 0; linkIndex < c.links.size(); ++linkIndex) {
        const auto& link = c.links[linkIndex];
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
            auto gate = ns3::CreateObject<AvailabilityGate>();
            gate->available = [&observer, a, b, linkIndex]() {
                return observer.active.links[linkIndex].available && observer.active.nodes[a].available && observer.active.nodes[b].available;
            };
            device->SetReceiveErrorModel(gate);
            auto index = observer.directions.size();
            directions[{from, to}] = index; links[{from, to}] = &link;
            observer.directions.push_back({from, to, device, interfaces.GetAddress(1 - side)});
            observer.result.queues.push_back({link.id, c.nodes[from].id, c.nodes[to].id});
            auto ipv4 = nodes.Get(static_cast<unsigned>(from))->GetObject<ns3::Ipv4>();
            ipv4->SetForwarding(static_cast<unsigned>(ipv4->GetInterfaceForDevice(device)), c.nodes[from].role != "endpoint");
            for (const auto& event : std::vector<std::pair<std::string, unsigned>>{{"MacTx",0},{"PhyTxBegin",2},{"MacTxDrop",3},{"PhyRxEnd",4},{"PhyRxDrop",5}})
                require(device->TraceConnectWithoutContext(event.first, ns3::MakeBoundCallback(&tracePacket, &observer, index, event.second)), "trace unavailable: " + event.first);
            require(device->GetQueue()->TraceConnectWithoutContext("Enqueue", ns3::MakeBoundCallback(&tracePacket, &observer, index, 1U)), "queue trace unavailable");
        }
    }
    auto rebuild = [&]() {
        for (auto protocol : routing) protocol->routes.clear();
        for (std::size_t f = 0; f < c.traffic.size(); ++f) for (const auto& destination : c.traffic[f].destinations)
        for (std::size_t source = 0; source < c.nodes.size(); ++source) {
            const auto& flow = c.traffic[f]; const auto target = ids.at(destination);
            // Endpoints cannot be transit routers. Only this flow's originating
            // endpoint needs an output route; destination delivery is local.
            if (c.nodes[source].role == "endpoint" && source != ids.at(flow.source)) continue;
            const auto payload = flow.bytes;
            const auto key = flowKey(routing[ids.at(flow.source)]->address.Get(), routing[target]->address.Get(),
                                     static_cast<unsigned>(20000+f), static_cast<unsigned>(10000+f));
            if (source == target || !observer.active.nodes[source].available || !observer.active.nodes[target].available) continue;
            std::vector<RoutingNode> snapshot;
            for (std::size_t i = 0; i < c.nodes.size(); ++i)
                snapshot.push_back({static_cast<RobotId>(i), c.nodes[i].position, 100, 0, 1, RobotState::Idle, {}, false});
            for (const auto& [pair, link] : links) {
                const auto linkIndex = static_cast<std::size_t>(link - c.links.data());
                if (observer.active.links[linkIndex].available && observer.active.nodes[pair.first].available &&
                    observer.active.nodes[pair.second].available && (c.nodes[pair.first].role != "endpoint" || pair.first == source))
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
    };
    rebuild();
    // Pre-scheduling gives availability events precedence at equal timestamps.
    for (const auto& event : c.events) ns3::Simulator::Schedule(timeMs(event.timeMs), [&, event]() {
        if (event.type == "node") observer.active.nodes[ids.at(event.target)].available = event.available;
        else {
            auto link = std::find_if(observer.active.links.begin(), observer.active.links.end(), [&](const auto& item) { return item.id == event.target; });
            require(link != observer.active.links.end(), "unknown availability target"); link->available = event.available;
        }
        rebuild();
        observer.result.availability.push_back({event.type,event.target,nowMs(),event.available});
    });
    observer.plan = [&](std::size_t id) {
        auto& record = observer.result.packets.at(id); const auto f = record.flowIndex;
        const auto& flow = c.traffic[f]; const auto source = ids.at(flow.source);
        record.destination = flow.destinations.front(); record.expectedPath.clear();
        if (!observer.active.nodes[source].available) {
            record.status = "source_unavailable"; record.finishedMs = nowMs(); return;
        }
        for (const auto& destination : flow.destinations) {
            const auto target = ids.at(destination);
            const auto key = flowKey(routing[source]->address.Get(),routing[target]->address.Get(),static_cast<unsigned>(20000+f),static_cast<unsigned>(10000+f));
            if (!routing[source]->lookup(key, flow.bytes)) continue;
            record.destination = destination; observer.destinations[f] = routing[target]->address;
            record.expectedPath = {flow.source}; auto node = source;
            while (node != target) {
                auto route = routing[node]->lookup(key,flow.bytes);
                require(bool(route), "installed table has incomplete path");
                auto direction = std::find_if(observer.directions.begin(),observer.directions.end(),[&](const auto& d) { return d.device == route->GetOutputDevice(); });
                require(direction != observer.directions.end(), "route has unknown output device");
                node = direction->to; record.expectedPath.push_back(c.nodes[node].id);
                require(record.expectedPath.size() <= c.nodes.size(), "forwarding table contains a loop");
            }
            return;
        }
    };
    ns3::FlowMonitorHelper flowMonitor;
    flowMonitor.SetMonitorAttribute("MaxPerHopDelay", ns3::TimeValue(timeMs(c.durationMs + c.drainMs) + ns3::Seconds(1)));
    auto monitor = flowMonitor.InstallAll();
    for (std::size_t f = 0; f < c.traffic.size(); ++f) {
        const auto& flow = c.traffic[f]; auto source = ids.at(flow.source), target = ids.at(flow.destinations.front());
        for (const auto& destination : flow.destinations) {
            auto receiver = ns3::Socket::CreateSocket(nodes.Get(static_cast<unsigned>(ids.at(destination))), ns3::UdpSocketFactory::GetTypeId());
            require(receiver->Bind(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), static_cast<std::uint16_t>(10000 + f))) == 0, "UDP receiver bind failed");
            receiver->SetRecvCallback(ns3::MakeCallback(&Observer::receive, &observer)); observer.receivers.push_back(receiver);
        }
        auto sender = ns3::Socket::CreateSocket(nodes.Get(static_cast<unsigned>(source)), ns3::UdpSocketFactory::GetTypeId());
        require(sender->Bind(ns3::InetSocketAddress(routing[source]->address, static_cast<std::uint16_t>(20000+f))) == 0, "UDP sender bind failed");
        sender->SetIpTtl(255);
        const unsigned tos[] = {224,160,96,0}; sender->SetIpTos(static_cast<std::uint8_t>(tos[flow.priority]));
        observer.senders.push_back(sender); observer.destinations.push_back(routing[target]->address);
        for (std::uint64_t k = 0; ; ++k) {
            const double created = flow.startMs + static_cast<double>(k) * flow.intervalMs;
            if (created >= flow.stopMs) break;
            auto id = observer.result.packets.size();
            require(id < 1000000, "initial trace limit is one million offered packets");
            observer.result.packets.push_back({id, f, "pending", created, 0, {flow.source}, {}, flow.destinations.front()});
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
            require(!c.events.empty() || packet.observedPath == packet.expectedPath, "observed packet path differs from installed forwarding table");
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
        packets << p.id << ',' << f.id << ',' << f.source << ',' << p.destination << ',' << f.priority << ',' << f.bytes << ',' << p.status << ','
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
    std::ofstream availability(output / "availability.csv"), ipDrops(output / "ip_drops.csv");
    availability << std::setprecision(17) << "type,target,time_ms,available\n";
    for (const auto& e : r.availability) availability << e.type << ',' << e.target << ',' << e.timeMs << ',' << e.available << '\n';
    ipDrops << std::setprecision(17) << "packet,node,time_ms,reason,ttl\n";
    for (const auto& d : r.ipDrops) ipDrops << d.packet << ',' << d.node << ',' << d.timeMs << ',' << d.reason << ',' << d.ttl << '\n';
    availability.close(); ipDrops.close(); require(bool(availability) && bool(ipDrops), "could not write availability/IP drop observations");
    boost::json::object summary{{"model", "ns-3 stationary wired IPv4/UDP schema-v2 subset; not Ethernet switching or radio"},
        {"scheduler", c.scheduler}, {"routing", c.routing.algorithm == "dijkstra" ? "static size-aware Dijkstra; propagation plus payload/UDP/IPv4/PPP serialization" : "static five-tuple ECMP; propagation plus payload/UDP/IPv4/PPP serialization"},
        {"offered_packets", r.packets.size()}, {"delivered_packets", delivered}, {"deadline_missed_packets", missed},
        {"packet_statuses", statuses}, {"stationary", r.stationary}, {"cutoff_ms", c.durationMs + c.drainMs},
        {"ip_transmitted_packets", r.ipTransmitted}, {"ip_received_packets", r.ipReceived},
        {"ip_unreceived_at_cutoff", r.ipUnreceivedAtCutoff}, {"ip_received_bytes", r.ipReceivedBytes},
        {"byte_contract", "traffic.bytes = UDP payload; IPv4 bytes = payload+28; PPP wire bytes = payload+30"},
        {"loss_contract", "all offered packets in deadline denominator; cutoff is censored, not proven permanent loss"}};
    if (!c.events.empty()) {
        summary["routing"] = c.routing.algorithm == "dijkstra" ? "centrally rebuilt size-aware Dijkstra; immediate availability knowledge" : "centrally rebuilt transport ECMP; immediate availability knowledge";
        summary["availability_contract"] = "receive-time link/incident-node gate; queues continue; immediate centralized detection/table updates; events precede equal-time packet callbacks";
        summary["failure_detection_ms"] = 0;
    }
    std::ofstream report(output / "summary.json"); report << boost::json::serialize(summary) << '\n';
    packets.close(); hops.close(); queues.close(); nodes.close(); report.close();
    require(bool(packets) && bool(hops) && bool(queues) && bool(nodes) && bool(report), "could not write result files");
}
} // namespace ealr::fabric
