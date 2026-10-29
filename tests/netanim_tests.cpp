#include "ealr/robot/robot.hpp"
#include "ealr/visualization/netanim_visualization.hpp"

#include <ns3/mobility-helper.h>
#include <ns3/mobility-model.h>
#include <ns3/position-allocator.h>
#include <ns3/node-container.h>
#include <ns3/simulator.h>
#include <ns3/vector.h>

#include <cmath>
#include <filesystem>
#include <iostream>

int main() {
    ns3::NodeContainer nodes;
    nodes.Create(2);
    const ns3::Ptr<ns3::ListPositionAllocator> positions = ns3::CreateObject<ns3::ListPositionAllocator>();
    positions->Add(ns3::Vector(0.0, 0.0, 0.0));
    positions->Add(ns3::Vector(5.0, 5.0, 0.0));
    ns3::MobilityHelper mobility;
    mobility.SetPositionAllocator(positions);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    const std::filesystem::path animationPath = std::filesystem::temp_directory_path() / "ealr-netanim-test.xml";
    {
        ealr::NetAnimVisualization visualization(animationPath);
        visualization.configureGnb(nodes.Get(1)->GetId());

        ealr::Robot robot(0, {0.0, 0.0}, 100.0, 20.0, {25.0, 10.0});
        visualization.updateRobot(nodes.Get(0)->GetId(), robot);
        if (ealr::netAnimColorFor(robot.state()).green != 180) {
            std::cerr << "normal robot should be green\n";
            return 1;
        }
        robot.consumeBattery(80.0);
        visualization.updateRobot(nodes.Get(0)->GetId(), robot);
        if (ealr::netAnimColorFor(robot.state()).red != 246) {
            std::cerr << "low-battery robot should be yellow\n";
            return 1;
        }
        robot.consumeBattery(11.0);
        visualization.updateRobot(nodes.Get(0)->GetId(), robot);
        if (ealr::netAnimColorFor(robot.state()).red != 217) {
            std::cerr << "returning robot should be red\n";
            return 1;
        }

        visualization.animateReturnToCharge(nodes.Get(0)->GetId(), {10.0, 0.0}, 10.0, 0.0, 0.05);
        visualization.setStopTime(1.0);
        ns3::Simulator::Stop(ns3::Seconds(1.0));
        ns3::Simulator::Run();
    }

    const ns3::Vector finalPosition = nodes.Get(0)->GetObject<ns3::MobilityModel>()->GetPosition();
    ns3::Simulator::Destroy();
    if (!std::filesystem::exists(animationPath) || std::abs(finalPosition.x - 10.0) > 0.00001 ||
        std::abs(finalPosition.y) > 0.00001) {
        std::cerr << "NetAnim return-to-charge animation did not complete\n";
        return 1;
    }
    return 0;
}
