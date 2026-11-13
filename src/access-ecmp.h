// SPDX-License-Identifier: GPL-2.0-only
// Deterministic Layer-3 ECMP; no UDP parsing during RouteOutput (header may be absent).
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"

class AccessEcmp : public Ipv4RoutingProtocol
{
public:
    struct Hop {std::string node;Ipv4Address gateway;uint32_t interface;};
    std::string name;
    Ipv4Address identity;
    std::map<Ipv4Address,std::vector<Hop>> routes;
    static TypeId GetTypeId()
    {
        static TypeId tid=TypeId("ns3::AccessEcmp").SetParent<Ipv4RoutingProtocol>().AddConstructor<AccessEcmp>();
        return tid;
    }
    static uint32_t Hash(const std::string& node,Ipv4Address source,Ipv4Address destination,uint8_t protocol)
    {
        uint32_t h=2166136261u;
        auto byte=[&](uint8_t value){h=(h^value)*16777619u;};
        for(unsigned char c:node)byte(c);
        byte(0);
        for(uint32_t value:{source.Get(),destination.Get()})
            for(int shift=24;shift>=0;shift-=8)byte((value>>shift)&255);
        byte(protocol);
        h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
        return h;
    }
    Ptr<Ipv4Route> Select(const Ipv4Header& header,Ptr<NetDevice> output)
    {
        auto found=routes.find(header.GetDestination());
        if(found==routes.end())return nullptr;
        std::vector<Hop> choices;
        for(const auto& hop:found->second)
            if(ipv4->IsUp(hop.interface) && (!output || ipv4->GetNetDevice(hop.interface)==output))choices.push_back(hop);
        if(choices.empty())return nullptr;
        // UDP RouteOutput may supply a default-constructed (uninitialized)
        // source, not just 0.0.0.0. Never serialize ns-3's 102.102.102.102 sentinel.
        auto source=header.GetSource();
        if(!source.IsInitialized() || source.IsAny())source=identity;
        const auto& hop=choices[Hash(name,source,header.GetDestination(),header.GetProtocol())%choices.size()];
        auto route=Create<Ipv4Route>();route->SetSource(source);route->SetDestination(header.GetDestination());
        route->SetGateway(hop.gateway);route->SetOutputDevice(ipv4->GetNetDevice(hop.interface));
        return route;
    }
    Ptr<Ipv4Route> RouteOutput(Ptr<Packet>,const Ipv4Header& header,Ptr<NetDevice> output,Socket::SocketErrno& error) override
    {
        auto route=Select(header,output);error=route?Socket::ERROR_NOTERROR:Socket::ERROR_NOROUTETOHOST;return route;
    }
    bool RouteInput(Ptr<const Packet> packet,const Ipv4Header& header,Ptr<const NetDevice> input,
                    const UnicastForwardCallback& forward,const MulticastForwardCallback&,const LocalDeliverCallback&,const ErrorCallback&) override
    {
        auto interface=ipv4->GetInterfaceForDevice(input);
        if(interface<0 || ipv4->IsDestinationAddress(header.GetDestination(),interface) || !ipv4->IsForwarding(interface))return false;
        auto route=Select(header,nullptr);
        if(!route)return false;
        forward(route,packet,header);return true;
    }
    void SetIpv4(Ptr<Ipv4> value) override {ipv4=value;}
    void NotifyInterfaceUp(uint32_t) override {}
    void NotifyInterfaceDown(uint32_t) override {}
    void NotifyAddAddress(uint32_t,Ipv4InterfaceAddress) override {}
    void NotifyRemoveAddress(uint32_t,Ipv4InterfaceAddress) override {}
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,Time::Unit =Time::S) const override
    {*stream->GetStream()<<"AccessEcmp "<<name<<" destinations="<<routes.size()<<'\n';}
protected:
    void DoDispose() override {routes.clear();ipv4=nullptr;Ipv4RoutingProtocol::DoDispose();}
private:
    Ptr<Ipv4> ipv4;
};
