#include "ealr/simulation/ns3_packet_smoke.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    int failures = 0;
    auto check = [&](bool valid, const char* label) { if (!valid) { ++failures; std::cerr << label << '\n'; } };
    for (unsigned hops : {1U, 2U}) {
        auto result = ealr::runNs3PacketSmoke(hops);
        check(result.transmitted == 3 && result.received == 3 && result.applicationReceived == 3 && result.lost == 0,
              "UDP receiver and FlowMonitor must agree on exact packets");
        check(result.ipReceivedBytes == 384, "FlowMonitor counts IP bytes, not UDP payload bytes");
        check(std::abs(result.meanDelayMs - hops * 2.208) < 1e-6, "Delay must include PPP/IP/UDP serialization and every routed hop");
        check(result.stationary, "Constant positions must remain unchanged");
        auto dropped = ealr::runNs3PacketSmoke(hops, true);
        check(dropped.transmitted == 3 && dropped.received == 0 && dropped.applicationReceived == 0 && dropped.lost == 3,
              "Destination receive errors must become real packet loss after FlowMonitor timeout");
        check(dropped.stationary, "Failure fixture remains stationary");
    }
    try { ealr::runNs3PacketSmoke(0); check(false, "Invalid hop count accepted"); } catch (const std::invalid_argument&) {}
    return failures ? 1 : 0;
}
