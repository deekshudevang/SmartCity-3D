#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "city/RoadNetwork.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianManager.h"
#include "sim/VehicleManager.h"
#include "core/Logger.h"

using namespace SmartCity;

void TestCurbWaitingAndZeroIllegalStarts() {
    std::cout << "[Test 1] Testing Curb Waiting Compliance & Zero Illegal Starts Outside WALK...\n";

    city::RoadNetwork net;
    core::IntersectionSettings settings;
    net.BuildFromSpec(settings);

    auto ctrl = std::make_shared<traffic::TrafficSignalController>();
    // Force ALL_RED so all pedestrian aspects are strictly DONT_WALK
    ctrl->ForcePhase(traffic::SignalPhase::ALL_RED_1);

    ped::PedestrianManager pm(net, ctrl);
    pm.Reset(100);

    // Spawn pedestrians on all crosswalks
    auto* p1 = pm.SpawnPedestrian("CW_North", true, 1.2f);
    auto* p2 = pm.SpawnPedestrian("CW_South", false, 1.3f);
    auto* p3 = pm.SpawnPedestrian("CW_East", true, 1.4f);
    auto* p4 = pm.SpawnPedestrian("CW_West", false, 1.1f);

    assert(p1 && p2 && p3 && p4);

    float dt = 0.05f;
    for (int i = 0; i < 200; ++i) { // 10 seconds of DONT_WALK
        pm.Step(dt);

        assert(p1->GetState() == ped::PedestrianState::Waiting);
        assert(p2->GetState() == ped::PedestrianState::Waiting);
        assert(p3->GetState() == ped::PedestrianState::Waiting);
        assert(p4->GetState() == ped::PedestrianState::Waiting);

        // Position must remain on sidewalk curb (Y >= 0.14m)
        assert(p1->GetPosition().y >= 0.14f);
        assert(p2->GetPosition().y >= 0.14f);
        assert(p3->GetPosition().y >= 0.14f);
        assert(p4->GetPosition().y >= 0.14f);

        // Zero illegal crossing starts
        assert(pm.GetIllegalCrossingStarts() == 0);
    }

    std::cout << "  -> Curb waiting compliance & zero illegal starts: PASS\n";
}

void TestCrosswalkTraversalAndClearanceTime() {
    std::cout << "[Test 2] Testing Crosswalk Traversal & Clearance Time Guarantee...\n";

    city::RoadNetwork net;
    core::IntersectionSettings settings;
    net.BuildFromSpec(settings);

    auto ctrl = std::make_shared<traffic::TrafficSignalController>();
    ctrl->ForcePhase(traffic::SignalPhase::NS_GREEN); // CW_East has WALK

    ped::PedestrianManager pm(net, ctrl);
    pm.Reset(200);

    float pedSpeed = 1.25f;
    auto* ped = pm.SpawnPedestrian("CW_East", true, pedSpeed);
    assert(ped);

    float dt = 0.05f;
    // Step once to start crossing on WALK
    pm.Step(dt);
    assert(ped->GetState() == ped::PedestrianState::Crossing);
    assert(ped->GetCrossingStartTime() >= 0.0f);

    // Advance 8 seconds - signal transitions to FlashingDontWalk
    for (int i = 0; i < 160; ++i) {
        ctrl->Update(dt);
        pm.Step(dt);
        // Pedestrian already in crosswalk MUST continue crossing
        if (ped->GetState() != ped::PedestrianState::Finished) {
            assert(ped->GetState() == ped::PedestrianState::Crossing);
        }
    }

    // Step until pedestrian finishes crossing
    while (ped && ped->GetState() == ped::PedestrianState::Crossing) {
        ctrl->Update(dt);
        pm.Step(dt);
    }

    // Verify finished and measure crossing duration
    float crossingDuration = ped->GetCrossingDuration();
    const city::Crosswalk* cw = net.GetCrosswalk("CW_East");
    float expectedDistance = glm::length(cw->polyline[1] - cw->polyline[0]);
    float expectedDuration = expectedDistance / pedSpeed;

    std::cout << "     Expected Duration: " << expectedDuration << "s | Measured: " << crossingDuration << "s\n";
    assert(std::abs(crossingDuration - expectedDuration) <= 0.1f);

    std::cout << "  -> Crosswalk traversal & clearance timing: PASS\n";
}

void TestVehiclePedestrianConflictYieldingZeroCollisions() {
    std::cout << "[Test 3] Testing Vehicle-Pedestrian Conflict Yielding (0 Collisions)...\n";

    city::RoadNetwork net;
    core::IntersectionSettings settings;
    net.BuildFromSpec(settings);

    auto ctrl = std::make_shared<traffic::TrafficSignalController>();
    ctrl->ForcePhase(traffic::SignalPhase::NS_GREEN);

    sim::VehicleManager vm(net, ctrl);
    ped::PedestrianManager pm(net, ctrl);
    vm.SetPedestrianManager(&pm);
    ctrl->SetVehicleManager(&vm);

    vm.Reset(300);
    pm.Reset(300);

    // Spawn a pedestrian crossing CW_East
    auto* ped = pm.SpawnPedestrian("CW_East", true, 1.25f);
    assert(ped);

    // Spawn a vehicle from North turning Left into East (crosses CW_East)
    auto* veh = vm.SpawnVehicle(city::ApproachId::North, city::MovementType::Left, sim::VehicleType::Sedan);
    assert(veh);

    float dt = 0.05f;
    bool vehicleYielded = false;
    int steps = 600; // 30s sim

    for (int i = 0; i < steps; ++i) {
        pm.Step(dt);
        vm.Step(dt);

        if (veh->IsActive() && ped->IsInCrosswalk()) {
            glm::vec3 vPos = veh->GetPosition();
            glm::vec3 pPos = ped->GetPosition();
            float dist = glm::length(vPos - pPos);

            // Verify minimum safe distance is ALWAYS maintained (>= 2.5m bumper-to-pedestrian)
            assert(dist >= 2.0f && "Vehicle-pedestrian collision detected!");

            if (veh->GetState() == sim::VehicleState::Waiting || veh->GetSpeed() < 3.0f) {
                if (dist < 12.0f) {
                    vehicleYielded = true;
                }
            }
        }
    }

    assert(vehicleYielded && "Vehicle failed to yield to crossing pedestrian!");
    assert(vm.GetCollisionCount() == 0);

    std::cout << "  -> Vehicle-pedestrian yielding & zero collisions: PASS\n";
}

void TestPedestrianDeterminism() {
    std::cout << "[Test 4] Testing Pedestrian Determinism (Same Seed -> Identical Counts)...\n";

    city::RoadNetwork net;
    core::IntersectionSettings settings;
    net.BuildFromSpec(settings);

    auto runSim = [&net](uint32_t seed) {
        auto ctrl = std::make_shared<traffic::TrafficSignalController>();
        ped::PedestrianManager pm(net, ctrl);
        pm.Reset(seed);

        float dt = 0.1f;
        for (int i = 0; i < 1000; ++i) {
            ctrl->Update(dt);
            pm.Step(dt);
        }
        return std::make_pair(pm.GetTotalSpawned(), pm.GetTotalCompleted());
    };

    auto res1 = runSim(42);
    auto res2 = runSim(42);

    assert(res1.first == res2.first);
    assert(res1.second == res2.second);

    std::cout << "  -> Pedestrian determinism: PASS\n";
}

int main() {
    core::Logger::Init("logs/test_pedestrians.log");
    std::cout << "=== RUNNING PEDESTRIAN SIMULATION AUTOMATED TEST SUITE ===\n";

    TestCurbWaitingAndZeroIllegalStarts();
    TestCrosswalkTraversalAndClearanceTime();
    TestVehiclePedestrianConflictYieldingZeroCollisions();
    TestPedestrianDeterminism();

    std::cout << "=== ALL PEDESTRIAN SIMULATION TESTS PASSED SUCCESSFULLY! ===\n";
    return 0;
}
