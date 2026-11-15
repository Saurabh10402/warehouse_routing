// SPDX-License-Identifier: GPL-2.0-only
// Small static routed-fabric control; equal-cost ties use deterministic BFS.
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <queue>

struct FabricFlow
{
    std::string source, destination;
    Ptr<UdpServer> sink;
};

static void ArpDrop(std::string node, Ptr<const Packet> packet)
{
    uint8_t header[20];
    if (packet->GetSize()<20) return;
    packet->CopyData(header,20);
    const uint32_t ihl=(header[0]&15)*4;
    const uint32_t total=(uint32_t(header[2])<<8)|header[3];
    if ((header[0]>>4)!=4 || ihl<20 || total<ihl+8 || total>packet->GetSize()
        || packet->GetSize()<ihl+8 || header[9]!=17) return;
    auto copy=packet->Copy();
    Ipv4Header ip;
    copy->RemoveHeader(ip);
    if (ip.GetProtocol()!=17) return;
    UdpHeader udp;
    copy->RemoveHeader(udp);
    SeqTsHeader sequence;
    if (copy->GetSize()<sequence.GetSerializedSize()) return;
    copy->RemoveHeader(sequence);
    std::cout << "ARP_DROP node=" << node << " time=" << Simulator::Now().GetSeconds()
              << " src=" << ip.GetSource() << " dst=" << ip.GetDestination()
              << " port=" << udp.GetDestinationPort() << " seq=" << sequence.GetSeq() << '\n';
}

static std::vector<FabricFlow>
InstallFabric(std::map<std::string, Entry>& entries, Ptr<NrPointToPointEpcHelper> epc)
{
    struct Arc { std::string to; Ipv4Address next; uint32_t interface; };
    std::map<std::string, Ptr<Node>> routers;
    std::map<std::string, std::vector<Arc>> graph;
    std::map<std::string, Ipv4Address> addresses;
    std::map<std::string, std::string> owners;
    std::vector<std::string> spines, robots, edges;
    Ipv4StaticRoutingHelper routing;
    for (auto& [id,e] : entries)
    {
        if (e.role=="spine" || e.role=="switch" || e.role=="ap" || e.role=="edge") routers[id]=e.node;
        if (e.role=="spine") spines.push_back(id);
        if (e.role=="edge") edges.push_back(id);
        if (e.role=="robot") robots.push_back(id);
    }
    NS_ABORT_MSG_IF(spines.empty() || edges.empty(), "Fabric control requires spine and edge server");
    routers["core-pgw"]=epc->GetPgwNode();
    PointToPointHelper wire;
    wire.SetDeviceAttribute("DataRate",StringValue("1Gbps"));
    wire.SetChannelAttribute("Delay",TimeValue(MicroSeconds(100)));
    Ipv4AddressHelper subnet;
    subnet.SetBase("10.10.0.0","255.255.255.252");
    auto link=[&](const std::string& a,const std::string& b) {
        auto dev=wire.Install(routers.at(a),routers.at(b));
        auto ip=subnet.Assign(dev); subnet.NewNetwork();
        auto av=routers.at(a)->GetObject<Ipv4>();
        auto bv=routers.at(b)->GetObject<Ipv4>();
        graph[a].push_back({b,ip.GetAddress(1),static_cast<uint32_t>(av->GetInterfaceForDevice(dev.Get(0)))});
        graph[b].push_back({a,ip.GetAddress(0),static_cast<uint32_t>(bv->GetInterfaceForDevice(dev.Get(1)))});
        if (!addresses.count(a)) addresses[a]=ip.GetAddress(0);
        if (!addresses.count(b)) addresses[b]=ip.GetAddress(1);
    };
    for (const auto& [id,e] : entries)
        if (e.role=="switch") for (const auto& spine : spines) link(id,spine);
    for (const auto& spine : spines) link("core-pgw",spine);
    for (const auto& [id,e] : entries)
        if (e.role=="ap" || e.role=="edge")
        {
            auto prefix=id.substr(0,id.rfind("-"+e.role+"-"));
            std::string leaf;
            for (const auto& [other,candidate] : entries)
                if (candidate.role=="switch" && other.substr(0,other.rfind("-switch-"))==prefix) {leaf=other;break;}
            NS_ABORT_MSG_IF(leaf.empty(),"Missing zone leaf");
            link(id,leaf);
        }
    subnet.SetBase("192.168.0.0","255.255.255.0");
    for (auto& [id,e] : entries)
        if (e.role=="ap")
        {
            auto apIp=subnet.Assign(NetDeviceContainer(e.radio));
            uint32_t stations=0;
            for (auto& [name,robot] : entries)
                if (robot.access=="wifi" && robot.attachment==id)
                {
                    NS_ABORT_MSG_IF(++stations>253,"Wi-Fi /24 station limit exceeded");
                    auto ip=subnet.Assign(NetDeviceContainer(robot.radio));
                    addresses[name]=ip.GetAddress(0); owners[name]=id;
                    auto ipv4=robot.node->GetObject<Ipv4>();
                    routing.GetStaticRouting(ipv4)->SetDefaultRoute(apIp.GetAddress(0),ipv4->GetInterfaceForDevice(robot.radio));
                }
            subnet.NewNetwork();
        }
    for (const auto& id : robots)
        if (entries.at(id).access=="nr")
        {
            const auto& e=entries.at(id);
            auto ip=e.node->GetObject<Ipv4>();
            addresses[id]=ip->GetAddress(ip->GetInterfaceForDevice(e.radio),0).GetLocal();
            owners[id]="core-pgw";
        }
    for (const auto& id : edges) owners[id]=id;
    for (const auto& [id,node] : routers)
        node->GetObject<ArpL3Protocol>()->TraceConnectWithoutContext("Drop",MakeBoundCallback(&ArpDrop,id));
    // Route each endpoint through a connected fabric owner. On the owner retain
    // the directly connected WLAN or EPC UE route installed by the helpers.
    for (const auto& [destination,owner] : owners)
        for (const auto& [start,node] : routers)
        {
            if (start==owner) continue;
            std::map<std::string,std::string> previous;
            std::queue<std::string> queue;
            queue.push(start); previous[start]=start;
            while (!queue.empty() && !previous.count(owner))
            {
                auto current=queue.front(); queue.pop();
                for (const auto& arc : graph[current]) if (!previous.count(arc.to)) {previous[arc.to]=current;queue.push(arc.to);}
            }
            NS_ABORT_MSG_IF(!previous.count(owner),"Disconnected fabric");
            auto next=owner;
            while (previous.at(next)!=start) next=previous.at(next);
            for (const auto& arc : graph[start]) if (arc.to==next)
                routing.GetStaticRouting(node->GetObject<Ipv4>())->AddHostRouteTo(addresses.at(destination),arc.next,arc.interface);
        }
    std::vector<FabricFlow> flows;
    uint16_t port=9000;
    for (size_t i=0;i<robots.size();++i)
        for (const auto& destination : {edges[i%edges.size()], robots[(i+1)%robots.size()]})
        {
            NS_ABORT_MSG_IF(port==65535,"Too many flows");
            UdpServerHelper server(port);
            auto sink=server.Install(entries.at(destination).node);
            sink.Start(Seconds(0)); sink.Stop(Seconds(3));
            UdpClientHelper client(addresses.at(destination),port++);
            client.SetAttribute("MaxPackets",UintegerValue(10));
            client.SetAttribute("Interval",TimeValue(MilliSeconds(20)));
            client.SetAttribute("PacketSize",UintegerValue(256));
            auto app=client.Install(entries.at(robots[i]).node);
            app.Start(Seconds(1.2+i*0.001)); app.Stop(Seconds(2.5));
            flows.push_back({robots[i],destination,DynamicCast<UdpServer>(sink.Get(0))});
        }
    return flows;
}
