#include "ealr/metrics/fabric_metrics.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <stdexcept>

namespace ealr::fabric {
namespace {
std::ofstream file(const std::filesystem::path& path) {
    std::ofstream f(path);if(!f)throw std::runtime_error("Cannot write "+path.string());
    f.exceptions(std::ios::badbit|std::ios::failbit);f<<std::setprecision(17);return f;
}
double quantile(const std::vector<double>& sorted,double q) {return sorted[static_cast<std::size_t>(std::ceil(q*static_cast<double>(sorted.size())))-1];}
void optional(std::ostream& o,const std::optional<double>& v) {if(v)o<<*v;}
}
PacketMetrics summarize(const Configuration& c,const SimulationResult& r,int priority) {
    if(priority < -1 || priority>3)throw std::invalid_argument("Metric priority must be -1 or 0..3");
    if(!std::isfinite(c.durationMs+c.drainMs)||c.durationMs+c.drainMs<=0)throw std::invalid_argument("Invalid observation window");
    PacketMetrics m;std::vector<double> delays;
    for(const auto& p:r.packets)if(priority==-1||p.priority==static_cast<unsigned>(priority)) {
        if(p.priority>3||p.status=="pending")throw std::invalid_argument("Metrics require finalized packets with valid priority");
        ++m.sent;
        if(p.status=="delivered") {
            double delay=p.finishedMs-p.createdMs;
            if(!std::isfinite(delay)||delay<0)throw std::invalid_argument("Invalid packet delay");
            ++m.delivered;m.receivedBytes+=p.bytes;delays.push_back(delay);
            if(delay>=c.deadlinesMs[p.priority])++m.deadlineMisses;
        } else {++m.lost;++m.deadlineMisses;}
    }
    if(!delays.empty()) {
        std::sort(delays.begin(),delays.end());m.meanMs=std::accumulate(delays.begin(),delays.end(),0.0)/static_cast<double>(delays.size());
        auto n=delays.size();m.medianMs=n%2?delays[n/2]:(delays[n/2-1]+delays[n/2])/2;
        if(n>=20)m.p95Ms=quantile(delays,0.95);
        if(n>=100)m.p99Ms=quantile(delays,0.99);
    }
    if(m.sent){m.deliveryPercent=100.0*static_cast<double>(m.delivered)/static_cast<double>(m.sent);m.deadlineMissPercent=100.0*static_cast<double>(m.deadlineMisses)/static_cast<double>(m.sent);}
    m.throughputMbps=static_cast<double>(m.receivedBytes)*8/((c.durationMs+c.drainMs)*1000);
    return m;
}
void writeResults(const Configuration& c,const SimulationResult& r,const std::filesystem::path& output) {
    if(output.empty()||std::filesystem::exists(output))throw std::invalid_argument("Result directory must be new: "+output.string());
    if(!output.parent_path().empty())std::filesystem::create_directories(output.parent_path());
    if(!std::filesystem::create_directory(output))throw std::runtime_error("Cannot create result directory");
    auto packets=file(output/"packets.csv");packets<<"id,flow,source,destination,priority,bytes,created_ms,finished_ms,status,latency_ms,deadline_miss,path\n";
    for(const auto& p:r.packets) {
        packets<<p.id<<','<<p.flow<<','<<p.source<<','<<p.destination<<','<<p.priority<<','<<p.bytes<<','<<p.createdMs<<','<<p.finishedMs<<','<<p.status<<',';
        if(p.status=="delivered")packets<<p.finishedMs-p.createdMs;
        packets<<','<<(p.status!="delivered"||p.finishedMs-p.createdMs>=c.deadlinesMs[p.priority])<<',';
        for(std::size_t i=0;i<p.path.size();++i){if(i)packets<<'>';packets<<p.path[i];}packets<<'\n';
    }
    auto classes=file(output/"per_priority.csv");classes<<"priority,sent,delivered,lost,mean_ms,median_ms,p95_ms,p99_ms,delivery_percent,deadline_miss_percent,throughput_mbps\n";
    for(int p=0;p<4;++p){auto m=summarize(c,r,p);classes<<p<<','<<m.sent<<','<<m.delivered<<','<<m.lost<<','<<m.meanMs<<','<<m.medianMs<<',';optional(classes,m.p95Ms);classes<<',';optional(classes,m.p99Ms);classes<<','<<m.deliveryPercent<<','<<m.deadlineMissPercent<<','<<m.throughputMbps<<'\n';}
    auto m=summarize(c,r);auto metrics=file(output/"metrics.csv");metrics<<"metric,value\n";
    metrics<<"transmitted_packets,"<<m.sent<<"\nreceived_packets,"<<m.delivered<<"\nlost_packets,"<<m.lost
        <<"\naverage_end_to_end_delay_ms,"<<m.meanMs<<"\nmedian_latency_ms,"<<m.medianMs
        <<"\npacket_delivery_ratio_percent,"<<m.deliveryPercent<<"\npacket_loss_percent,"<<(m.sent?100-m.deliveryPercent:0)
        <<"\ndeadline_miss_percent,"<<m.deadlineMissPercent<<"\nthroughput_mbps,"<<m.throughputMbps
        <<"\nroute_recomputations,"<<r.routeComputations<<"\nlatency_guard_fallbacks,"<<r.latencyGuardFallbacks
        <<"\nstationary,"<<r.stationary<<"\nobservation_window_ms,"<<c.durationMs+c.drainMs<<'\n';
    if(m.p95Ms)metrics<<"p95_latency_ms,"<<*m.p95Ms<<'\n';
    if(m.p99Ms)metrics<<"p99_latency_ms,"<<*m.p99Ms<<'\n';
    for(const auto* status:{"queue_overflow","reserved_capacity","priority_policed"})
        metrics<<status<<"_packets,"<<std::count_if(r.packets.begin(),r.packets.end(),[&](const auto& p){return p.status==status;})<<'\n';
    auto queues=file(output/"queue_stats.csv");
    queues<<"link,from,to,priority,attempts,admitted,service_starts,bytes_started,overflow_drops,reservation_drops,policed_drops,rerouted,queued_at_cutoff,peak_waiting_packets,mean_wait_before_service_ms,max_wait_before_service_ms\n";
    for(const auto& q:r.queues) {
        const auto& s=q.statistics;
        queues<<q.link<<','<<q.from<<','<<q.to<<','<<q.priority<<','<<s.attempts<<','<<s.admitted<<','<<s.serviceStarts<<','<<s.bytesStarted
            <<','<<s.overflowDrops<<','<<s.reservationDrops<<','<<s.policedDrops<<','<<s.rerouted<<','<<s.queuedAtCutoff<<','<<s.peakWaiting
            <<','<<(s.serviceStarts?s.totalWaitMs/static_cast<double>(s.serviceStarts):0)<<','<<s.maxWaitMs<<'\n';
    }
    double sum=0,minimum=100,energy=0;std::size_t count=0;
    auto positions=file(output/"nodes.csv");positions<<"id,role,zone,access,initial_x,initial_y,final_x,final_y,energy_constrained,battery_percent,available\n";
    for(std::size_t i=0;i<r.finalNodes.size();++i) {
        const auto& n=r.finalNodes[i];const auto& initial=c.nodes[i];
        if(n.energyConstrained){++count;sum+=n.batteryPercent;minimum=std::min(minimum,n.batteryPercent);energy+=initial.batteryPercent-n.batteryPercent;}
        positions<<n.id<<','<<n.role<<','<<n.zone<<','<<n.access<<','<<initial.position.x<<','<<initial.position.y<<','<<n.position.x<<','<<n.position.y<<','<<n.energyConstrained<<','<<n.batteryPercent<<','<<n.available<<'\n';
    }
    metrics<<"energy_device_count,"<<count<<"\nenergy_consumed_percentage_points,"<<energy<<'\n';
    if(count)metrics<<"average_battery_remaining_percent,"<<sum/static_cast<double>(count)<<"\nminimum_battery_remaining_percent,"<<minimum
        <<"\nnetwork_lifetime_seconds,"<<(r.firstEnergyExclusionMs<0?c.durationMs+c.drainMs:r.firstEnergyExclusionMs)/1000
        <<"\nnetwork_lifetime_censored,"<<(r.firstEnergyExclusionMs<0)<<'\n';
    auto safety=file(output/"safety.csv");safety<<"zone,state,reading_ms,response_ms,processing_response_ms,controller_available\n";
    for(const auto& e:r.safetyEvents)safety<<e.zone<<','<<e.state<<','<<e.readingMs<<','<<e.responseMs<<','<<e.responseMs-e.readingMs<<','<<e.controllerAvailable<<'\n';
    auto events=file(output/"events.csv");events<<"time_ms,type,target,available\n";
    for(const auto& e:r.events)events<<e.timeMs<<','<<e.type<<','<<e.target<<','<<e.available<<'\n';
    auto recovery=file(output/"recovery.csv");recovery<<"event_ms,type,target,flow,first_post_event_delivery_ms,observed_recovery_ms\n";
    for(const auto& e:r.events)if((e.type=="link"||e.type=="node")&&!e.available)for(const auto& w:c.traffic) {
        std::optional<double> first;for(const auto& p:r.packets)if(p.flow==w.id&&p.createdMs>=e.timeMs&&p.status=="delivered")if(!first||p.finishedMs<*first)first=p.finishedMs;
        recovery<<e.timeMs<<','<<e.type<<','<<e.target<<','<<w.id<<',';optional(recovery,first);recovery<<',';if(first)recovery<<*first-e.timeMs;recovery<<'\n';
    }
    auto summary=file(output/"summary.txt");
    summary<<"Stationary C++ discrete-event link abstraction (NOT ns-3/5G radio scheduling)\nAlgorithm: "<<c.routing.algorithm
        <<"\nSeed: "<<c.seed<<"\nPackets generated/delivered/lost: "<<m.sent<<'/'<<m.delivered<<'/'<<m.lost
        <<"\nMean delivered latency (ms): "<<m.meanMs<<"\nDeadline misses including loss (%): "<<m.deadlineMissPercent
        <<"\nStationary invariant: "<<(r.stationary?"PASS":"FAIL")<<"\nSee docs/fabric-model.md for definitions and limits.\n";
}
} // namespace ealr::fabric
