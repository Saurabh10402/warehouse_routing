#include "ealr/routing/ecmp_routing.hpp"
#include "ealr/simulation/fabric_simulator.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>

int main() {
    using namespace ealr;
    int failures=0;
    auto check=[&](bool ok,const char* label){if(!ok){++failures;std::cerr<<label<<'\n';}};
    std::vector<RoutingNode> nodes;
    for(RobotId id=0;id<5;++id)nodes.push_back({id,{static_cast<double>(id),0},100,0,1,RobotState::Idle,{}});
    nodes[0].neighbours={1,2,3};nodes[1].neighbours={4};nodes[2].neighbours={4};nodes[3].neighbours={4};
    RoutingTopology topology(nodes);
    auto costs=[](const RoutingNode& a,const RoutingNode&){return a.id==3?5.0:1.0;};
    std::set<RobotId> selected;
    for(std::uint64_t flow=0;flow<64;++flow) {
        auto route=EcmpRouting(flow).computeRoute({topology,0,4,costs});
        check(route.found()&&route.hops.size()==3&&route.totalCost==2,"equal minimum-cost directed path");
        selected.insert(route.hops.at(1));
        check(route.hops==EcmpRouting(flow).computeRoute({topology,0,4,costs}).hops,"flow selection reproducible");
    }
    check(selected==std::set<RobotId>({1,2}),"multiple flows use both equal paths, never expensive path");
    check(!EcmpRouting(0).computeRoute({topology,4,0,costs}).found(),"directed unreachable return path");
    check(EcmpRouting(0).computeRoute({topology,0,0,costs}).hops==std::vector<RobotId>{0},"same source/destination");
    check(EcmpRouting(0).computeRoute({topology,9,4,costs}).status==RouteStatus::InvalidSource,"invalid source");
    nodes[1].state=RobotState::ReturningToCharge;RoutingTopology failed(nodes);
    check(EcmpRouting(1).computeRoute({failed,0,4,costs}).hops==std::vector<RobotId>({0,2,4}),"failed next hop excluded");
    try {EcmpRouting(0).computeRoute({topology,0,4,[](auto&,auto&){return 0.0;}});check(false,"zero cost rejected");}
    catch(const std::invalid_argument&) {}

    auto config=fabric::load(std::filesystem::path(EALR_SOURCE_DIRECTORY)/"configs/warehouse_communications.json");
    check(config.routing.algorithm=="ecmp","JSON accepts ECMP");
    for(const std::string algorithm:{"dijkstra","ecmp"}) {
        auto c=config;c.routing.algorithm=algorithm;
        auto normal=fabric::simulate(c);
        std::map<std::string,std::vector<std::string>> paths;
        for(const auto& p:normal.packets) {
            check(p.status=="delivered","baseline communication delivered");
            if(paths.count(p.flow))check(paths[p.flow]==p.path,"each flow keeps its path under stable topology");
            paths[p.flow]=p.path;
            if(p.flow.back()=='0')check(p.path.size()==3,"same-zone traffic uses local switch only");
            if(p.flow.back()=='1')check(p.path.size()==5,"cross-zone traffic uses two leaves and one spine");
        }
        c.events={{"node","spine0","",300,20,false},{"node","spine1","",300,20,false}};
        auto outage=fabric::simulate(c);
        for(const auto& p:outage.packets)if(p.createdMs>=300) {
            if(p.flow.back()=='0')check(p.status=="delivered","same-zone forwarding survives complete spine outage");
            else check(p.status=="no_route","cross-zone and edge traffic fail with both spines down");
        }
        c.events={{"node","spine0","",300,20,false}};
        for(const auto& p:fabric::simulate(c).packets)if(p.createdMs>=300) {
            check(p.status=="delivered","single spine failover delivers");
            check(std::find(p.path.begin(),p.path.end(),"spine0")==p.path.end(),"failed spine absent from path");
        }
        c.events={{"node","edge0","",300,20,false}};
        for(const auto& p:fabric::simulate(c).packets)if(p.createdMs>=300&&p.flow.back()=='2')
            check(p.status=="delivered"&&p.destination=="edge1","service failover to secondary edge");
    }
    std::cout<<"ECMP/communications failures: "<<failures<<'\n';return failures?1:0;
}
