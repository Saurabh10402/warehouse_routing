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

static void FabricTx(std::string from, std::string to, Ptr<const Packet> packet)
{
    auto copy=packet->Copy(); PppHeader ppp; copy->RemoveHeader(ppp);
    if (ppp.GetProtocol()!=0x0021) return;
    Ipv4Header ip; copy->RemoveHeader(ip);
    if (ip.GetProtocol()!=17) return;
    UdpHeader udp; copy->RemoveHeader(udp);
    std::cout << "WIRE " << from << " " << to << " " << ip.GetSource() << " "
              << ip.GetDestination() << " " << udp.GetDestinationPort() << " "
              << ip.GetIdentification() << " " << Simulator::Now().GetNanoSeconds() << '\n';
}

static std::vector<FabricFlow>
InstallFabric(std::map<std::string, Entry>& entries, Ptr<NrNoBackhaulEpcHelper> epc, const std::string& linksFile)
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
        if (e.role=="spine" || e.role=="switch" || e.role=="ap" || e.role=="gnb") routers[id]=e.node;
        if (e.role=="spine") spines.push_back(id);
        if (e.role=="edge") edges.push_back(id);
        if (e.role=="robot") robots.push_back(id);
    }
    NS_ABORT_MSG_IF(spines.empty(), "Fabric control requires spine");
    routers["core-pgw"]=epc->GetPgwNode();
    routers["core-sgw"]=epc->GetSgwNode();
    PointToPointHelper wire;
    wire.SetDeviceAttribute("DataRate",StringValue("1Gbps"));
    wire.SetChannelAttribute("Delay",TimeValue(MicroSeconds(100)));
    Ipv4AddressHelper subnet;
    subnet.SetBase("10.10.0.0","255.255.255.252");
    auto link=[&](const std::string& a,const std::string& b) {
        auto dev=wire.Install(routers.at(a),routers.at(b));
        dev.Get(0)->TraceConnectWithoutContext("PhyTxBegin",MakeBoundCallback(&FabricTx,a,b));
        dev.Get(1)->TraceConnectWithoutContext("PhyTxBegin",MakeBoundCallback(&FabricTx,b,a));
        std::cout << "LINK " << a << " " << b << '\n';
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
    for (const auto& spine : spines) {link("core-pgw",spine); link("core-sgw",spine);}
    for (const auto& [id,e] : entries)
        if (e.role=="ap" || e.role=="gnb")
        {
            auto prefix=id.substr(0,id.rfind("-"+e.role+"-"));
            std::string leaf;
            for (const auto& [other,candidate] : entries)
                if (candidate.role=="switch" && other.substr(0,other.rfind("-switch-"))==prefix) {leaf=other;break;}
            NS_ABORT_MSG_IF(leaf.empty(),"Missing zone leaf");
            link(id,leaf);
        }
    std::ifstream zoneLinks(linksFile);
    NS_ABORT_MSG_IF(!zoneLinks,"Cannot read direct zone links");
    std::string zoneA,zoneB;
    while (zoneLinks >> zoneA >> zoneB) link("zone-"+zoneA+"-switch-0","zone-"+zoneB+"-switch-0");
    NS_ABORT_MSG_IF(!zoneLinks.eof(),"Invalid zone link table");
    for (const auto& [id,node] : routers) owners[id]=id;
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
                for (const auto& arc : graph[current]) if (!previous.count(arc.to) && (arc.to==owner || arc.to.rfind("core-",0)!=0)) {previous[arc.to]=current;queue.push(arc.to);}
            }
            NS_ABORT_MSG_IF(!previous.count(owner),"Disconnected fabric");
            auto next=owner;
            while (previous.at(next)!=start) next=previous.at(next);
            for (const auto& arc : graph[start]) if (arc.to==next)
                routing.GetStaticRouting(node->GetObject<Ipv4>())->AddHostRouteTo(addresses.at(destination),arc.next,arc.interface);
        }
    for (const auto& [id,e] : entries) if (e.role=="gnb")
        epc->AddS1Interface(e.node,addresses.at(id),addresses.at("core-sgw"),DynamicCast<NrGnbNetDevice>(e.radio)->GetCellIds());
    std::vector<FabricFlow> flows;
    uint16_t port=9000;
    std::vector<std::pair<std::string,std::string>> probes;
    for (size_t i=0;i<robots.size();++i)
        probes.emplace_back(robots[i],robots[(i+1)%robots.size()]);
    // Explicit cross-zone Wi-Fi probe, in addition to the existing peer ring.
    for (const auto& a : robots)
    {
        if (entries.at(a).access!="wifi") continue;
        for (const auto& b : robots)
            if (entries.at(b).access=="wifi" && entries.at(a).attachment!=entries.at(b).attachment)
            {probes.emplace_back(a,b);break;}
        break;
    }
    for (size_t i=0;i<probes.size();++i)
        {
            const auto& source=probes[i].first;
            const auto& destination=probes[i].second;
            NS_ABORT_MSG_IF(port==65535,"Too many flows");
            UdpServerHelper server(port);
            auto sink=server.Install(entries.at(destination).node);
            sink.Start(Seconds(0)); sink.Stop(Seconds(3));
            UdpClientHelper client(addresses.at(destination),port++);
            client.SetAttribute("MaxPackets",UintegerValue(10));
            client.SetAttribute("Interval",TimeValue(MilliSeconds(20)));
            client.SetAttribute("PacketSize",UintegerValue(256));
            auto app=client.Install(entries.at(source).node);
            app.Start(Seconds(1.2+i*0.001)); app.Stop(Seconds(2.5));
            flows.push_back({source,destination,DynamicCast<UdpServer>(sink.Get(0))});
        }
    return flows;
}
