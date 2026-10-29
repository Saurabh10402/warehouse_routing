#include "ealr/metrics/ns3_flow_monitor_collector.hpp"

#include <ns3/applications-module.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/simulator.h>
#include <ns3/string.h>

#include <iostream>

int main() {
    ns3::NodeContainer nodes;
    nodes.Create(2);

    ns3::PointToPointHelper link;
    link.SetDeviceAttribute("DataRate", ns3::StringValue("5Mbps"));
    link.SetChannelAttribute("Delay", ns3::StringValue("2ms"));
    const ns3::NetDeviceContainer devices = link.Install(nodes);

    ns3::InternetStackHelper internet;
    internet.Install(nodes);
    ns3::Ipv4AddressHelper addresses;
    addresses.SetBase("10.1.1.0", "255.255.255.0");
    const ns3::Ipv4InterfaceContainer interfaces = addresses.Assign(devices);

    constexpr std::uint16_t port = 9000;
    ns3::UdpServerHelper server(port);
    ns3::ApplicationContainer serverApplication = server.Install(nodes.Get(1));
    serverApplication.Start(ns3::Seconds(0.0));
    serverApplication.Stop(ns3::Seconds(1.0));

    ns3::UdpClientHelper client(interfaces.GetAddress(1), port);
    client.SetAttribute("MaxPackets", ns3::UintegerValue(3));
    client.SetAttribute("Interval", ns3::TimeValue(ns3::MilliSeconds(10)));
    client.SetAttribute("PacketSize", ns3::UintegerValue(100));
    ns3::ApplicationContainer clientApplication = client.Install(nodes.Get(0));
    clientApplication.Start(ns3::MilliSeconds(10));
    clientApplication.Stop(ns3::Seconds(1.0));

    ns3::FlowMonitorHelper flowMonitorHelper;
    const ns3::Ptr<ns3::FlowMonitor> flowMonitor = flowMonitorHelper.InstallAll();

    ns3::Simulator::Stop(ns3::Seconds(1.0));
    ns3::Simulator::Run();

    ealr::MetricsCollector metrics;
    const ealr::FlowMonitorCollectionResult result =
        ealr::Ns3FlowMonitorCollector::collect(flowMonitor, metrics);
    const ealr::SimulationMetrics summary = metrics.summarize(1.0);
    ns3::Simulator::Destroy();

    if (result.monitoredFlowCount == 0 || summary.transmittedPackets < 3 || summary.receivedPackets < 3 ||
        summary.packetDeliveryRatioPercent < 99.0 || summary.averageEndToEndDelayMilliseconds <= 0.0 ||
        summary.throughputMbps <= 0.0) {
        std::cerr << "FlowMonitor statistics were not collected as expected\n";
        return 1;
    }
    return 0;
}
