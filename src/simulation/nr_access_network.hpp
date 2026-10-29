#pragma once

#include "ealr/config/configuration.hpp"

#include <ns3/ideal-beamforming-helper.h>
#include <ns3/node-container.h>
#include <ns3/nr-helper.h>
#include <ns3/nr-point-to-point-epc-helper.h>

namespace ealr {

struct NrAccessNetwork {
    ns3::Ptr<ns3::NrPointToPointEpcHelper> epcHelper;
    ns3::Ptr<ns3::IdealBeamformingHelper> beamformingHelper;
    ns3::Ptr<ns3::NrHelper> nrHelper;
};

NrAccessNetwork installNrAccessNetwork(const Private5gConfiguration& configuration,
                                       const ns3::NodeContainer& gnbNodes,
                                       const ns3::NodeContainer& robotNodes);

}  // namespace ealr
