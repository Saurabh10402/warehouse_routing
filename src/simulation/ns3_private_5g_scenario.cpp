#include "ealr/simulation/ns3_private_5g_scenario.hpp"

#include "ealr/robot/robot.hpp"
#include "ealr/visualization/netanim_visualization.hpp"

#ifdef EALR_HAS_5G_LENA
#include "nr_access_network.hpp"
#endif

#include <ns3/double.h>
#include <ns3/mobility-helper.h>
#include <ns3/mobility-model.h>
#include <ns3/node-container.h>
#include <ns3/pointer.h>
#include <ns3/position-allocator.h>
#include <ns3/random-variable-stream.h>
#include <ns3/rng-seed-manager.h>
#include <ns3/simulator.h>
#include <ns3/string.h>
#include <ns3/vector.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace ealr {
namespace {

vector<Position2D> gridPositions(const ExperimentConfiguration& configuration) {
    const uint32_t columns = static_cast<uint32_t>(ceil(sqrt(configuration.robots.count)));
    const uint32_t rows = static_cast<uint32_t>(ceil(static_cast<double>(configuration.robots.count) / columns));
    vector<Position2D> positions;
    positions.reserve(configuration.robots.count);

    for (uint32_t robotIndex = 0; robotIndex < configuration.robots.count; ++robotIndex) {
        const uint32_t column = robotIndex % columns;
        const uint32_t row = robotIndex / columns;
        positions.push_back({
            configuration.mobility.factoryWidthMeters * static_cast<double>(column + 1) / static_cast<double>(columns + 1),
            configuration.mobility.factoryHeightMeters * static_cast<double>(row + 1) / static_cast<double>(rows + 1)
        });
    }
    return positions;
}

void installRobotMobility(const ExperimentConfiguration& configuration,
                          const ns3::NodeContainer& robotNodes) {
    ns3::MobilityHelper mobility;
    if (configuration.mobility.model == MobilityModel::RandomWaypoint) {
        const ns3::Ptr<ns3::RandomRectanglePositionAllocator> waypoints = ns3::CreateObject<ns3::RandomRectanglePositionAllocator>();
        waypoints->SetAttribute("X", ns3::StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + to_string(configuration.mobility.factoryWidthMeters) + "]"));
        waypoints->SetAttribute("Y", ns3::StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + to_string(configuration.mobility.factoryHeightMeters) + "]"));

        const ns3::Ptr<ns3::UniformRandomVariable> speed = ns3::CreateObject<ns3::UniformRandomVariable>();
        speed->SetAttribute("Min", ns3::DoubleValue(configuration.mobility.randomWaypointMinSpeedMetersPerSecond));
        speed->SetAttribute("Max", ns3::DoubleValue(configuration.mobility.randomWaypointMaxSpeedMetersPerSecond));
        const ns3::Ptr<ns3::ConstantRandomVariable> pause = ns3::CreateObject<ns3::ConstantRandomVariable>();
        pause->SetAttribute("Constant", ns3::DoubleValue(configuration.mobility.randomWaypointPauseSeconds));

        mobility.SetPositionAllocator(waypoints);
        mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                  "Speed", ns3::PointerValue(speed),
                                  "Pause", ns3::PointerValue(pause),
                                  "PositionAllocator", ns3::PointerValue(waypoints));
    } else {
        const vector<Position2D> positions = gridPositions(configuration);
        const ns3::Ptr<ns3::ListPositionAllocator> allocator = ns3::CreateObject<ns3::ListPositionAllocator>();
        for (const Position2D& position : positions) {
            allocator->Add(ns3::Vector(position.x, position.y, 0.0));
        }
        mobility.SetPositionAllocator(allocator);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }
    mobility.Install(robotNodes);
}

void installGnbMobility(const Private5gConfiguration& configuration, const ns3::NodeContainer& gnbNodes) {
    const ns3::Ptr<ns3::ListPositionAllocator> allocator = ns3::CreateObject<ns3::ListPositionAllocator>();
    allocator->Add(ns3::Vector(configuration.gnbX, configuration.gnbY, 0.0));

    ns3::MobilityHelper mobility;
    mobility.SetPositionAllocator(allocator);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(gnbNodes);
}

}  // namespace

Ns3Private5gScenario::Ns3Private5gScenario(ExperimentConfiguration configuration)
    : configuration_(std::move(configuration)) {
}

Ns3ScenarioResult Ns3Private5gScenario::run(const filesystem::path& animationPath) const {
    if (animationPath.empty()) {
        throw invalid_argument("NetAnim output path must not be empty");
    }
    if (!animationPath.parent_path().empty()) {
        filesystem::create_directories(animationPath.parent_path());
    }

    ns3::NodeContainer robotNodes;
    ns3::RngSeedManager::SetSeed(configuration_.simulation.seed);
    robotNodes.Create(configuration_.robots.count);
    ns3::NodeContainer gnbNodes;
    gnbNodes.Create(1);

    installRobotMobility(configuration_, robotNodes);
    installGnbMobility(configuration_.private5g, gnbNodes);

#ifdef EALR_HAS_5G_LENA
    const NrAccessNetwork accessNetwork = installNrAccessNetwork(configuration_.private5g, gnbNodes, robotNodes);
#endif

    NetAnimVisualization visualization(animationPath);
    visualization.configureGnb(gnbNodes.Get(0)->GetId());
    for (uint32_t robotIndex = 0; robotIndex < robotNodes.GetN(); ++robotIndex) {
        const ns3::Ptr<ns3::Node> robot = robotNodes.Get(robotIndex);
        const ns3::Vector position = robot->GetObject<ns3::MobilityModel>()->GetPosition();
        const Robot robotModel(robotIndex,
                               {position.x, position.y},
                               configuration_.battery.initialPercent,
                               configuration_.robots.communicationRadiusMeters,
                               {configuration_.battery.lowBatteryThreshold,
                                configuration_.battery.criticalBatteryThreshold});
        visualization.updateRobot(robot->GetId(), robotModel);
    }
    visualization.setStopTime(configuration_.simulation.durationSeconds);

    ns3::Simulator::Stop(ns3::Seconds(configuration_.simulation.durationSeconds));
    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

#ifdef EALR_HAS_5G_LENA
    static_cast<void>(accessNetwork);
#endif
    return {configuration_.robots.count, has5gLenaNrSupport(), animationPath};
}

bool Ns3Private5gScenario::has5gLenaNrSupport() {
#ifdef EALR_HAS_5G_LENA
    return true;
#else
    return false;
#endif
}

}  // namespace ealr
