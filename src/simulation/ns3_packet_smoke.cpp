#include "ealr/simulation/ns3_packet_smoke.hpp"
#include "ealr/metrics/ns3_flow_monitor_collector.hpp"
#include <ns3/applications-module.h>
#include <ns3/double.h>
#include <ns3/error-model.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-helper.h>
#include <ns3/mobility-model.h>
#include <ns3/point-to-point-module.h>
#include <ns3/pointer.h>
#include <ns3/position-allocator.h>
#include <ns3/rng-seed-manager.h>
#include <ns3/simulator.h>
#include <ns3/string.h>
#include <stdexcept>
#include <string>

namespace ealr {
Ns3PacketSmokeResult runNs3PacketSmoke(unsigned hops, bool dropAtDestination) {
    if (hops < 1 || hops > 2) throw std::invalid_argument("Smoke fixture supports one or two hops");
    // Restore ns-3 global simulation state even when a C++ exception interrupts the fixture.
    struct Cleanup { ~Cleanup() { ns3::Simulator::Destroy(); } } cleanup;
    ns3::RngSeedManager::SetSeed(1);
    ns3::RngSeedManager::SetRun(1);
    ns3::NodeContainer nodes;
    nodes.Create(hops + 1);
    ns3::MobilityHelper mobility;
    auto positions = ns3::CreateObject<ns3::ListPositionAllocator>();
    for (unsigned i = 0; i <= hops; ++i) positions->Add(ns3::Vector(i * 5.0, 0, 0));
    mobility.SetPositionAllocator(positions);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    ns3::InternetStackHelper internet;
    internet.Install(nodes);
    ns3::PointToPointHelper link;
    link.SetDeviceAttribute("DataRate", ns3::StringValue("5Mbps"));
    link.SetChannelAttribute("Delay", ns3::StringValue("2ms"));
    ns3::Ipv4Address destination;
    for (unsigned i = 0; i < hops; ++i) {
        auto devices = link.Install(nodes.Get(i), nodes.Get(i + 1));
        ns3::Ipv4AddressHelper addresses;
        auto network = "10.1." + std::to_string(i + 1) + ".0";
        addresses.SetBase(network.c_str(), "255.255.255.0");
        auto interfaces = addresses.Assign(devices);
        destination = interfaces.GetAddress(1);
        if (dropAtDestination && i + 1 == hops) {
            auto errors = ns3::CreateObject<ns3::RateErrorModel>();
            errors->SetAttribute("ErrorRate", ns3::DoubleValue(1.0));
            devices.Get(1)->SetAttribute("ReceiveErrorModel", ns3::PointerValue(errors));
        }
    }
    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    ns3::UdpServerHelper server(9000);
    auto serverApps = server.Install(nodes.Get(hops));
    serverApps.Start(ns3::Seconds(0));
    serverApps.Stop(ns3::Seconds(1));
    ns3::UdpClientHelper client(destination, 9000);
    client.SetAttribute("MaxPackets", ns3::UintegerValue(3));
    client.SetAttribute("Interval", ns3::TimeValue(ns3::MilliSeconds(10)));
    client.SetAttribute("PacketSize", ns3::UintegerValue(100));
    auto clientApps = client.Install(nodes.Get(0));
    clientApps.Start(ns3::MilliSeconds(10));
    clientApps.Stop(ns3::Seconds(1));
    ns3::FlowMonitorHelper flowHelper;
    auto monitor = flowHelper.InstallAll();
    // FlowMonitor's default loss timeout is 10 seconds. Wait past it in the loss fixture too.
    ns3::Simulator::Stop(ns3::Seconds(12));
    ns3::Simulator::Run();
    MetricsCollector metrics;
    auto flows = Ns3FlowMonitorCollector::collect(monitor, metrics);
    auto summary = metrics.summarize(12);
    Ns3PacketSmokeResult result;
    result.hops = hops;
    result.transmitted = summary.transmittedPackets;
    result.received = summary.receivedPackets;
    result.lost = summary.lostPackets;
    result.applicationReceived = ns3::DynamicCast<ns3::UdpServer>(serverApps.Get(0))->GetReceived();
    result.ipReceivedBytes = flows.statistics.receivedBytes;
    result.meanDelayMs = summary.averageEndToEndDelayMilliseconds;
    result.stationary = true;
    for (unsigned i = 0; i <= hops; ++i) {
        auto p = nodes.Get(i)->GetObject<ns3::MobilityModel>()->GetPosition();
        result.stationary = result.stationary && p.x == i * 5.0 && p.y == 0 && p.z == 0;
    }
    return result;
}
}
