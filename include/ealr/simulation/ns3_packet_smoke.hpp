#pragma once
#include <cstdint>

namespace ealr {
struct Ns3PacketSmokeResult {
    unsigned hops = 0;
    std::uint64_t transmitted = 0, received = 0, lost = 0, applicationReceived = 0;
    std::uint64_t ipReceivedBytes = 0;
    double meanDelayMs = 0;
    bool stationary = false;
};

// Fixed, routed UDP fixture for dependency/runtime validation, not warehouse or NR simulation.
// Each point-to-point hop is 5 Mbps / 2 ms; three 100-byte UDP payloads are sent 10 ms apart.
Ns3PacketSmokeResult runNs3PacketSmoke(unsigned hops = 2, bool dropAtDestination = false);
}
