#include "ealr/config/fabric_configuration.hpp"
#include "ealr/safety/temperature_safety_controller.hpp"
#include <boost/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>

namespace ealr::fabric {
namespace {
namespace j = boost::json;
[[noreturn]] void fail(const std::string& path, const std::string& reason) {
    throw WarehouseConfigurationError(path + ": " + reason);
}
void require(bool valid, const std::string& path, const std::string& reason) { if(!valid) fail(path,reason); }
void keys(const j::object& o, std::initializer_list<const char*> names, const std::string& path) {
    for(const auto& entry:o) {
        bool found=false;
        for(auto name:names) if(entry.key()==name) found=true;
        require(found,path+"."+std::string(entry.key()),"unknown field");
    }
}
const j::value& value(const j::object& o, const char* key) {
    auto it=o.find(key); if(it==o.end()) fail(key,"required field missing"); return it->value();
}
const j::object& object(const j::value& v) { require(v.is_object(),"object","expected object"); return v.as_object(); }
const j::array& array(const j::value& v) { require(v.is_array(),"array","expected array"); return v.as_array(); }
std::string string(const j::value& v) { require(v.is_string(),"string","expected string"); return std::string(v.as_string()); }
double number(const j::value& v) {
    require(v.is_number(),"number","expected numeric value");
    double n=v.is_double()?v.as_double():(v.is_int64()?static_cast<double>(v.as_int64()):static_cast<double>(v.as_uint64()));
    require(std::isfinite(n),"number","must be finite"); return n;
}
double num(const j::object& o, const char* k, double d) { auto p=o.if_contains(k); return p?number(*p):d; }
unsigned integer(const j::object& o, const char* k, unsigned d) {
    double n=num(o,k,d); require(n>=0 && n<=1000000000 && std::floor(n)==n,k,"must be an integer in [0, 1000000000]");
    return static_cast<unsigned>(n);
}
bool boolean(const j::object& o,const char* k,bool d) {
    auto p=o.if_contains(k); if(!p)return d; require(p->is_bool(),k,"expected boolean"); return p->as_bool();
}
std::string str(const j::object& o,const char* k,const std::string& d) { auto p=o.if_contains(k);return p?string(*p):d; }
void finite(double n,const std::string& p,double lower=0) { require(std::isfinite(n)&&n>=lower,p,"must be finite and >= "+std::to_string(lower)); }
void positive(double n,const std::string& p) {finite(n,p);require(n>0,p,"must be positive");}
bool identifier(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.';});
}
}

Configuration load(const std::filesystem::path& path) {
    std::ifstream input(path); require(bool(input),path.string(),"cannot open configuration");
    std::string text((std::istreambuf_iterator<char>(input)),{});
    boost::system::error_code ec; auto document=j::parse(text,ec);
    require(!ec,path.string(),"invalid JSON: "+ec.message());
    const auto& root=object(document);
    keys(root,{"schemaVersion","name","topology","simulation","routing","deadlinesMs","safety","zones","nodes","links","traffic","events"},"root");
    require(number(value(root,"schemaVersion"))==2,"schemaVersion","expected 2 (stationary fabric)");
    Configuration c; c.name=string(value(root,"name"));
    const auto& topology=object(value(root,"topology"));
    keys(topology,{"kind","minimumSpinesPerLeaf","minimumEdges"},"topology");
    c.topology=string(value(topology,"kind")); c.minimumSpinesPerLeaf=integer(topology,"minimumSpinesPerLeaf",0); c.minimumEdges=integer(topology,"minimumEdges",0);
    const auto& sim=object(value(root,"simulation"));
    keys(sim,{"durationMs","drainMs","seed","scheduler","protectedQueues"},"simulation");
    c.durationMs=number(value(sim,"durationMs")); c.drainMs=num(sim,"drainMs",100); c.seed=integer(sim,"seed",1); c.scheduler=str(sim,"scheduler","strictPriority");
    if(auto p=sim.if_contains("protectedQueues")) {
        require(c.scheduler=="protectedPriority","simulation.protectedQueues","requires protectedPriority scheduler");
        const auto& q=object(*p);keys(q,{"reservedEmergencyPackets","emergencyBurstBytes","emergencyRateFraction","emergencyPolicingEnabled","lowerClassWeights"},"simulation.protectedQueues");
        c.protectedQueues.emergencyPolicingEnabled=boolean(q,"emergencyPolicingEnabled",true);
        c.protectedQueues.reservedEmergencyPackets=integer(q,"reservedEmergencyPackets",8);
        c.protectedQueues.emergencyBurstBytes=integer(q,"emergencyBurstBytes",512);
        c.protectedQueues.emergencyRateFraction=num(q,"emergencyRateFraction",0.2);
        if(auto weights=q.if_contains("lowerClassWeights")) {
            const auto& values=array(*weights);require(values.size()==3,"lowerClassWeights","requires P1/P2/P3 weights");
            for(unsigned i=0;i<3;++i) {
                double weight=number(values[i]);require(weight>=1&&weight<=1024&&std::floor(weight)==weight,"lowerClassWeights","integer weights in [1,1024] required");
                c.protectedQueues.lowerClassWeights[i]=static_cast<unsigned>(weight);
            }
        }
    }
    if(auto p=root.if_contains("routing")) {
        const auto& r=object(*p); keys(r,{"algorithm","alpha","beta","gamma","latencyScaleMs","maxStretch"},"routing");
        c.routing={str(r,"algorithm","dijkstra"),num(r,"alpha",1),num(r,"beta",1),num(r,"gamma",1),num(r,"latencyScaleMs",1),num(r,"maxStretch",1.5)};
    }
    if(auto p=root.if_contains("deadlinesMs")) {
        const auto& a=array(*p); require(a.size()==4,"deadlinesMs","four priority deadlines required");
        for(unsigned i=0;i<4;++i)c.deadlinesMs[i]=number(a[i]);
    }
    if(auto p=root.if_contains("safety")) {
        const auto& s=object(*p);keys(s,{"warningCelsius","stopCelsius","broadcastEmergencyStop","processAtLocalEdge","requireManualResetAfterStop","processingMs"},"safety");
        c.safety={num(s,"warningCelsius",70),num(s,"stopCelsius",80),boolean(s,"broadcastEmergencyStop",true),boolean(s,"processAtLocalEdge",true),boolean(s,"requireManualResetAfterStop",true)};
        c.safetyProcessingMs=num(s,"processingMs",0.1);
    }
    for(const auto& z:array(value(root,"zones")))c.zones.push_back(string(z));
    for(const auto& v:array(value(root,"nodes"))) {
        const auto& o=object(v); keys(o,{"id","role","zone","access","x","y","available","energy"},"node");
        Node n; n.id=string(value(o,"id")); n.role=string(value(o,"role")); n.zone=str(o,"zone",""); n.access=str(o,"access","wired");
        n.position={number(value(o,"x")),number(value(o,"y"))}; n.available=boolean(o,"available",true);
        if(auto p=o.if_contains("energy")) {
            const auto& e=object(*p);keys(e,{"batteryPercent","criticalPercent","txPercentPerByte","rxPercentPerByte"},n.id+".energy");
            n.energyConstrained=true; n.batteryPercent=number(value(e,"batteryPercent")); n.criticalPercent=num(e,"criticalPercent",10);
            n.txPercentPerByte=num(e,"txPercentPerByte",0);n.rxPercentPerByte=num(e,"rxPercentPerByte",0);
        }
        c.nodes.push_back(n);
    }
    for(const auto& v:array(value(root,"links"))) {
        const auto& o=object(v);keys(o,{"id","from","to","capacityMbps","delayMs","queuePackets","available"},"link");
        c.links.push_back({string(value(o,"id")),string(value(o,"from")),string(value(o,"to")),number(value(o,"capacityMbps")),number(value(o,"delayMs")),integer(o,"queuePackets",100),boolean(o,"available",true)});
    }
    for(const auto& v:array(value(root,"traffic"))) {
        const auto& o=object(v); keys(o,{"id","source","destinations","priority","bytes","startMs","stopMs","intervalMs","phaseJitterMs"},"traffic");
        Workload w; w.id=string(value(o,"id")); w.source=string(value(o,"source"));
        for(const auto& d:array(value(o,"destinations")))w.destinations.push_back(string(d));
        value(o,"priority"); w.priority=integer(o,"priority",2); value(o,"bytes");w.bytes=integer(o,"bytes",512);
        w.startMs=num(o,"startMs",0); w.stopMs=number(value(o,"stopMs"));w.intervalMs=number(value(o,"intervalMs"));w.phaseJitterMs=num(o,"phaseJitterMs",0);
        c.traffic.push_back(w);
    }
    if(auto p=root.if_contains("events")) for(const auto& v:array(*p)) {
        const auto& o=object(v);keys(o,{"type","target","zone","timeMs","temperatureCelsius","available"},"event");
        Event e{string(value(o,"type")),str(o,"target",""),str(o,"zone",""),number(value(o,"timeMs")),num(o,"temperatureCelsius",20),boolean(o,"available",false)};
        if(e.type=="node"||e.type=="link"||e.type=="cloud")value(o,"available");
        if(e.type=="temperature"||e.type=="reset")value(o,"temperatureCelsius");
        c.events.push_back(e);
    }
    validate(c); return c;
}

void validate(const Configuration& c) {
    require(!c.name.empty(),"name","must not be empty");
    require(c.topology=="graph"||c.topology=="leafSpine","topology.kind","supported: graph, leafSpine");
    require(c.scheduler=="strictPriority"||c.scheduler=="fifo"||c.scheduler=="protectedPriority","simulation.scheduler","supported: strictPriority, fifo, protectedPriority");
    if(c.scheduler=="protectedPriority") {
        const auto& q=c.protectedQueues;
        require(std::isfinite(q.emergencyRateFraction)&&q.emergencyRateFraction>0&&q.emergencyRateFraction<1,"protectedQueues.emergencyRateFraction","must be strictly between 0 and 1");
        require(q.emergencyBurstBytes>0&&q.emergencyBurstBytes<=1000000000,"protectedQueues.emergencyBurstBytes","expected 1..1000000000");
        for(auto weight:q.lowerClassWeights)require(weight>=1&&weight<=1024,"protectedQueues.lowerClassWeights","expected 1..1024");
        require(c.routing.algorithm!="ealr","routing.algorithm","EALR queue estimator does not support protectedPriority scheduling");
    }
    positive(c.durationMs,"simulation.durationMs"); finite(c.drainMs,"simulation.drainMs");positive(c.durationMs+c.drainMs,"simulation.end");
    finite(c.safetyProcessingMs,"safety.processingMs"); TemperatureSafetyController controller(c.safety);
    require(c.routing.algorithm=="dijkstra"||c.routing.algorithm=="ecmp"||c.routing.algorithm=="greedy"||c.routing.algorithm=="ealr","routing.algorithm","supported: dijkstra, ecmp, greedy, ealr");
    positive(c.routing.alpha,"routing.alpha");finite(c.routing.beta,"routing.beta");finite(c.routing.gamma,"routing.gamma");positive(c.routing.latencyScaleMs,"routing.latencyScaleMs");finite(c.routing.maxStretch,"routing.maxStretch",1);
    for(unsigned i=0;i<4;++i){positive(c.deadlinesMs[i],"deadlinesMs");if(i)require(c.deadlinesMs[i]>=c.deadlinesMs[i-1],"deadlinesMs","must be ascending");}
    std::set<std::string> zones;
    for(const auto& z:c.zones)require(identifier(z)&&zones.insert(z).second,"zones."+z,"invalid or duplicate ID");
    require(!zones.empty(),"zones","at least one zone required");
    std::map<std::string,const Node*> nodes;
    unsigned edges=0;
    for(const auto& n:c.nodes) {
        require(identifier(n.id)&&nodes.emplace(n.id,&n).second,"nodes."+n.id,"invalid or duplicate ID");
        require(n.role=="endpoint"||n.role=="leaf"||n.role=="spine"||n.role=="edge"||n.role=="access"||n.role=="relay",n.id+".role","unsupported role");
        require(n.access=="wired"||n.access=="wifi"||n.access=="private5g",n.id+".access","unsupported access technology");
        require(n.zone.empty()||zones.count(n.zone),n.id+".zone","unknown zone");
        if(n.role=="endpoint"||n.role=="leaf"||n.role=="access")require(zones.count(n.zone),n.id+".zone","zone membership required");
        require(std::isfinite(n.position.x)&&std::isfinite(n.position.y),n.id+".position","must be finite");
        require(std::abs(n.position.x)<=1e9&&std::abs(n.position.y)<=1e9,n.id+".position","coordinates exceed supported +/-1e9 metres");
        if(n.energyConstrained) {
            require(n.role=="endpoint"||n.role=="relay",n.id+".energy","battery only applies to endpoints or explicit constrained relays");
            finite(n.batteryPercent,n.id+".batteryPercent");finite(n.criticalPercent,n.id+".criticalPercent");
            require(n.batteryPercent<=100&&n.criticalPercent<100,n.id+".energy","invalid percentage");
            finite(n.txPercentPerByte,n.id+".txPercentPerByte");finite(n.rxPercentPerByte,n.id+".rxPercentPerByte");
        }
        if(n.role=="edge")++edges;
    }
    require(!nodes.empty(),"nodes","must not be empty");
    require(edges>=c.minimumEdges,"topology.minimumEdges","not enough edge nodes");
    std::set<std::string> linkIds; std::set<std::pair<std::string,std::string>> pairs;
    std::map<std::string,std::vector<std::string>> adjacency;
    for(const auto& l:c.links) {
        require(identifier(l.id)&&linkIds.insert(l.id).second,"links."+l.id,"invalid or duplicate ID");
        require(nodes.count(l.from)&&nodes.count(l.to)&&l.from!=l.to,l.id,"invalid link endpoints");
        require(pairs.insert(std::minmax(l.from,l.to)).second,l.id,"parallel links unsupported; use distinct intermediary nodes");
        positive(l.capacityMbps,l.id+".capacityMbps");finite(l.delayMs,l.id+".delayMs");require(l.queuePackets>0,l.id+".queuePackets","must be positive");
        if(c.scheduler=="protectedPriority")require(c.protectedQueues.reservedEmergencyPackets<l.queuePackets,l.id+".queuePackets","must exceed reservedEmergencyPackets");
        // Bound extreme inputs so serialisation/arrival arithmetic remains representable.
        require(l.capacityMbps>=1e-6&&l.capacityMbps<=1e9&&l.delayMs<=1e9,l.id,"link parameters outside supported numerical range");
        adjacency[l.from].push_back(l.to);adjacency[l.to].push_back(l.from);
        if(c.topology=="leafSpine") {
            auto a=nodes.at(l.from), b=nodes.at(l.to);
            auto rolePair=std::minmax(a->role,b->role);
            bool ok=(rolePair.first=="leaf"&&rolePair.second=="spine") || (rolePair.first=="edge"&&rolePair.second=="spine");
            auto accessLink=[](const Node& endpoint,const Node& uplink) {
                if(endpoint.zone!=uplink.zone)return false;
                if(endpoint.role=="access"&&uplink.role=="leaf")return true;
                if(endpoint.role!="endpoint")return false;
                return (uplink.role=="leaf"&&endpoint.access=="wired") ||
                       (uplink.role=="access"&&endpoint.access==uplink.access);
            };
            ok=ok || accessLink(*a,*b) || accessLink(*b,*a);
            require(ok,l.id,"link contradicts leafSpine role/zone constraints");
        }
    }
    for(const auto& n:c.nodes) {
        if(n.role=="leaf") {
            unsigned count=0;for(const auto& id:adjacency[n.id])if(nodes.at(id)->role=="spine")++count;
            require(count>=c.minimumSpinesPerLeaf,n.id,"insufficient configured spine uplinks");
        }
        if(c.topology=="leafSpine"&&n.role=="endpoint") {
            std::set<std::string> visited{n.id};std::queue<std::string> q;q.push(n.id);bool reachable=false;
            while(!q.empty()) {auto u=q.front();q.pop(); if(nodes.at(u)->role=="edge")reachable=true;
                for(const auto& v:adjacency[u])if(nodes.at(v)->role!="endpoint"&&visited.insert(v).second)q.push(v);}
            require(reachable,n.id,"mandatory edge infrastructure structurally unreachable");
        }
    }
    std::set<std::string> flowIds;
    for(const auto& w:c.traffic) {
        require(identifier(w.id)&&flowIds.insert(w.id).second,"traffic."+w.id,"invalid or duplicate ID");
        require(nodes.count(w.source),w.id+".source","unknown node");require(!w.destinations.empty(),w.id+".destinations","must not be empty");
        std::set<std::string> unique;for(const auto& d:w.destinations)require(nodes.count(d)&&unique.insert(d).second,w.id+".destinations","unknown or duplicate destination");
        require(w.priority<4,w.id+".priority","expected 0..3");require(w.bytes>0&&w.bytes<=1000000,w.id+".bytes","expected 1..1000000");
        if(c.scheduler=="protectedPriority"&&c.protectedQueues.emergencyPolicingEnabled&&w.priority==0)require(w.bytes<=c.protectedQueues.emergencyBurstBytes,w.id+".bytes","P0 packet exceeds emergencyBurstBytes");
        finite(w.startMs,w.id+".startMs");positive(w.stopMs,w.id+".stopMs");positive(w.intervalMs,w.id+".intervalMs");finite(w.phaseJitterMs,w.id+".phaseJitterMs");
        require(w.startMs<w.stopMs&&w.stopMs<=c.durationMs,w.id,"require 0 <= start < stop <= duration");
        require(w.phaseJitterMs<w.intervalMs,w.id+".phaseJitterMs","must be less than interval");
        require(w.intervalMs>=1e-6 && (w.stopMs-w.startMs)/w.intervalMs<=10000000,w.id,"workload exceeds 10 million packets or resolution bound");
    }
    for(const auto& e:c.events) {
        finite(e.timeMs,"event.timeMs");require(e.timeMs<c.durationMs,"event.timeMs","must precede duration");
        if(e.type=="node")require(nodes.count(e.target),"event.target","unknown node "+e.target);
        else if(e.type=="link")require(linkIds.count(e.target),"event.target","unknown link "+e.target);
        else if(e.type=="temperature"||e.type=="reset") {
            if(c.scheduler=="protectedPriority"&&c.protectedQueues.emergencyPolicingEnabled&&c.safety.broadcastEmergencyStop)require(c.protectedQueues.emergencyBurstBytes>=64,"protectedQueues.emergencyBurstBytes","safety fan-out requires at least 64 bytes");
            require(zones.count(e.zone),"event.zone","unknown zone");require(nodes.count(e.target)&&nodes.at(e.target)->role=="edge","event.target","safety processing target must be a local edge node");
            require(std::isfinite(e.temperatureCelsius),"event.temperatureCelsius","must be finite");
        } else require(e.type=="cloud","event.type","unsupported event type");
    }
}
} // namespace ealr::fabric
