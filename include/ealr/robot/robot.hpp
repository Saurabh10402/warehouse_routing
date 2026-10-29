#pragma once

#include <cstdint>
#include <vector>

namespace ealr {

using RobotId = std::uint32_t;

struct Position2D {
    double x;
    double y;

    double distanceTo(const Position2D& other) const;
};

enum class RobotState {
    Idle,
    Working,
    LowBattery,
    ReturningToCharge
};

enum class BatteryTransition {
    None,
    LowBatteryWarning,
    ReturningToCharge
};

struct BatteryThresholds {
    double lowBatteryPercent;
    double criticalBatteryPercent;
};

/**
 * Domain representation of one robot on the factory floor.
 *
 * A robot permanently leaves routing when its battery reaches the critical
 * threshold. Low-battery robots remain eligible relays so EALR can decide how
 * strongly to avoid them through its routing cost.
 */
class Robot {
public:
    Robot(RobotId id,
          Position2D position,
          double initialBatteryPercent,
          double communicationRadiusMeters,
          BatteryThresholds batteryThresholds);

    RobotId id() const;
    const Position2D& position() const;
    double batteryPercent() const;
    double communicationRadiusMeters() const;
    RobotState state() const;
    std::uint32_t queueLength() const;
    const std::vector<RobotId>& neighbours() const;

    /** Updates the robot's position; factory mobility models own when this occurs. */
    void setPosition(Position2D position);

    /** Chooses the normal activity state when the battery is above the low threshold. */
    void setWorking(bool working);

    /**
     * Decreases battery percentage and returns any state transition caused by
     * crossing a configured threshold. Battery never increases in this model.
     */
    BatteryTransition consumeBattery(double percentagePoints);

    /** Replaces the current neighbour view after topology discovery. */
    void setNeighbours(std::vector<RobotId> neighbourIds);
    void clearNeighbours();

    void enqueuePacket();
    bool dequeuePacket();

    /** True unless the robot has entered its permanent return-to-charge state. */
    bool isAvailableForRouting() const;

private:
    BatteryTransition updateBatteryState();

    RobotId id_;
    Position2D position_;
    double batteryPercent_;
    double communicationRadiusMeters_;
    BatteryThresholds batteryThresholds_;
    bool working_ = false;
    RobotState state_ = RobotState::Idle;
    std::uint32_t queueLength_ = 0;
    std::vector<RobotId> neighbours_;
};

const char* toString(RobotState state);

}  // namespace ealr
