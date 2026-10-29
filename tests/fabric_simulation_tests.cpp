#include "ealr/simulation/fabric_simulator.hpp"
#include "ealr/routing/dijkstra_routing.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
using namespace ealr::fabric;
int main() {
    int failures=0;
    auto check=[&](bool ok,const char* s){if(!ok){++failures;std::cerr<<"FAIL: "<<s<<'\n';}};
    auto sample=load(std::filesystem::path(EALR_SOURCE_DIRECTORY)/"configs/warehouse_small.json");
    for(const std::string algorithm:{"dijkstra","greedy","ealr"}) {
        auto c=sample;c.routing.algorithm=algorithm;
        auto r=simulate(c);check(r.stationary,"stationary invariant");
        check(r.packets.size()==36,"deterministic traffic count");
        check(std::all_of(r.packets.begin(),r.packets.end(),[](const auto& p){return p.status=="delivered";}),"all baseline packets delivered");
        check(r.packets[0].path==std::vector<std::string>({"source","fast","edge0"}),"packets actually traverse selected route");
        check(std::abs(r.packets[0].finishedMs-r.packets[0].createdMs-1.2)<1e-9,"two-hop analytical serialization plus propagation");
        auto again=simulate(c);check(r.packets.back().finishedMs==again.packets.back().finishedMs,"repeatability");
        c.links.clear();auto broken=simulate(c);check(broken.packets[0].status=="no_route","disconnected graph loss");
        c=sample;c.routing.algorithm=algorithm;c.traffic.resize(1);c.traffic[0].destinations={"source"};
        auto local=simulate(c);check(local.packets[0].path.size()==1&&local.packets[0].finishedMs==0,"source equals destination");
        c=sample;c.routing.algorithm=algorithm;c.nodes[0].available=false;
        check(simulate(c).packets[0].status=="node_unavailable","unavailable source");
        c=sample;c.routing.algorithm=algorithm;c.events={{"node","fast","",20,20,false},{"node","fast","",60,20,true}};
        auto failed=simulate(c);bool alternate=false,recovered=false;
        for(const auto& p:failed.packets)if(p.flow=="p0"&&p.status=="delivered") {
            if(p.createdMs>=20&&p.createdMs<60)alternate=alternate||p.path[1]=="alternate";
            if(p.createdMs>=60)recovered=recovered||p.path[1]=="fast";
        }
        check(alternate&&recovered,"node failover and recovery carry traffic");
        c=sample;c.routing.algorithm=algorithm;c.events={{"link","source--fast","",20,20,false}};
        auto linkFailed=simulate(c);check(linkFailed.packets[8].path[1]=="alternate","link failure reroutes later packet");
        c=sample;c.routing.algorithm=algorithm;c.events={{"node","edge0","",20,20,false}};
        auto edgeFailed=simulate(c);bool backup=false;
        for(const auto& p:edgeFailed.packets)if(p.createdMs>=20&&p.status=="delivered"&&p.destination=="edge1")backup=true;
        check(backup,"primary edge failure uses secondary");
    }
    auto c=sample;c.traffic.resize(1);c.traffic[0].stopMs=1;c.events={{"link","source--fast","",0.1,20,false},{"link","source--fast","",0.2,20,true}};
    check(simulate(c).packets[0].status=="in_flight_failure","in-flight loss survives fast failure recovery");
    c=sample;c.traffic.clear();c.links={{"bottleneck","source","edge0",1,0,100,true}};
    c.traffic={{"active","source",{"edge0"},3,1000,0,1,10,0},
               {"waiting","source",{"edge0"},3,1000,1,2,10,0},
               {"safety","source",{"edge0"},0,100,2,3,10,0}};
    auto priority=simulate(c);
    check(std::abs(priority.packets[2].finishedMs-8.8)<1e-9,"safety overtakes waiting background but cannot preempt frame");
    check(priority.packets[2].finishedMs<priority.packets[1].finishedMs,"real priority service order");
    c.scheduler="fifo";auto fifo=simulate(c);check(std::abs(fifo.packets[2].finishedMs-16.8)<1e-9,"FIFO ablation service order");
    c.scheduler="strictPriority";c.links[0].queuePackets=1;check(simulate(c).packets[2].status=="queue_overflow","finite queue loss");
    c=sample;c.nodes[1].energyConstrained=true;c.nodes[1].batteryPercent=11;c.nodes[1].criticalPercent=10;c.nodes[1].txPercentPerByte=0.004;
    auto energy=simulate(c);check(energy.firstEnergyExclusionMs>=0,"energy exclusion recorded");
    check(energy.packets[4].path[1]=="alternate","depleted relay removed from later paths");
    c=sample;c.routing.algorithm="ealr";c.nodes[1].energyConstrained=true;c.nodes[1].batteryPercent=11;c.routing.beta=2;
    check(simulate(c).packets[0].path[1]=="alternate","EALR avoids low battery with equal latency");
    c.routing.beta=0;check(simulate(c).packets[0].path[1]=="fast","energy ablation restores baseline");
    c.routing.beta=2;c.links[2].delayMs=30;c.links[3].delayMs=30;
    auto guarded=simulate(c);check(guarded.packets[0].path[1]=="fast","latency cost prevents energy detour");
    c=sample;c.events={{"cloud","","",0,20,false},{"temperature","edge0","A",10,80,false},{"temperature","edge0","A",20,50,false},{"reset","edge0","A",30,70,false},{"reset","edge0","A",40,60,false}};
    auto safe=simulate(c);check(safe.safetyEvents.size()==4,"safety events logged");
    check(safe.safetyEvents[0].state=="emergencyStop"&&safe.safetyEvents[1].state=="emergencyStop","latched safety independent of cloud");
    check(safe.safetyEvents[2].state=="reset_rejected"&&safe.safetyEvents[3].state=="reset","manual reset boundaries");
    check(std::abs(safe.safetyEvents[0].responseMs-safe.safetyEvents[0].readingMs-0.1)<1e-9,"measured local processing response");
    bool deliveredStop=false;for(const auto& p:safe.packets)if(p.flow=="safety-broadcast"&&p.priority==0&&p.status=="delivered")deliveredStop=true;
    check(deliveredStop,"safety broadcast follows packet forwarding");
    c=sample;c.events={{"node","edge0","",0,20,false},{"temperature","edge0","A",1,80,false}};
    check(!simulate(c).safetyEvents[0].controllerAvailable,"local controller failure reported honestly");
    c=sample;c.traffic.clear();check(simulate(c).packets.empty(),"zero traffic supported");
    c=sample;c.durationMs=1;c.drainMs=0;c.traffic.resize(1);c.traffic[0].stopMs=1;
    check(simulate(c).packets[0].status=="cutoff","undelivered packets at cutoff counted");
    ealr::RoutingTopology graph({{0,{0,0},100,0,1,ealr::RobotState::Idle,{1}},{1,{1,0},100,0,1,ealr::RobotState::Idle,{}}});
    for(double value:{-1.0,std::numeric_limits<double>::infinity()}) {
        try{ealr::DijkstraRouting().computeRoute({graph,0,1,[value](auto&,auto&){return value;}});check(false,"invalid explicit edge cost rejected");}catch(const std::invalid_argument&){}
    }
    std::cout<<"simulation failures: "<<failures<<'\n';return failures?1:0;
}
