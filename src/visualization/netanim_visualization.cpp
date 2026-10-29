#include "ealr/visualization/netanim_visualization.hpp"

#include <ns3/animation-interface.h>
#include <ns3/mobility-model.h>
#include <ns3/node-list.h>
#include <ns3/simulator.h>
#include <ns3/vector.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

using namespace std;

namespace ealr {
namespace {

void validateFiniteNonNegative(double value, const char* name) {
    if (!isfinite(value) || value < 0.0) {
        throw invalid_argument(string(name) + " must be finite and non-negative");
    }
}

void validateFinite(double value, const char* name) {
    if (!isfinite(value)) {
        throw invalid_argument(string(name) + " must be finite");
    }
}

}  // namespace

class NetAnimVisualization::Implementation {
public:
    explicit Implementation(const filesystem::path& outputPath) {
        if (outputPath.empty()) {
            throw invalid_argument("NetAnim output path must not be empty");
        }
        if (!outputPath.parent_path().empty()) {
            filesystem::create_directories(outputPath.parent_path());
        }
        animation = make_unique<ns3::AnimationInterface>(outputPath.string());
        animation->EnablePacketMetadata(true);
    }

    unique_ptr<ns3::AnimationInterface> animation;
};

NetAnimColor netAnimColorFor(RobotState state) {
    switch (state) {
        case RobotState::Idle:
        case RobotState::Working:
            return {40, 180, 80};
        case RobotState::LowBattery:
            return {246, 195, 67};
        case RobotState::ReturningToCharge:
            return {217, 83, 79};
    }
    return {128, 128, 128};
}

NetAnimVisualization::NetAnimVisualization(const filesystem::path& outputPath)
    : implementation_(make_unique<Implementation>(outputPath)) {
}

NetAnimVisualization::~NetAnimVisualization() = default;

void NetAnimVisualization::configureGnb(uint32_t nodeId) {
    implementation_->animation->UpdateNodeDescription(nodeId, "Private 5G gNB");
    implementation_->animation->UpdateNodeColor(nodeId, 40, 90, 220);
    implementation_->animation->UpdateNodeSize(nodeId, 10.0, 10.0);
}

void NetAnimVisualization::updateRobot(uint32_t nodeId, const Robot& robot) {
    const NetAnimColor color = netAnimColorFor(robot.state());
    implementation_->animation->UpdateNodeDescription(
        nodeId,
        "Robot " + to_string(robot.id()) + " | " + toString(robot.state()) +
            " | Battery " + to_string(robot.batteryPercent()) + "%");
    implementation_->animation->UpdateNodeColor(nodeId, color.red, color.green, color.blue);
    implementation_->animation->UpdateNodeSize(nodeId, 7.0, 7.0);
}

void NetAnimVisualization::setStopTime(double simulationTimeSeconds) {
    validateFiniteNonNegative(simulationTimeSeconds, "NetAnim stop time");
    implementation_->animation->SetStopTime(ns3::Seconds(simulationTimeSeconds));
}

void NetAnimVisualization::animateReturnToCharge(uint32_t nodeId,
                                                  Position2D chargingStation,
                                                  double speedMetersPerSecond,
                                                  double startTimeSeconds,
                                                  double updateIntervalSeconds) {
    validateFinite(chargingStation.x, "Charging station x");
    validateFinite(chargingStation.y, "Charging station y");
    if (!isfinite(speedMetersPerSecond) || speedMetersPerSecond <= 0.0) {
        throw invalid_argument("Charging return speed must be finite and greater than zero");
    }
    if (!isfinite(startTimeSeconds) || startTimeSeconds < 0.0) {
        throw invalid_argument("Charging return start time must be finite and non-negative");
    }
    if (!isfinite(updateIntervalSeconds) || updateIntervalSeconds <= 0.0) {
        throw invalid_argument("Charging return update interval must be finite and greater than zero");
    }
    if (nodeId >= ns3::NodeList::GetNNodes()) {
        throw invalid_argument("NetAnim robot node does not exist");
    }

    const ns3::Ptr<ns3::MobilityModel> mobility = ns3::NodeList::GetNode(nodeId)->GetObject<ns3::MobilityModel>();
    if (mobility == nullptr) {
        throw invalid_argument("NetAnim robot node has no mobility model");
    }

    const ns3::Vector initialPosition = mobility->GetPosition();
    const double xDistance = chargingStation.x - initialPosition.x;
    const double yDistance = chargingStation.y - initialPosition.y;
    const double travelDistance = sqrt((xDistance * xDistance) + (yDistance * yDistance));
    const double travelTimeSeconds = travelDistance / speedMetersPerSecond;
    const uint32_t updateCount = max<uint32_t>(1, static_cast<uint32_t>(ceil(travelTimeSeconds / updateIntervalSeconds)));

    for (uint32_t update = 1; update <= updateCount; ++update) {
        const double progress = static_cast<double>(update) / static_cast<double>(updateCount);
        const ns3::Vector nextPosition(initialPosition.x + (xDistance * progress),
                                       initialPosition.y + (yDistance * progress),
                                       initialPosition.z);
        ns3::Simulator::Schedule(ns3::Seconds(startTimeSeconds + (travelTimeSeconds * progress)),
                                 [mobility, nextPosition] { mobility->SetPosition(nextPosition); });
    }
}

}  // namespace ealr
