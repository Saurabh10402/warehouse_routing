#include "ealr/metrics/fabric_metrics.hpp"
#include <iostream>
#include <cmath>
using namespace ealr::fabric;
int main() {
    Configuration c;c.durationMs=1000;c.drainMs=0;c.deadlinesMs={10,20,40,100};
    SimulationResult r;
    int failures=0;auto check=[&](bool ok){if(!ok)++failures;};
    auto zero=summarize(c,r);check(zero.sent==0&&zero.deliveryPercent==0&&zero.throughputMbps==0&&!zero.p95Ms);
    for(unsigned i=1;i<=100;++i)r.packets.push_back({i,"f","a","b","delivered",0,100,0,static_cast<double>(i),{"a","b"}});
    auto m=summarize(c,r);check(m.sent==100&&m.delivered==100&&m.lost==0);check(m.meanMs==50.5&&m.medianMs==50.5);
    check(m.p95Ms&&*m.p95Ms==95&&m.p99Ms&&*m.p99Ms==99);check(m.deadlineMisses==91);check(std::abs(m.throughputMbps-0.08)<1e-12);
    r.packets.push_back({101,"f","a","b","no_route",0,100,0,0,{"a"}});
    m=summarize(c,r);check(m.sent==101&&m.lost==1&&m.deadlineMisses==92);check(summarize(c,r,1).sent==0);
    r.packets.resize(19);check(!summarize(c,r).p95Ms);r.packets[0].status="pending";
    try{summarize(c,r);++failures;}catch(const std::invalid_argument&){}
    std::cout<<"metrics failures: "<<failures<<'\n';return failures?1:0;
}
