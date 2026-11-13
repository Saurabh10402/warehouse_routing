// SPDX-License-Identifier: GPL-2.0-only
// Small stationary radio/fabric delivery control; not a calibrated study.
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/wifi-module.h"
#include <fstream>
#include <map>
#include <string>
using namespace ns3;

struct Entry
{
    std::string id, role, access, attachment;
    Vector position;
    Ptr<Node> node;
    Ptr<NetDevice> radio;
};
#include "access-task-fabric.h"
#include "access-authorization.h"

int main(int argc, char** argv)
{
    std::string input, output, links, tasks;
    uint32_t radioSeed=1;uint64_t radioRun=1;
    CommandLine cmd;
    cmd.AddValue("inventory", "Validated node table from prepare_wireless_nodes.py", input);
    cmd.AddValue("output", "New CSV device audit path", output);
    cmd.AddValue("links", "Validated direct-zone links", links);
    cmd.AddValue("tasks", "Validated task control table", tasks);
    cmd.AddValue("radioSeed","ns-3 radio RNG seed",radioSeed);
    cmd.AddValue("radioRun","ns-3 radio RNG substream run",radioRun);
    cmd.Parse(argc, argv);
    NS_ABORT_MSG_IF(radioSeed<1 || radioSeed>2147483647 || radioRun<1 || radioRun>2147483647,"Invalid radio seed/run");
    RngSeedManager::SetSeed(radioSeed);RngSeedManager::SetRun(radioRun);
    std::cout<<"RADIO_RNG "<<RngSeedManager::GetSeed()<<" "<<RngSeedManager::GetRun()<<'\n';
    NS_ABORT_MSG_IF(input.empty() || output.empty(), "inventory and output required");
    NS_ABORT_MSG_IF(std::ifstream(output).good(), "Refusing to overwrite audit");
    std::ifstream source(input);
    NS_ABORT_MSG_IF(!source, "Cannot read inventory");
    std::map<std::string, Entry> entries;
    std::string id, role, access, attachment;
    double x, y, z;
    while (source >> id >> role >> access >> attachment >> x >> y >> z)
    {
        NS_ABORT_MSG_IF(entries.count(id), "Duplicate node");
        NS_ABORT_MSG_IF(entries.size() >= 4096, "Inventory too large");
        entries.emplace(id, Entry{id,role,access,attachment,Vector(x,y,z),CreateObject<Node>(),nullptr});
    }
    NS_ABORT_MSG_IF(!source.eof() || entries.empty(), "Invalid node table");
    InternetStackHelper internet;
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    auto epc = CreateObject<NrNoBackhaulEpcHelper>();
    auto nr = CreateObject<NrHelper>();
    auto beam = CreateObject<IdealBeamformingHelper>();
    beam->SetAttribute("BeamformingMethod", TypeIdValue(DirectPathBeamforming::GetTypeId()));
    nr->SetEpcHelper(epc);
    nr->SetBeamformingHelper(beam);
    CcBwpCreator creator;
    auto band = creator.CreateOperationBandContiguousCc(
        CcBwpCreator::SimpleOperationBandConf(3.5e9,20e6,1));
    auto channel = CreateObject<NrChannelHelper>();
    channel->ConfigureFactories("UMi", "Default", "ThreeGpp");
    channel->SetPathlossAttribute("ShadowingEnabled",BooleanValue(false));
    channel->SetChannelConditionModelAttribute("UpdatePeriod",TimeValue(MilliSeconds(0)));
    channel->AssignChannelsToBands({band});
    auto bwps = CcBwpCreator::GetAllBwps({band});
    NodeContainer gnbs, ues;
    for (auto& [name,e] : entries)
    {
        mobility.Install(e.node);
        e.node->GetObject<MobilityModel>()->SetPosition(e.position);
        if (e.role == "gnb") gnbs.Add(e.node);
        else internet.Install(e.node);
        if (e.access == "nr") ues.Add(e.node);
    }
    NS_ABORT_MSG_IF(gnbs.GetN()==0 || ues.GetN()==0, "This control requires NR gNBs and UEs");
    auto gd = nr->InstallGnbDevice(gnbs,bwps);
    auto ud = nr->InstallUeDevice(ues,bwps);
    int64_t stream = 1;
    stream += nr->AssignStreams(gd,stream);
    stream += nr->AssignStreams(ud,stream);
    for (uint32_t i=0; i<gd.GetN(); ++i)
    {
        NrHelper::GetGnbPhy(gd.Get(i),0)->SetAttribute("Numerology",UintegerValue(1));
        NrHelper::GetGnbPhy(gd.Get(i),0)->SetAttribute("TxPower",DoubleValue(20));
    }
    epc->AssignUeIpv4Address(ud);
    for (auto& [name,e] : entries)
    {
        for (uint32_t i=0;i<gd.GetN();++i) if (gd.Get(i)->GetNode()==e.node) e.radio=gd.Get(i);
        for (uint32_t i=0;i<ud.GetN();++i) if (ud.Get(i)->GetNode()==e.node) e.radio=ud.Get(i);
    }
    // A shared channel retains co-channel interference between all Wi-Fi BSSs.
    auto wifiChannel = YansWifiChannelHelper::Default().Create();
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",StringValue("HeMcs0"),"ControlMode",StringValue("OfdmRate6Mbps"));
    YansWifiPhyHelper phy;
    phy.SetChannel(wifiChannel);
    phy.Set("ChannelSettings",StringValue("{36, 20, BAND_5GHZ, 0}"));
    WifiMacHelper mac;
    for (auto& [name,e] : entries)
    {
        if (e.role!="ap" && e.access!="wifi") continue;
        const auto ap = e.role=="ap" ? name : e.attachment;
        NS_ABORT_MSG_IF(!entries.count(ap) || entries.at(ap).role!="ap", "Invalid Wi-Fi attachment");
        if (e.role=="ap") mac.SetType("ns3::ApWifiMac","Ssid",SsidValue(Ssid(ap)));
        else mac.SetType("ns3::StaWifiMac","Ssid",SsidValue(Ssid(ap)),"ActiveProbing",BooleanValue(false));
        auto devices=wifi.Install(phy,mac,e.node);
        stream += wifi.AssignStreams(devices,stream);
        e.radio=devices.Get(0);
    }
    TaskControl control;
    control.Load(tasks);
    auto server=CreateObject<Node>(); internet.Install(server);
    auto addresses=InstallTaskFabric(entries,epc,links,server,control.upperDelayMs,control.outageSpine,control.downMs,control.upMs,control.routingPolicy);
    control.Install(entries,addresses,server);
    for (auto& [name,e] : entries)
        if (e.access=="nr")
        {
            NS_ABORT_MSG_IF(!entries.count(e.attachment) || entries.at(e.attachment).role!="gnb", "Invalid NR attachment");
            nr->AttachToGnb(e.radio, entries.at(e.attachment).radio);
        }

    Simulator::Stop(Seconds(control.stopSeconds));
    Simulator::Run();
    bool deliveryPassed=control.Report();
    for (const auto& [name,e] : entries)
    {
        if (e.access=="nr")
        {
            auto ue=DynamicCast<NrUeNetDevice>(e.radio);
            auto gnb=DynamicCast<NrGnbNetDevice>(entries.at(e.attachment).radio);
            NS_ABORT_MSG_IF(ue->GetCellId()!=gnb->GetCellId(), "NR cell mismatch: " << name);
            NS_ABORT_MSG_IF(ue->GetRrc()->GetState()!=NrUeRrc::CONNECTED_NORMALLY, "NR not connected: " << name);
        }
        if (e.access=="wifi")
        {
            auto station=DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(e.radio)->GetMac());
            auto ap=DynamicCast<WifiNetDevice>(entries.at(e.attachment).radio);
            NS_ABORT_MSG_IF(!station->IsAssociated() || station->GetBssid(0)!=ap->GetMac()->GetAddress(), "Wi-Fi association mismatch: " << name);
        }
    }
    NS_ABORT_MSG_IF(NodeList::GetNNodes()!=entries.size()+4, "Unexpected EPC/inventory node count");
    std::ofstream audit(output);
    NS_ABORT_MSG_IF(!audit,"Cannot create audit");
    audit << "id,role,nodeId,radioType,attachment,x,y,z\n";
    for (const auto& [name,e] : entries)
    {
        audit << name << ',' << e.role << ',' << e.node->GetId() << ','
              << (e.radio ? e.radio->GetInstanceTypeId().GetName() : "none") << ','
              << e.attachment << ',' << e.position.x << ',' << e.position.y << ',' << e.position.z << '\n';
    }
    audit.close();
    std::cout << "AUTHORIZATION CONTROL (external main-server model): inventory=" << entries.size()+3
              << " actualNodes=" << NodeList::GetNNodes() << "; tasks=" << control.tasks.size() << '\n';
    Simulator::Destroy();
    return deliveryPassed ? 0 : 2;
}
