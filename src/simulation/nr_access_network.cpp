#include "nr_access_network.hpp"

#include <ns3/cc-bwp-helper.h>
#include <ns3/double.h>
#include <ns3/nr-channel-helper.h>
#include <ns3/nr-gnb-net-device.h>
#include <ns3/nr-ue-net-device.h>
#include <ns3/uinteger.h>

using namespace std;

namespace ealr {

NrAccessNetwork installNrAccessNetwork(const Private5gConfiguration& configuration,
                                       const ns3::NodeContainer& gnbNodes,
                                       const ns3::NodeContainer& robotNodes) {
    NrAccessNetwork network{
        ns3::CreateObject<ns3::NrPointToPointEpcHelper>(),
        ns3::CreateObject<ns3::IdealBeamformingHelper>(),
        ns3::CreateObject<ns3::NrHelper>()
    };

    network.nrHelper->SetEpcHelper(network.epcHelper);
    network.nrHelper->SetBeamformingHelper(network.beamformingHelper);
    network.nrHelper->SetGnbPhyAttribute("Numerology", ns3::UintegerValue(configuration.numerology));
    network.nrHelper->SetGnbPhyAttribute("TxPower", ns3::DoubleValue(configuration.gnbTxPowerDbm));
    network.nrHelper->SetUePhyAttribute("TxPower", ns3::DoubleValue(configuration.ueTxPowerDbm));

    ns3::CcBwpCreator bwpCreator;
    const ns3::CcBwpCreator::SimpleOperationBandConf bandConfiguration(
        configuration.centralFrequencyHz,
        configuration.channelBandwidthHz,
        1);
    ns3::OperationBandInfo band = bwpCreator.CreateOperationBandContiguousCc(bandConfiguration);

    const ns3::Ptr<ns3::NrChannelHelper> channelHelper = ns3::CreateObject<ns3::NrChannelHelper>();
    channelHelper->ConfigureFactories(configuration.channelScenario,
                                      configuration.channelCondition,
                                      configuration.channelModel);
    channelHelper->AssignChannelsToBands({band});
    const ns3::BandwidthPartInfoPtrVector bandwidthParts = ns3::CcBwpCreator::GetAllBwps({band});

    const ns3::NetDeviceContainer gnbDevices = network.nrHelper->InstallGnbDevice(gnbNodes, bandwidthParts);
    const ns3::NetDeviceContainer ueDevices = network.nrHelper->InstallUeDevice(robotNodes, bandwidthParts);
    for (auto device = gnbDevices.Begin(); device != gnbDevices.End(); ++device) {
        ns3::DynamicCast<ns3::NrGnbNetDevice>(*device)->UpdateConfig();
    }
    for (auto device = ueDevices.Begin(); device != ueDevices.End(); ++device) {
        ns3::DynamicCast<ns3::NrUeNetDevice>(*device)->UpdateConfig();
    }
    network.nrHelper->AttachToClosestGnb(ueDevices, gnbDevices);

    return network;
}

}  // namespace ealr
