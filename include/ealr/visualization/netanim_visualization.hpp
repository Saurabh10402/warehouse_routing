#pragma once

#include "ealr/robot/robot.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace ealr {

struct NetAnimColor {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

/** Maps operational and battery states to the NetAnim colour convention. */
NetAnimColor netAnimColorFor(RobotState state);

/**
 * Owns the NetAnim trace for an ns-3 scenario and keeps visual presentation
 * separate from mobility, energy, and routing decisions.
 */
class NetAnimVisualization {
public:
    explicit NetAnimVisualization(const std::filesystem::path& outputPath);
    ~NetAnimVisualization();

    NetAnimVisualization(const NetAnimVisualization&) = delete;
    NetAnimVisualization& operator=(const NetAnimVisualization&) = delete;

    void configureGnb(std::uint32_t nodeId);
    void updateRobot(std::uint32_t nodeId, const Robot& robot);
    void setStopTime(double simulationTimeSeconds);

    /**
     * Schedules visible position updates from a robot's current location to a
     * charging station. The caller retains responsibility for changing the
     * robot state before invoking this method.
     */
    void animateReturnToCharge(std::uint32_t nodeId,
                               Position2D chargingStation,
                               double speedMetersPerSecond,
                               double startTimeSeconds,
                               double updateIntervalSeconds);

private:
    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace ealr
