#include "ealr/config/fabric_configuration.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <map>
using namespace ealr::fabric;
int main() {
    int failures=0;
    auto check=[&](bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';++failures;}};
    auto root=std::filesystem::path(EALR_SOURCE_DIRECTORY);
    auto c=load(root/"configs/warehouse_small.json");
    auto reject=[&](std::function<void(Configuration&)> mutate){auto bad=c;mutate(bad);try{validate(bad);++failures;std::cerr<<"invalid config accepted\n";}catch(const std::exception&) {}};
    auto ref=load(root/"configs/warehouse_reference.json");
    unsigned endpoints=0;for(const auto& n:ref.nodes)if(n.role=="endpoint")++endpoints;
    check(endpoints==500,"reference has 500 endpoints");
    auto uneven=load(root/"configs/warehouse_uneven.json");std::map<std::string,unsigned> counts;
    for(const auto& n:uneven.nodes)if(n.role=="endpoint")++counts[n.zone];
    check(counts["zone0"]==10&&counts["zone1"]==35&&counts["zone2"]==75&&counts["zone3"]==20,"unequal zones retained");
    reject([](auto& x){x.nodes.push_back(x.nodes[0]);});
    reject([](auto& x){x.zones.push_back("A");});
    reject([](auto& x){x.nodes[0].zone="missing";});
    reject([](auto& x){x.nodes[0].access="bluetooth";});
    reject([](auto& x){x.links[0].to="missing";});
    reject([](auto& x){x.links[0].capacityMbps=0;});
    reject([](auto& x){x.links[0].delayMs=-1;});
    reject([](auto& x){x.links.push_back(x.links[0]);});
    reject([](auto& x){x.traffic[0].priority=4;});
    reject([](auto& x){x.traffic[0].intervalMs=0;});
    reject([](auto& x){x.traffic[0].destinations={"missing"};});
    reject([](auto& x){x.safety.stopCelsius=60;});
    reject([](auto& x){x.routing.beta=std::numeric_limits<double>::quiet_NaN();});
    reject([](auto& x){x.routing.gamma=-1;});
    reject([](auto& x){x.nodes[3].energyConstrained=true;});
    reject([](auto& x){x.events.push_back({"link","unknown","",1,20,false});});
    reject([](auto& x){x.scheduler="bogus";});
    reject([](auto& x){x.topology="leafSpine";});
    auto disconnected=ref;disconnected.links.clear();try{validate(disconnected);++failures;}catch(const std::exception&){}
    auto mismatched=ref;for(auto& n:mismatched.nodes)if(n.role=="endpoint"&&n.access=="private5g"){n.access="wifi";break;}
    try{validate(mismatched);++failures;}catch(const std::exception&){}
    // Explicit graph supports disconnected topology for loss experiments.
    c.links.clear();validate(c);
    std::cout<<"configuration failures: "<<failures<<'\n';return failures?1:0;
}
