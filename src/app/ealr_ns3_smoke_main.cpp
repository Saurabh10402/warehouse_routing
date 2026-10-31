#include "ealr/simulation/ns3_packet_smoke.hpp"
#include <boost/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc != 3 || std::string(argv[1]) != "--output")
            throw std::invalid_argument("Usage: ealr-ns3-smoke --output NEW_DIRECTORY");
        std::filesystem::path output = argv[2];
        if (output.empty() || std::filesystem::exists(output))
            throw std::invalid_argument("Choose a new output directory; existing results are never overwritten");
        auto r = ealr::runNs3PacketSmoke();
        // 100 UDP payload + 8 UDP + 20 IPv4 + 2 PPP bytes, serialized on each 5 Mbps hop.
        if (r.transmitted != 3 || r.received != 3 || r.lost != 0 || r.applicationReceived != 3 ||
            r.ipReceivedBytes != 384 || !r.stationary || std::abs(r.meanDelayMs - 4.416) > 1e-6)
            throw std::runtime_error("ns-3 packet fixture failed its analytical checks");
        boost::json::object report{{"status", "PASS"}, {"model", "ns-3 wired IPv4/UDP two-hop smoke; not warehouse or NR"},
            {"hops", r.hops}, {"transmitted_packets", r.transmitted}, {"received_packets", r.received},
            {"lost_packets", r.lost}, {"application_received_packets", r.applicationReceived},
            {"udp_payload_bytes_per_packet", 100}, {"ip_received_bytes", r.ipReceivedBytes},
            {"mean_delay_ms", r.meanDelayMs}, {"stationary", r.stationary}};
        if (!std::filesystem::create_directories(output)) throw std::runtime_error("Output directory creation failed");
        std::ofstream file(output / "smoke.json");
        file << boost::json::serialize(report) << '\n';
        file.close();
        if (!file) throw std::runtime_error("Could not write smoke.json");
        std::cout << "PASS: actual ns-3 two-hop UDP: 3/3 packets, 4.416 ms; stationary. Output " << output << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ns-3 smoke failed: " << e.what() << '\n';
        return 1;
    }
}
