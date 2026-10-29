#include "ealr/robot/robot.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace ealr {
namespace {

void validateFinite(double value, const char* name) {
    if (!isfinite(value)) {
        throw invalid_argument(string(name) + " must be finite");
    }
}

void validatePosition(Position2D position) {
    validateFinite(position.x, "Position x");
    validateFinite(position.y, "Position y");
}

void validateThresholds(BatteryThresholds thresholds) {
    validateFinite(thresholds.lowBatteryPercent, "Low battery threshold");
    validateFinite(thresholds.criticalBatteryPercent, "Critical battery threshold");
    if (thresholds.criticalBatteryPercent < 0.0 || thresholds.criticalBatteryPercent >= thresholds.lowBatteryPercent || thresholds.lowBatteryPercent > 100.0) {
        throw invalid_argument("Battery thresholds must satisfy 0 <= critical < low <= 100");
    }
}

}  // namespace

double Position2D::distanceTo(const Position2D& other) const {
    validatePosition(*this);
    validatePosition(other);
    const double xDifference = x - other.x;
    const double yDifference = y - other.y;
    const double distance = hypot(xDifference, yDifference);
    if (!isfinite(distance)) {
        throw overflow_error("Position distance is not finite");
    }
    return distance;
}

Robot::Robot(RobotId id,
             Position2D position,
             double initialBatteryPercent,
             double communicationRadiusMeters,
             BatteryThresholds batteryThresholds)
    : id_(id),
      position_(position),
      batteryPercent_(initialBatteryPercent),
      communicationRadiusMeters_(communicationRadiusMeters),
      batteryThresholds_(batteryThresholds) {
    validatePosition(position_);
    validateFinite(batteryPercent_, "Initial battery percentage");
    validateFinite(communicationRadiusMeters_, "Communication radius");
    validateThresholds(batteryThresholds_);
    if (batteryPercent_ < 0.0 || batteryPercent_ > 100.0) {
        throw invalid_argument("Initial battery percentage must be between 0 and 100");
    }
    if (communicationRadiusMeters_ <= 0.0) {
        throw invalid_argument("Communication radius must be greater than zero");
    }
    updateBatteryState();
}

RobotId Robot::id() const {
    return id_;
}

const Position2D& Robot::position() const {
    return position_;
}

double Robot::batteryPercent() const {
    return batteryPercent_;
}

double Robot::communicationRadiusMeters() const {
    return communicationRadiusMeters_;
}

RobotState Robot::state() const {
    return state_;
}

uint32_t Robot::queueLength() const {
    return queueLength_;
}

const vector<RobotId>& Robot::neighbours() const {
    return neighbours_;
}

void Robot::setPosition(Position2D position) {
    validatePosition(position);
    position_ = position;
}

void Robot::setWorking(bool working) {
    working_ = working;
    if (state_ != RobotState::LowBattery && state_ != RobotState::ReturningToCharge) {
        state_ = working_ ? RobotState::Working : RobotState::Idle;
    }
}

BatteryTransition Robot::consumeBattery(double percentagePoints) {
    validateFinite(percentagePoints, "Battery consumption");
    if (percentagePoints < 0.0) {
        throw invalid_argument("Battery consumption must not be negative");
    }

    batteryPercent_ = max(0.0, batteryPercent_ - percentagePoints);
    return updateBatteryState();
}

void Robot::setNeighbours(vector<RobotId> neighbourIds) {
    unordered_set<RobotId> uniqueNeighbours;
    for (const RobotId neighbourId : neighbourIds) {
        if (neighbourId == id_) {
            throw invalid_argument("A robot cannot be its own neighbour");
        }
        if (!uniqueNeighbours.insert(neighbourId).second) {
            throw invalid_argument("Neighbour list contains a duplicate robot ID");
        }
    }
    neighbours_ = std::move(neighbourIds);
}

void Robot::clearNeighbours() {
    neighbours_.clear();
}

void Robot::enqueuePacket() {
    if (queueLength_ == numeric_limits<uint32_t>::max()) {
        throw overflow_error("Robot queue length cannot exceed uint32_t maximum");
    }
    ++queueLength_;
}

bool Robot::dequeuePacket() {
    if (queueLength_ == 0) {
        return false;
    }
    --queueLength_;
    return true;
}

bool Robot::isAvailableForRouting() const {
    return state_ != RobotState::ReturningToCharge;
}

BatteryTransition Robot::updateBatteryState() {
    if (state_ == RobotState::ReturningToCharge) {
        return BatteryTransition::None;
    }
    if (batteryPercent_ <= batteryThresholds_.criticalBatteryPercent) {
        state_ = RobotState::ReturningToCharge;
        return BatteryTransition::ReturningToCharge;
    }
    if (batteryPercent_ <= batteryThresholds_.lowBatteryPercent) {
        const bool wasLowBattery = state_ == RobotState::LowBattery;
        state_ = RobotState::LowBattery;
        return wasLowBattery ? BatteryTransition::None : BatteryTransition::LowBatteryWarning;
    }

    state_ = working_ ? RobotState::Working : RobotState::Idle;
    return BatteryTransition::None;
}

const char* toString(RobotState state) {
    switch (state) {
        case RobotState::Idle:
            return "Idle";
        case RobotState::Working:
            return "Working";
        case RobotState::LowBattery:
            return "LowBattery";
        case RobotState::ReturningToCharge:
            return "ReturningToCharge";
    }
    return "Unknown";
}

}  // namespace ealr
