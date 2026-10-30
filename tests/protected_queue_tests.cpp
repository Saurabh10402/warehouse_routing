#include "ealr/simulation/fabric_simulator.hpp"
#include "ealr/metrics/fabric_metrics.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>

using namespace ealr::fabric;
int main() {
    int failures=0;
    auto check=[&](bool ok,const char* message){if(!ok){++failures;std::cerr<<message<<'\n';}};
    auto fixture=load(std::filesystem::path(EALR_SOURCE_DIRECTORY)/"configs/warehouse_small.json");
    fixture.scheduler="protectedPriority";fixture.protectedQueues.reservedEmergencyPackets=1;
    fixture.links={{"single","source","edge0",1,0,1000,true}};fixture.traffic.clear();
    auto conservation=[&](const SimulationResult& result){for(const auto& q:result.queues){const auto& s=q.statistics;
        check(s.attempts==s.admitted+s.overflowDrops+s.reservationDrops+s.policedDrops,"per-queue admission conservation");
        check(s.admitted==s.serviceStarts+s.rerouted+s.queuedAtCutoff,"per-queue occupancy conservation");
        check(s.totalWaitMs>=0&&s.maxWaitMs>=0,"nonnegative measured queue waiting");
    }};
    auto c=fixture;c.links[0].queuePackets=3;
    c.traffic={{"active","source",{"edge0"},3,1000,0,0.1,10,0},
               {"bg1","source",{"edge0"},3,1000,1,1.01,10,0},
               {"bg2","source",{"edge0"},3,1000,1.1,1.11,10,0},
               {"bg3","source",{"edge0"},3,1000,1.2,1.21,10,0},
               {"emergency","source",{"edge0"},0,100,2,2.01,10,0}};
    auto protectedResult=simulate(c);conservation(protectedResult);
    check(protectedResult.packets[3].status=="reserved_capacity","background cannot consume emergency reservation");
    check(protectedResult.packets[4].status=="delivered"&&std::abs(protectedResult.packets[4].finishedMs-8.8)<1e-9,"reserved P0 waits for current frame then transmits");
    c.protectedQueues.emergencyPolicingEnabled=false;
    c.protectedQueues.emergencyBurstBytes=1;
    auto unpolicedReserved=simulate(c);conservation(unpolicedReserved);
    check(unpolicedReserved.packets[3].status=="reserved_capacity"&&unpolicedReserved.packets[4].status=="delivered","disabled policing keeps reservation and permits payload above inactive burst");
    c.scheduler="strictPriority";check(simulate(c).packets[4].status=="queue_overflow","unchanged baseline drops P0 at shared-full buffer");

    c=fixture;c.protectedQueues.emergencyBurstBytes=100;
    c.traffic={{"policed","source",{"edge0"},0,100,0,5,1,0},
               {"reverse","edge0",{"source"},0,100,0,0.1,10,0}};
    auto policed=simulate(c);conservation(policed);unsigned accepted=0,rejected=0;
    for(const auto& p:policed.packets)if(p.flow=="policed") {
        if(p.status=="delivered"){++accepted;check(p.createdMs==0||p.createdMs==4,"exact token refill boundary");}
        if(p.status=="priority_policed")++rejected;
    }else check(p.status=="delivered","reverse direction has independent token bucket");
    check(accepted==2&&rejected==3,"100-byte burst and 25-byte/ms token rate");
    auto metrics=summarize(c,policed,0);check(metrics.sent==6&&metrics.deadlineMisses==3,"policed packets remain in deadline denominator");
    c.traffic={{"initial","source",{"edge0"},0,100,0,0.1,10,0},
               {"after_idle","source",{"edge0"},0,100,50,50.1,10,0},
               {"excess","source",{"edge0"},0,100,50,50.1,10,0}};
    auto idle=simulate(c);check(idle.packets[1].status=="delivered"&&idle.packets[2].status=="priority_policed","idle refill caps at burst size");

    c=fixture;c.durationMs=56;c.drainMs=0;
    c.traffic={{"p1","source",{"edge0"},1,100,0,1,0.01,0},
               {"p2","source",{"edge0"},2,200,0,1,0.01,0},
               {"p3","source",{"edge0"},3,400,0,1,0.01,0}};
    auto fair=simulate(c);conservation(fair);std::array<double,3> normalized{};
    for(const auto& q:fair.queues)if(q.from=="source"&&q.priority>0) {
        check(q.statistics.bytesStarted>=800,"all admitted backlogged classes receive service");
        normalized[q.priority-1]=static_cast<double>(q.statistics.bytesStarted)/c.protectedQueues.lowerClassWeights[q.priority-1];
    }
    check(*std::max_element(normalized.begin(),normalized.end())-*std::min_element(normalized.begin(),normalized.end())<=400,"weighted service is byte-fair despite unequal packet sizes");
    c.scheduler="strictPriority";auto strict=simulate(c);
    for(const auto& q:strict.queues)if(q.from=="source"&&q.priority==3)check(q.statistics.serviceStarts==0,"baseline starvation is exposed in same backlog");

    c=fixture;c.durationMs=40;c.drainMs=0;c.protectedQueues.emergencyBurstBytes=100;
    c.traffic={{"flood","source",{"edge0"},0,100,0,40,0.05,0},
               {"background","source",{"edge0"},3,100,0,40,0.5,0}};
    auto overload=simulate(c);conservation(overload);
    check(std::any_of(overload.packets.begin(),overload.packets.end(),[](const auto& p){return p.status=="priority_policed";}),"overload admission failures visible");
    for(const auto& q:overload.queues)if(q.from=="source") {
        if(q.priority==0)check(q.statistics.admitted*100<=1100,"token admission bound burst plus rate times duration");
        if(q.priority==3)check(q.statistics.serviceStarts>0,"P0 flood cannot starve admitted background forever");
    }
    c.protectedQueues.emergencyPolicingEnabled=false;
    auto unpoliced=simulate(c);conservation(unpoliced);
    check(std::none_of(unpoliced.packets.begin(),unpoliced.packets.end(),[](const auto& p){return p.status=="priority_policed";}),"disabled policing never rejects for tokens");
    for(const auto& q:unpoliced.queues)if(q.from=="source"&&q.priority==3)
        check(q.statistics.admitted>0&&q.statistics.serviceStarts==0,"unpoliced P0 flood can starve admitted lower traffic");
    c=fixture;c.links[0].delayMs=20;c.traffic={{"failed","source",{"edge0"},2,1000,0,10,1,0}};
    c.events={{"link","single","",2,20,false},{"link","single","",6,20,true}};
    conservation(simulate(c));
    auto reject=[&](std::function<void(Configuration&)> change){auto bad=fixture;change(bad);try{validate(bad);check(false,"invalid protected queue config accepted");}catch(const std::exception&) {}};
    reject([](auto& x){x.protectedQueues.emergencyRateFraction=0;});
    reject([](auto& x){x.protectedQueues.emergencyRateFraction=1;});
    reject([](auto& x){x.protectedQueues.emergencyRateFraction=std::numeric_limits<double>::quiet_NaN();});
    reject([](auto& x){x.protectedQueues.emergencyBurstBytes=0;});
    reject([](auto& x){x.protectedQueues.lowerClassWeights[2]=0;});
    reject([](auto& x){x.protectedQueues.reservedEmergencyPackets=x.links[0].queuePackets;});
    reject([](auto& x){x.routing.algorithm="ealr";});
    reject([](auto& x){x.traffic={{"large","source",{"edge0"},0,1000,0,1,10,0}};});
    std::cout<<"protected queue failures: "<<failures<<'\n';return failures?1:0;
}
