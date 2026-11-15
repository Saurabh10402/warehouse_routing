// SPDX-License-Identifier: GPL-2.0-only
// Routed fabric with immediate centralized outage knowledge; deterministic BFS.
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <queue>
#include <cmath>
#include "access-ecmp.h"

// JSON timings may be fractional milliseconds; do not use the integer MilliSeconds overload.
static Time ControlMs(double ms) { return NanoSeconds(static_cast<int64_t>(std::llround(ms*1e6))); }

struct FabricFlow
{
    std::string source, destination;
    Ptr<UdpServer> sink;
};

static void ArpDrop(std::string node, Ptr<const Packet> packet)
{
    // ArpL3Protocol::Drop also reports received ARP replies after removing their
    // ARP header. Those packets have no IPv4 header (often no bytes at all).
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
    std::cout << "ARP_DROP node=" << node << " time=" << Simulator::Now().GetSeconds()
              << " src=" << ip.GetSource() << " dst=" << ip.GetDestination()
              << " port=" << udp.GetDestinationPort() << " payloadBytes=" << copy->GetSize() << '\n';
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

// IPv4 receive-side evidence distinguishes observed interface loss from cutoff.
static void FabricDrop(std::string node, const Ipv4Header& ip, Ptr<const Packet> packet,
                       Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t interface)
{
    if(ip.GetProtocol()!=17 || packet->GetSize()<8)return;
    auto copy=packet->Copy();UdpHeader udp;copy->RemoveHeader(udp);
    std::cout << "IP_DROP " << node << " " << ip.GetSource() << " " << ip.GetDestination()
              << " " << udp.GetSourcePort() << " " << udp.GetDestinationPort()
              << " " << ip.GetIdentification() << " " << Simulator::Now().GetNanoSeconds()
              << " " << (reason==Ipv4L3Protocol::DROP_INTERFACE_DOWN?"interface-down":"other")
              << " " << interface << '\n';
}

static std::map<std::string, Ipv4Address>
InstallTaskFabric(std::map<std::string, Entry>& entries, Ptr<NrNoBackhaulEpcHelper> epc, const std::string& linksFile, Ptr<Node> server, double upperDelayMs, const std::string& outageSpine, double downMs, double upMs, const std::string& policy)
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
    routers["main-server-model"]=server;
    // Stable router identities belong to loopback, not either spine-facing link.
    // Physical /30 addresses below remain next hops only. Robot radio addresses
    // and the helper-managed EPC interfaces are unchanged.
    uint32_t identity=0x0afe0001; // 10.254.0.1, disjoint from radio/EPC/link ranges
    for (const auto& [id,node] : routers)
    {
        auto ipv4=node->GetObject<Ipv4>();
        Ipv4Address address(identity++);
        NS_ABORT_MSG_IF(!ipv4->AddAddress(0,Ipv4InterfaceAddress(address,Ipv4Mask("255.255.255.255"))),
                        "Cannot install stable fabric identity");
        addresses[id]=address;
        std::cout << "IDENTITY " << id << " " << address << '\n';
    }
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
    wire.SetChannelAttribute("Delay",TimeValue(ControlMs(upperDelayMs)));
    for (const auto& spine : spines) link("main-server-model",spine);
    owners["main-server-model"]="main-server-model";
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
    {
        node->GetObject<ArpL3Protocol>()->TraceConnectWithoutContext("Drop",MakeBoundCallback(&ArpDrop,id));
        node->GetObject<Ipv4L3Protocol>()->TraceConnectWithoutContext("Drop",MakeBoundCallback(&FabricDrop,id));
    }
    std::map<std::string,Ptr<AccessEcmp>> multipath;
    NS_ABORT_MSG_IF(policy!="shortest-path" && policy!="ecmp","Unsupported routing policy");
    if(policy=="ecmp")
    {
        for(const auto& [id,node]:routers)
        {
            auto ipv4=node->GetObject<Ipv4>();
            auto list=DynamicCast<Ipv4ListRouting>(ipv4->GetRoutingProtocol());
            NS_ABORT_MSG_IF(!list,"Expected list routing to preserve EPC/local routes");
            auto protocol=CreateObject<AccessEcmp>();protocol->name=id;protocol->identity=addresses.at(id);
            list->AddRoutingProtocol(protocol,10);multipath[id]=protocol;
        }
        for(const auto& [id,owner]:owners)
            std::cout<<"ECMP_DEST "<<id<<" "<<addresses.at(id)<<" "<<owner<<'\n';
    }
    // Keep a value-owned graph for scheduled rebuilds, never references to locals.
    // Remove only our generated gateway host routes; retain connected, default,
    // WLAN and helper-installed EPC subnet routes. Interface-down notifications
    // can already have removed entries, so never retain mutable route indices.
    auto rebuild=[routers,graph,addresses,owners,multipath](const std::string& failed) {
        Ipv4StaticRoutingHelper helper;
        uint32_t installed=0;
        for(const auto& [id,protocol]:multipath)protocol->routes.clear();
        for(const auto& [id,node]:routers)
        {
            auto table=helper.GetStaticRouting(node->GetObject<Ipv4>());
            for(uint32_t index=table->GetNRoutes();index>0;--index)
            {
                auto route=table->GetRoute(index-1);
                if(!route.IsHost() || route.GetGateway()==Ipv4Address::GetAny())continue;
                for(const auto& [destination,address]:addresses)
                    if(route.GetDest()==address) {table->RemoveRoute(index-1);break;}
            }
        }
        for(const auto& [destination,owner]:owners)
            for(const auto& [start,node]:routers)
            {
                if(start==owner || start==failed || owner==failed)continue;
                if(!multipath.empty())
                {
                    // Evaluate each first hop using a separate shortest-hop search.
                    // Core/server nodes can be endpoints, never transit shortcuts.
                    std::vector<AccessEcmp::Hop> equal;
                    uint32_t best=UINT32_MAX;
                    for(const auto& first:graph.at(start))
                    {
                        if(first.to==failed || (first.to!=owner && (first.to.rfind("core-",0)==0 || first.to=="main-server-model")))continue;
                        std::map<std::string,uint32_t> distance{{start,0},{first.to,1}};
                        std::queue<std::string> queue;queue.push(first.to);
                        while(!queue.empty() && !distance.count(owner))
                        {
                            auto current=queue.front();queue.pop();
                            for(const auto& arc:graph.at(current))
                                if(arc.to!=failed && !distance.count(arc.to) &&
                                   (arc.to==owner || (arc.to.rfind("core-",0)!=0 && arc.to!="main-server-model")))
                                {distance[arc.to]=distance.at(current)+1;queue.push(arc.to);}
                        }
                        if(!distance.count(owner))continue;
                        uint32_t cost=distance.at(owner);
                        if(cost<best){equal.clear();best=cost;}
                        if(cost==best)equal.push_back({first.to,first.next,first.interface});
                    }
                    NS_ABORT_MSG_IF(equal.empty(),"Disconnected ECMP fabric");
                    std::sort(equal.begin(),equal.end(),[](const auto& a,const auto& b){return a.node<b.node;});
                    multipath.at(start)->routes[addresses.at(destination)]=equal;
                    installed+=equal.size();
                    continue;
                }
                std::map<std::string,std::string> previous;
                std::queue<std::string> queue;
                queue.push(start);previous[start]=start;
                while(!queue.empty() && !previous.count(owner))
                {
                    auto current=queue.front();queue.pop();
                    for(const auto& arc:graph.at(current))
                        if(arc.to!=failed && !previous.count(arc.to) &&
                           (arc.to==owner || (arc.to.rfind("core-",0)!=0 && arc.to!="main-server-model")))
                        {previous[arc.to]=current;queue.push(arc.to);}
                }
                NS_ABORT_MSG_IF(!previous.count(owner),"Disconnected surviving fabric");
                auto next=owner;
                while(previous.at(next)!=start)next=previous.at(next);
                for(const auto& arc:graph.at(start))if(arc.to==next)
                {
                    helper.GetStaticRouting(node->GetObject<Ipv4>())->AddHostRouteTo(addresses.at(destination),arc.next,arc.interface);
                    ++installed;
                }
            }
        std::cout << "ROUTE_REBUILD " << (failed.empty()?"none":failed) << " "
                  << Simulator::Now().GetNanoSeconds() << " " << installed << '\n';
    };
    rebuild("");
    if(outageSpine!="none")
    {
        NS_ABORT_MSG_IF(std::find(spines.begin(),spines.end(),outageSpine)==spines.end() ||
                        downMs<1000 || upMs<=downMs || upMs>60000,"Invalid spine outage");
        // Both ends are administratively down. Packets already in flight can
        // finish physical transmission, but IPv4 drops arrivals on down ports.
        // Rebuild is instantaneous; this is not a routing-protocol convergence model.
        auto change=[routers,graph,outageSpine,rebuild](bool up) {
            uint32_t interfaces=0;
            for(const auto& [from,arcs]:graph)
                for(const auto& arc:arcs)if(from==outageSpine || arc.to==outageSpine)
                {
                    auto ip=routers.at(from)->GetObject<Ipv4>();
                    if(up)ip->SetUp(arc.interface);else ip->SetDown(arc.interface);
                    ++interfaces;
                }
            rebuild(up?"":outageSpine);
            std::cout << "SPINE_EVENT " << outageSpine << " " << (up?"up":"down")
                      << " " << Simulator::Now().GetNanoSeconds() << " " << interfaces << '\n';
        };
        Simulator::Schedule(ControlMs(downMs),[change](){change(false);});
        Simulator::Schedule(ControlMs(upMs),[change](){change(true);});
    }
    for (const auto& [id,e] : entries) if (e.role=="gnb")
        epc->AddS1Interface(e.node,addresses.at(id),addresses.at("core-sgw"),DynamicCast<NrGnbNetDevice>(e.radio)->GetCellIds());
    return addresses;
}
