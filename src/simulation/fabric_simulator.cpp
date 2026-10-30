#include "ealr/simulation/fabric_simulator.hpp"
#include "ealr/routing/dijkstra_routing.hpp"
#include "ealr/routing/ecmp_routing.hpp"
#include "ealr/routing/ealr_routing.hpp"
#include "ealr/routing/greedy_routing.hpp"
#include "ealr/safety/temperature_safety_controller.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <functional>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <tuple>

namespace ealr::fabric {
namespace {
struct Scheduled {
    double time;
    int order;
    std::uint64_t sequence;
    std::function<void()> action;
    bool operator>(const Scheduled& other) const {return std::tie(time,order,sequence)>std::tie(other.time,other.order,other.sequence);}
};
struct PendingPacket {std::vector<std::string> destinations; std::size_t node; std::uint64_t queueOrder=0;};
struct Direction {
    std::size_t link, from, to;
    std::array<std::deque<std::size_t>,4> queues;
    bool busy=false;
    double busyUntil=0;
};

class Engine {
    const Configuration& c;
    SimulationResult result;
    std::vector<Node> nodes;
    std::vector<bool> linkUp;
    std::vector<unsigned> linkEpoch, nodeEpoch;
    std::map<std::string,std::size_t> nodeIds, linkIds;
    std::vector<Direction> directions;
    std::map<std::pair<std::size_t,std::size_t>,std::size_t> directionIds;
    std::vector<PendingPacket> pending;
    std::priority_queue<Scheduled,std::vector<Scheduled>,std::greater<Scheduled>> calendar;
    TemperatureSafetyController safety;
    double now=0;
    std::uint64_t sequence=0;

    void schedule(double time,int order,std::function<void()> action) {
        if(!std::isfinite(time)||time<now)throw std::runtime_error("Invalid simulation event time");
        calendar.push({time,order,sequence++,std::move(action)});
    }
    bool eligible(std::size_t n) const {const auto& x=nodes[n];return x.available&&(!x.energyConstrained||x.batteryPercent>x.criticalPercent);}
    bool usable(const Direction& d) const {return linkUp[d.link]&&eligible(d.from)&&eligible(d.to);}
    double serialization(const Direction& d,unsigned bytes) const {return static_cast<double>(bytes)*8/(c.links[d.link].capacityMbps*1000);}
    double baseDelay(const Direction& d,unsigned bytes) const {return c.links[d.link].delayMs+serialization(d,bytes);}
    double queuedDelay(const Direction& d,unsigned priority) const {
        double delay=std::max(0.0,d.busyUntil-now);
        unsigned last=c.scheduler=="fifo"?3:priority;
        for(unsigned p=0;p<=last;++p)for(auto packet:d.queues[p])delay+=serialization(d,result.packets[packet].bytes);
        return delay;
    }
    void finish(std::size_t id,const std::string& status) {
        auto& p=result.packets[id];if(p.status!="pending")return;
        p.status=status;p.finishedMs=now;
    }
    void consume(std::size_t n,double percent) {
        if(!nodes[n].energyConstrained)return;
        bool before=eligible(n);nodes[n].batteryPercent=std::max(0.0,nodes[n].batteryPercent-percent);
        if(before&&!eligible(n)) {
            if(result.firstEnergyExclusionMs<0)result.firstEnergyExclusionMs=now;
            result.events.push_back({now,"energy",nodes[n].id,false});
        }
    }
    RouteResult route(std::size_t packet,std::size_t target) {
        const auto& p=result.packets[packet];auto source=pending[packet].node;
        std::set<std::string> visited(p.path.begin(),p.path.end());visited.erase(nodes[source].id);
        std::vector<RoutingNode> snapshot;
        for(std::size_t i=0;i<nodes.size();++i) {
            const auto& n=nodes[i];bool up=eligible(i)&&!visited.count(n.id);
            snapshot.push_back({static_cast<RobotId>(i),n.position,n.batteryPercent,0,1,up?RobotState::Idle:RobotState::ReturningToCharge,{},n.energyConstrained});
        }
        for(const auto& d:directions)if(usable(d)&&(nodes[d.from].role!="endpoint"||d.from==source))
            snapshot[d.from].neighbours.push_back(static_cast<RobotId>(d.to));
        for(auto& n:snapshot)std::sort(n.neighbours.begin(),n.neighbours.end());
        RoutingTopology topology(std::move(snapshot));
        auto base=[&](const RoutingNode& a,const RoutingNode& b){return baseDelay(directions[directionIds.at({a.id,b.id})],p.bytes);};
        auto estimated=[&](const RouteResult& r) {
            double t=0;
            for(std::size_t i=1;i<r.hops.size();++i){const auto& d=directions[directionIds.at({r.hops[i-1],r.hops[i]})];t+=baseDelay(d,p.bytes)+queuedDelay(d,p.priority);}
            return t;
        };
        RoutingRequest request{topology,static_cast<RobotId>(source),static_cast<RobotId>(target),base};
        ++result.routeComputations;
        if(c.routing.algorithm=="greedy")return GreedyRouting().computeRoute(request);
        if(c.routing.algorithm=="dijkstra")return DijkstraRouting().computeRoute(request);
        if(c.routing.algorithm=="ecmp") {
            // Stable flow identity + source + chosen destination + seed. Packet ID/time
            // and queue occupancy deliberately do not change the flow's selected path.
            std::uint64_t hash=UINT64_C(14695981039346656037);
            for(const auto& field:{p.flow,p.source,nodes[target].id,std::to_string(c.seed)}) {
                for(unsigned char byte:field){hash^=byte;hash*=UINT64_C(1099511628211);}
                hash^=0;hash*=UINT64_C(1099511628211);
            }
            return EcmpRouting(hash).computeRoute(request);
        }
        auto shortest=DijkstraRouting().computeRoute(request);
        request.linkCost=[&](const RoutingNode& a,const RoutingNode& b) {
            const auto& d=directions[directionIds.at({a.id,b.id})];
            double energy=nodes[b.id].energyConstrained?(100-nodes[b.id].batteryPercent)/100:0;
            return c.routing.alpha*baseDelay(d,p.bytes)/c.routing.latencyScaleMs+c.routing.beta*energy+
                   c.routing.gamma*queuedDelay(d,p.priority)/c.routing.latencyScaleMs;
        };
        auto selected=EalrRouting({1,0,0}).computeRoute(request);
        if(selected.found()&&shortest.found()) {
            double remaining=c.deadlinesMs[p.priority]-(now-p.createdMs);
            double baseline=estimated(shortest), candidate=estimated(selected);
            if(candidate>baseline*c.routing.maxStretch || (candidate>=remaining&&baseline<candidate)) {
                ++result.latencyGuardFallbacks;return shortest;
            }
        }
        return selected;
    }
    void forward(std::size_t id) {
        if(result.packets[id].status!="pending")return;
        auto source=pending[id].node;
        if(!eligible(source)){finish(id,"node_unavailable");return;}
        RouteResult chosen;
        for(const auto& destination:pending[id].destinations) {
            chosen=route(id,nodeIds.at(destination));
            if(chosen.found()){result.packets[id].destination=destination;break;}
        }
        if(!chosen.found()){finish(id,"no_route");return;}
        if(chosen.hops.size()==1){finish(id,"delivered");return;}
        auto direction=directionIds.at({source,chosen.hops[1]});auto& d=directions[direction];
        std::size_t count=0;for(const auto& q:d.queues)count+=q.size();
        if(count>=c.links[d.link].queuePackets){finish(id,"queue_overflow");return;}
        d.queues[result.packets[id].priority].push_back(id);
        pending[id].queueOrder=sequence++;
        // Service is ordered after all arrivals/generations at this instant.
        schedule(now,3,[this,direction]{pump(direction);});
    }
    void pump(std::size_t direction) {
        auto& d=directions[direction];if(d.busy)return;
        unsigned selected=4;
        for(unsigned p=0;p<4;++p)if(!d.queues[p].empty()) {
            if(selected==4 || (c.scheduler=="fifo"&&pending[d.queues[p].front()].queueOrder<pending[d.queues[selected].front()].queueOrder))selected=p;
            if(c.scheduler=="strictPriority")break;
        }
        if(selected==4)return;
        auto id=d.queues[selected].front();d.queues[selected].pop_front();
        if(!usable(d)){forward(id);schedule(now,3,[this,direction]{pump(direction);});return;}
        d.busy=true;d.busyUntil=now+serialization(d,result.packets[id].bytes);
        auto le=linkEpoch[d.link], fromEpoch=nodeEpoch[d.from], toEpoch=nodeEpoch[d.to];
        consume(d.from,nodes[d.from].txPercentPerByte*result.packets[id].bytes);
        schedule(d.busyUntil,1,[this,direction,id,le,fromEpoch,toEpoch]{
            auto& active=directions[direction];active.busy=false;active.busyUntil=now;
            schedule(now+c.links[active.link].delayMs,1,[this,direction,id,le,fromEpoch,toEpoch]{
                const auto& completed=directions[direction];
                if(le!=linkEpoch[completed.link]||fromEpoch!=nodeEpoch[completed.from]||toEpoch!=nodeEpoch[completed.to]||
                   !linkUp[completed.link]||!nodes[completed.from].available||!eligible(completed.to)) {
                    finish(id,"in_flight_failure");return;
                }
                pending[id].node=completed.to;result.packets[id].path.push_back(nodes[completed.to].id);
                consume(completed.to,nodes[completed.to].rxPercentPerByte*result.packets[id].bytes);
                if(nodes[completed.to].id==result.packets[id].destination)finish(id,"delivered");else forward(id);
            });
            schedule(now,3,[this,direction]{pump(direction);});
        });
    }
    void generate(const std::string& flow,const std::string& source,const std::vector<std::string>& destinations,unsigned priority,unsigned bytes) {
        std::size_t id=result.packets.size();
        result.packets.push_back({id,flow,source,"","pending",priority,bytes,now,now,{source}});
        pending.push_back({destinations,nodeIds.at(source)});forward(id);
    }
    void topologyEvent(const Event& e) {
        result.events.push_back({now,e.type,e.target,e.available});
        if(e.type=="node") {auto i=nodeIds.at(e.target);if(nodes[i].available!=e.available){nodes[i].available=e.available;++nodeEpoch[i];}}
        if(e.type=="link") {auto i=linkIds.at(e.target);if(linkUp[i]!=e.available){linkUp[i]=e.available;++linkEpoch[i];}}
        // Flush affected waiting packets for immediate rerouting. Already-transmitting packets
        // retain epochs and are lost even if the failed component recovers before arrival.
        std::vector<std::size_t> reroute;
        for(auto& d:directions)if(!usable(d))for(auto& q:d.queues){reroute.insert(reroute.end(),q.begin(),q.end());q.clear();}
        std::sort(reroute.begin(),reroute.end());for(auto id:reroute)forward(id);
    }
    void safetyEvent(const Event& e) {
        auto controller=nodeIds.at(e.target);
        auto epoch=nodeEpoch[controller];double reading=now;
        schedule(now+c.safetyProcessingMs,1,[this,e,controller,epoch,reading]{
            if(!eligible(controller)||epoch!=nodeEpoch[controller]) {
                result.safetyEvents.push_back({e.zone,"controller_unavailable",reading,now,false});return;
            }
            if(e.type=="reset") {
                bool reset=safety.acknowledgeEmergencyStop(e.zone,e.temperatureCelsius);
                result.safetyEvents.push_back({e.zone,reset?"reset":"reset_rejected",reading,now,true});return;
            }
            auto decision=safety.processReading(e.zone,e.temperatureCelsius);
            result.safetyEvents.push_back({e.zone,toString(decision.state),reading,now,true});
            if(decision.broadcastEmergencyStop)for(const auto& n:nodes)if(n.role=="endpoint"&&n.zone==e.zone)
                generate("safety-broadcast",e.target,{n.id},0,64);
        });
    }
public:
    explicit Engine(const Configuration& config):c(config),nodes(c.nodes),safety(c.safety) {
        for(std::size_t i=0;i<nodes.size();++i)nodeIds.emplace(nodes[i].id,i);
        nodeEpoch.resize(nodes.size());
        for(std::size_t i=0;i<c.links.size();++i) {
            const auto& l=c.links[i];linkIds.emplace(l.id,i);linkUp.push_back(l.available);linkEpoch.push_back(0);
            auto from=nodeIds.at(l.from),to=nodeIds.at(l.to);
            directionIds.emplace(std::make_pair(from,to),directions.size());directions.push_back({i,from,to,{}});
            directionIds.emplace(std::make_pair(to,from),directions.size());directions.push_back({i,to,from,{}});
        }
    }
    SimulationResult run() {
        for(const auto& n:nodes)if(n.energyConstrained&&n.batteryPercent<=n.criticalPercent)result.firstEnergyExclusionMs=0;
        for(const auto& e:c.events)schedule(e.timeMs,0,[this,e]{
            if(e.type=="temperature"||e.type=="reset")safetyEvent(e);else topologyEvent(e);
        });
        std::mt19937 random(c.seed);
        for(const auto& w:c.traffic) {
            // Defined integer-to-double mapping avoids implementation-specific distribution behavior.
            double phase=static_cast<double>(random())/4294967296.0*w.phaseJitterMs;
            for(std::uint64_t k=0;;++k) {
                double t=w.startMs+phase+static_cast<double>(k)*w.intervalMs;
                if(t>=w.stopMs)break;
                schedule(t,2,[this,w]{generate(w.id,w.source,w.destinations,w.priority,w.bytes);});
            }
        }
        const double end=c.durationMs+c.drainMs;
        while(!calendar.empty()&&calendar.top().time<=end) {
            auto event=calendar.top();calendar.pop();now=event.time;event.action();
        }
        now=end;for(std::size_t i=0;i<result.packets.size();++i)finish(i,"cutoff");
        result.stationary=true;
        for(std::size_t i=0;i<nodes.size();++i)if(nodes[i].position.x!=c.nodes[i].position.x||nodes[i].position.y!=c.nodes[i].position.y)result.stationary=false;
        if(!result.stationary)throw std::logic_error("Stationary-node invariant violated");
        result.finalNodes=nodes;return result;
    }
};
}
SimulationResult simulate(const Configuration& c) {validate(c);return Engine(c).run();}
} // namespace ealr::fabric
