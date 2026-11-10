#include "sim/TrafficManager.h"
#include "core/Logger.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace SmartCity::sim;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_collision_avoidance.log");
    Logger::Info("Starting test_collision_avoidance...");

    SimulationSettings simSettings;
    simSettings.fixedTimestep = 0.0166666667;
    simSettings.northSpawnRate = 0.0f; // Disable auto-spawning to control scenario
    simSettings.southSpawnRate = 0.0f;
    simSettings.eastSpawnRate = 0.0f;
    simSettings.westSpawnRate = 0.0f;

    IntersectionSettings interSettings;
    interSettings.approachLength = 80.0f;
    interSettings.stopLineDistance = 13.0f;

    TrafficManager traffic(simSettings, interSettings);

    // 1. Spawn a platoon of 4 vehicles on North Straight approach (Traj 0)
    Vehicle* v1 = traffic.SpawnVehicle(VehicleType::Sedan, ApproachDirection::North, TurnManeuver::Straight);
    assert(v1 != nullptr);
    v1->SetDistanceAlongPath(45.0f);
    v1->SetSpeed(10.0f);

    Vehicle* v2 = traffic.SpawnVehicle(VehicleType::SUV, ApproachDirection::North, TurnManeuver::Straight);
    assert(v2 != nullptr);
    v2->SetDistanceAlongPath(30.0f);
    v2->SetSpeed(12.0f);

    Vehicle* v3 = traffic.SpawnVehicle(VehicleType::Bus, ApproachDirection::North, TurnManeuver::Straight);
    assert(v3 != nullptr);
    v3->SetDistanceAlongPath(15.0f);
    v3->SetSpeed(11.0f);

    Vehicle* v4 = traffic.SpawnVehicle(VehicleType::Sedan, ApproachDirection::North, TurnManeuver::Straight);
    assert(v4 != nullptr);
    v4->SetDistanceAlongPath(0.0f);
    v4->SetSpeed(12.0f);

    Logger::Info("Spawned 4-vehicle platoon on North approach heading towards RED signal.");

    // 2. Simulate 800 steps (13.33 seconds) under RED signal (nsSignal = 0)
    float dt = static_cast<float>(simSettings.fixedTimestep);
    float minObservedGap = 1e9f;

    for (int step = 0; step < 800; ++step) {
        // Red signal for North-South, Red for East-West
        traffic.Update(dt, 0, 0, 0, 0);

        // Check for collisions between all consecutive pairs
        const auto& vehicles = traffic.GetVehicles();
        for (size_t i = 0; i < vehicles.size(); ++i) {
            for (size_t j = i + 1; j < vehicles.size(); ++j) {
                if (vehicles[i]->GetTrajectoryId() == vehicles[j]->GetTrajectoryId()) {
                    float distI = vehicles[i]->GetDistanceAlongPath();
                    float distJ = vehicles[j]->GetDistanceAlongPath();
                    float leaderDist = std::max(distI, distJ);
                    float followerDist = std::min(distI, distJ);
                    const Vehicle* leader = (distI > distJ) ? vehicles[i].get() : vehicles[j].get();

                    float bumperGap = (leaderDist - followerDist) - leader->GetDimensions().length;
                    minObservedGap = std::min(minObservedGap, bumperGap);

                    // Must never have collision (bumperGap < 0)
                    assert(bumperGap > 0.0f);
                }
            }
        }
    }

    Logger::Info("Platoon stopped. Minimum observed bumper gap: ", minObservedGap, " m (Zero collisions recorded).");

    // Verify all vehicles stopped safely before the stop line (~67m)
    for (const auto& v : traffic.GetVehicles()) {
        assert(v->GetDistanceAlongPath() <= 67.5f);
        assert(v->GetSpeed() < 0.1f); // Completely stopped
    }
    Logger::Info("Assertion passed: All platoon vehicles halted at red signal before stop line.");

    // 3. Switch signal to GREEN (nsSignal = 2) and simulate 600 steps
    Logger::Info("Switching signal to GREEN (nsSignal = 2)...");
    for (int step = 0; step < 600; ++step) {
        traffic.Update(dt, 2, 0, 0, 0);

        const auto& vehicles = traffic.GetVehicles();
        for (size_t i = 0; i < vehicles.size(); ++i) {
            for (size_t j = i + 1; j < vehicles.size(); ++j) {
                if (vehicles[i]->GetTrajectoryId() == vehicles[j]->GetTrajectoryId()) {
                    float distI = vehicles[i]->GetDistanceAlongPath();
                    float distJ = vehicles[j]->GetDistanceAlongPath();
                    float leaderDist = std::max(distI, distJ);
                    float followerDist = std::min(distI, distJ);
                    const Vehicle* leader = (distI > distJ) ? vehicles[i].get() : vehicles[j].get();

                    float bumperGap = (leaderDist - followerDist) - leader->GetDimensions().length;
                    assert(bumperGap > 0.0f);
                }
            }
        }
    }

    // Lead vehicle should have moved into/through intersection
    assert(v1->GetDistanceAlongPath() > 70.0f);
    Logger::Info("Assertion passed: Green wave discharge successful without collisions.");

    // 4. Test Emergency Vehicle Preemption Detection
    traffic.Clear();
    Vehicle* ev = traffic.SpawnVehicle(VehicleType::Emergency, ApproachDirection::South, TurnManeuver::Straight);
    assert(ev != nullptr);
    ev->SetDistanceAlongPath(20.0f);
    ev->SetSpeed(15.0f);

    traffic.Update(dt, 0, 0, 0, 0);
    assert(traffic.IsEmergencyActive());
    assert(traffic.GetEmergencyApproach() == static_cast<int>(ApproachDirection::South));
    Logger::Info("Assertion passed: Emergency vehicle preemption triggered correctly.");

    Logger::Info("test_collision_avoidance PASSED successfully.");
    Logger::Shutdown();
    return 0;
}

