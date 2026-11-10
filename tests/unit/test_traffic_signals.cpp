#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include "traffic/TrafficSignalController.h"
#include "sim/VehicleManager.h"
#include "city/RoadNetwork.h"
#include "core/Logger.h"

using namespace SmartCity;

void TestTransitionCorrectness() {
    std::cout << "[Test 1] Testing FSM Transition Table & Exact Durations...\n";

    // 1. Validate allowed transitions
    assert(traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::NS_GREEN,  traffic::SignalPhase::NS_YELLOW));
    assert(traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::NS_YELLOW, traffic::SignalPhase::ALL_RED_1));
    assert(traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::ALL_RED_1, traffic::SignalPhase::EW_GREEN));
    assert(traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::EW_GREEN,  traffic::SignalPhase::EW_YELLOW));
    assert(traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::EW_YELLOW, traffic::SignalPhase::ALL_RED_2));
    assert(traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::ALL_RED_2, traffic::SignalPhase::NS_GREEN));

    // 2. Validate illegal transitions
    assert(!traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::NS_GREEN,  traffic::SignalPhase::EW_GREEN));
    assert(!traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::NS_GREEN,  traffic::SignalPhase::ALL_RED_1));
    assert(!traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::NS_YELLOW, traffic::SignalPhase::EW_GREEN));
    assert(!traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::ALL_RED_1, traffic::SignalPhase::NS_GREEN));
    assert(!traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::EW_GREEN,  traffic::SignalPhase::NS_GREEN));
    assert(!traffic::TrafficSignalController::IsValidTransition(traffic::SignalPhase::EW_YELLOW, traffic::SignalPhase::NS_GREEN));

    // 3. Exact durations in fixed-time mode
    traffic::SignalConfig cfg;
    cfg.mode = traffic::SignalMode::FixedTime;
    cfg.fixedGreenNS = 20.0f;
    cfg.yellowDuration = 3.0f;
    cfg.allRedClearance = 2.0f;
    cfg.fixedGreenEW = 15.0f;

    traffic::TrafficSignalController ctrl(cfg);
    ctrl.EnableCsvLogging("logs/signals_test_transitions.csv");

    float dt = 0.1f;
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_GREEN);

    // Run until NS green expires at 20.0s
    for (int i = 0; i < 200; ++i) ctrl.Update(dt);
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_YELLOW);

    // Run until yellow expires at 3.0s
    for (int i = 0; i < 30; ++i) ctrl.Update(dt);
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::ALL_RED_1);

    // Run until all-red expires at 2.0s
    for (int i = 0; i < 20; ++i) ctrl.Update(dt);
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::EW_GREEN);

    // Run until EW green expires at 15.0s
    for (int i = 0; i < 150; ++i) ctrl.Update(dt);
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::EW_YELLOW);

    // Run until yellow expires at 3.0s
    for (int i = 0; i < 30; ++i) ctrl.Update(dt);
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::ALL_RED_2);

    // Run until all-red expires at 2.0s
    for (int i = 0; i < 20; ++i) ctrl.Update(dt);
    assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_GREEN);

    std::cout << "  -> FSM transition correctness & exact timing: PASS\n";
}

void TestConflictSafetyMatrix() {
    std::cout << "[Test 2] Testing Conflict Safety Matrix (1-Hour Simulation, 0 Conflicts Asserted)...\n";

    traffic::SignalConfig cfg;
    cfg.mode = traffic::SignalMode::FixedTime;
    cfg.fixedGreenNS = 24.0f;
    cfg.yellowDuration = 3.0f;
    cfg.allRedClearance = 2.0f;
    cfg.fixedGreenEW = 20.0f;

    traffic::TrafficSignalController ctrl(cfg);
    ctrl.EnableCsvLogging("logs/signals_test_conflict.csv");

    float dt = 0.05f;
    int steps = static_cast<int>(3600.0f / dt); // 72,000 steps

    for (int i = 0; i < steps; ++i) {
        ctrl.Update(dt);
        // AssertConflictSafety() is called inside ctrl.Update(dt).
        // It asserts that conflicting through greens, left greens, or crosswalk walks never occur simultaneously.
        assert(!ctrl.HasConflictViolation());
    }

    std::cout << "  -> 1-hour conflict safety (72,000 steps, 0 conflicts): PASS\n";
}

void TestAdaptiveResponsivenessAndStarvation() {
    std::cout << "[Test 3] Testing Adaptive Responsiveness & Starvation Protection...\n";

    city::RoadNetwork net;
    core::IntersectionSettings settings;
    net.BuildFromSpec(settings);

    // Scenario 1: Asymmetric Demand - Heavy NS, Light EW
    {
        auto ctrl = std::make_shared<traffic::TrafficSignalController>();
        ctrl->SetMode(traffic::SignalMode::Adaptive);

        sim::VehicleManager vm(net, ctrl);
        ctrl->SetVehicleManager(&vm);

        // Configure demand: heavy NS (1200 arr/hr), light EW (120 arr/hr)
        sim::SimEngineConfig cfg = vm.GetConfig();
        cfg.demand.northArrivalsPerHour = 1200.0f;
        cfg.demand.southArrivalsPerHour = 1200.0f;
        cfg.demand.eastArrivalsPerHour  = 120.0f;
        cfg.demand.westArrivalsPerHour  = 120.0f;
        vm.SetConfig(cfg);

        vm.Reset(1001);
        ctrl->EnableCsvLogging("logs/signals_test_adaptive_ns_heavy.csv");

        float nsTotalGreen = 0.0f;
        float ewTotalGreen = 0.0f;

        float dt = 0.1f;
        for (int i = 0; i < 3000; ++i) { // 300s sim
            vm.Step(dt);

            traffic::SignalPhase p = ctrl->GetCurrentPhase();
            if (p == traffic::SignalPhase::NS_GREEN) nsTotalGreen += dt;
            if (p == traffic::SignalPhase::EW_GREEN) ewTotalGreen += dt;
        }

        std::cout << "     [Heavy NS] NS Green: " << nsTotalGreen << "s | EW Green: " << ewTotalGreen << "s\n";
        assert(nsTotalGreen > ewTotalGreen * 1.3f);
    }

    // Scenario 2: Reverse Demand - Light NS, Heavy EW
    {
        auto ctrl = std::make_shared<traffic::TrafficSignalController>();
        ctrl->SetMode(traffic::SignalMode::Adaptive);

        sim::VehicleManager vm(net, ctrl);
        ctrl->SetVehicleManager(&vm);

        sim::SimEngineConfig cfg = vm.GetConfig();
        cfg.demand.northArrivalsPerHour = 120.0f;
        cfg.demand.southArrivalsPerHour = 120.0f;
        cfg.demand.eastArrivalsPerHour  = 1200.0f;
        cfg.demand.westArrivalsPerHour  = 1200.0f;
        vm.SetConfig(cfg);

        vm.Reset(2002);
        ctrl->EnableCsvLogging("logs/signals_test_adaptive_ew_heavy.csv");

        float nsTotalGreen = 0.0f;
        float ewTotalGreen = 0.0f;

        float dt = 0.1f;
        for (int i = 0; i < 3000; ++i) {
            vm.Step(dt);

            traffic::SignalPhase p = ctrl->GetCurrentPhase();
            if (p == traffic::SignalPhase::NS_GREEN) nsTotalGreen += dt;
            if (p == traffic::SignalPhase::EW_GREEN) ewTotalGreen += dt;
        }

        std::cout << "     [Heavy EW] NS Green: " << nsTotalGreen << "s | EW Green: " << ewTotalGreen << "s\n";
        assert(ewTotalGreen > nsTotalGreen * 1.3f);
    }

    // Scenario 3: Starvation limit
    {
        auto ctrl = std::make_shared<traffic::TrafficSignalController>();
        ctrl->SetMode(traffic::SignalMode::Adaptive);

        sim::VehicleManager vm(net, ctrl);
        ctrl->SetVehicleManager(&vm);

        // Fill NS and keep 1 vehicle waiting on East
        vm.Reset(3003);
        vm.SpawnVehicle(city::ApproachId::East, city::MovementType::Through, sim::VehicleType::Sedan);

        // Run until starvation triggers (max 60s red)
        float maxRedWait = 0.0f;
        float dt = 0.1f;
        for (int i = 0; i < 1000; ++i) {
            // Keep adding vehicles to North so it doesn't gap out
            if (i % 20 == 0) {
                vm.SpawnVehicle(city::ApproachId::North, city::MovementType::Through, sim::VehicleType::Sedan);
            }
            vm.Step(dt);

            if (ctrl->GetCurrentPhase() == traffic::SignalPhase::EW_GREEN) {
                break;
            }
            maxRedWait += dt;
        }

        std::cout << "     [Starvation Limit] Opposing approach waited: " << maxRedWait << "s (Cap <= 65s)\n";
        assert(maxRedWait <= 65.0f);
    }

    std::cout << "  -> Adaptive responsiveness & starvation protection: PASS\n";
}

void TestEmergencyPreemption() {
    std::cout << "[Test 4] Testing Emergency Preemption Scenarios...\n";

    // Scenario (a): Ambulance on green approach -> hold green
    {
        traffic::SignalConfig cfg;
        cfg.minGreen = 10.0f;
        cfg.maxGreen = 40.0f;
        cfg.fixedGreenNS = 20.0f;
        cfg.emergencyDetectionDist = 100.0f;

        traffic::TrafficSignalController ctrl(cfg);
        ctrl.EnableCsvLogging("logs/signals_test_emergency_a.csv");

        float dt = 0.1f;
        // Run to 12s on NS_GREEN
        for (int i = 0; i < 120; ++i) ctrl.Update(dt);
        assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_GREEN);

        // Ambulance requests priority on North approach (currently green)
        ctrl.RegisterEmergencyVehicle(999, city::ApproachId::North, 50.0f);

        // Advance past original 20s duration
        for (int i = 0; i < 150; ++i) ctrl.Update(dt); // at t = 27.0s
        // Green is held!
        assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_GREEN);

        // Clear request
        ctrl.UnregisterEmergencyVehicle(999);
    }

    // Scenario (b): Ambulance on red approach just after switch -> service in bounded delay
    {
        traffic::SignalConfig cfg;
        cfg.minGreen = 10.0f;
        cfg.yellowDuration = 3.0f;
        cfg.allRedClearance = 2.0f;

        traffic::TrafficSignalController ctrl(cfg);
        ctrl.EnableCsvLogging("logs/signals_test_emergency_b.csv");

        float dt = 0.1f;
        // Step to t = 1.0s (just after NS green started)
        for (int i = 0; i < 10; ++i) ctrl.Update(dt);
        assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_GREEN);

        // Ambulance arrives on East approach (which has RED)
        float reqTime = ctrl.GetSimTime();
        ctrl.RegisterEmergencyVehicle(888, city::ApproachId::East, 60.0f);

        // Step until EW_GREEN turns green
        float serviceTime = -1.0f;
        for (int i = 0; i < 300; ++i) {
            ctrl.Update(dt);
            if (ctrl.GetCurrentPhase() == traffic::SignalPhase::EW_GREEN) {
                serviceTime = ctrl.GetSimTime();
                break;
            }
        }

        assert(serviceTime > 0.0f);
        float delay = serviceTime - reqTime;
        std::cout << "     [Emergency Red Approach] Measured Service Delay: " << delay << "s (Expected <= 15.0s)\n";
        assert(delay >= 13.9f && delay <= 15.1f); // 9s remaining min green + 3s yellow + 2s all-red = 14s
    }

    // Scenario (c): Simultaneous ambulances on conflicting approaches -> first-come tie-break
    {
        traffic::SignalConfig cfg;
        cfg.minGreen = 10.0f;
        cfg.yellowDuration = 3.0f;
        cfg.allRedClearance = 2.0f;

        traffic::TrafficSignalController ctrl(cfg);
        ctrl.EnableCsvLogging("logs/signals_test_emergency_c.csv");

        // Ambulance 1 on North (distance 40m), Ambulance 2 on East (distance 70m)
        ctrl.RegisterEmergencyVehicle(101, city::ApproachId::North, 40.0f);
        ctrl.RegisterEmergencyVehicle(102, city::ApproachId::East, 70.0f);

        // First served is North (NS_GREEN)
        assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::NS_GREEN);
        float dt = 0.1f;
        for (int i = 0; i < 200; ++i) ctrl.Update(dt);

        // Once Ambulance 1 clears:
        ctrl.UnregisterEmergencyVehicle(101);

        // Controller sequences to serve Ambulance 2 on East
        bool servedEast = false;
        for (int i = 0; i < 200; ++i) {
            ctrl.Update(dt);
            if (ctrl.GetCurrentPhase() == traffic::SignalPhase::EW_GREEN) {
                servedEast = true;
                break;
            }
        }
        assert(servedEast);
        ctrl.UnregisterEmergencyVehicle(102);
    }

    // Scenario (d): Ambulance during all-red -> clearance duration honoured
    {
        traffic::SignalConfig cfg;
        cfg.fixedGreenNS = 10.0f;
        cfg.yellowDuration = 3.0f;
        cfg.allRedClearance = 2.0f;

        traffic::TrafficSignalController ctrl(cfg);
        ctrl.EnableCsvLogging("logs/signals_test_emergency_d.csv");

        float dt = 0.1f;
        // Step into ALL_RED_1 (t = 13.1s)
        for (int i = 0; i < 132; ++i) ctrl.Update(dt);
        assert(ctrl.GetCurrentPhase() == traffic::SignalPhase::ALL_RED_1);

        // Ambulance arrives on North during all-red
        ctrl.RegisterEmergencyVehicle(333, city::ApproachId::North, 50.0f);

        // All-red MUST complete its remaining duration (about 1.8s) before next phase opens
        int allRedSteps = 0;
        while (ctrl.GetCurrentPhase() == traffic::SignalPhase::ALL_RED_1) {
            ctrl.Update(dt);
            allRedSteps++;
        }
        assert(allRedSteps >= 18); // ~1.8s honoured
    }

    std::cout << "  -> Emergency preemption scenarios (a, b, c, d): PASS\n";
}

void TestDeterminism() {
    std::cout << "[Test 5] Testing Signal Log Determinism (Identical Seeds)...\n";

    city::RoadNetwork net;
    core::IntersectionSettings settings;
    net.BuildFromSpec(settings);

    auto runSim = [&net](uint32_t seed, const std::string& logPath) {
        auto ctrl = std::make_shared<traffic::TrafficSignalController>();
        ctrl->SetMode(traffic::SignalMode::Adaptive);
        ctrl->EnableCsvLogging(logPath);

        sim::VehicleManager vm(net, ctrl);
        ctrl->SetVehicleManager(&vm);

        vm.Reset(seed);

        float dt = 0.1f;
        for (int i = 0; i < 2000; ++i) {
            vm.Step(dt);
        }
        return ctrl->GetTransitionHistory();
    };

    auto hist1 = runSim(4242, "logs/signals_det_run1.csv");
    auto hist2 = runSim(4242, "logs/signals_det_run2.csv");

    assert(hist1.size() == hist2.size());
    for (size_t i = 0; i < hist1.size(); ++i) {
        assert(hist1[i].timestamp == hist2[i].timestamp);
        assert(hist1[i].prevPhase == hist2[i].prevPhase);
        assert(hist1[i].newPhase == hist2[i].newPhase);
        assert(hist1[i].trigger == hist2[i].trigger);
        assert(hist1[i].nsQueue == hist2[i].nsQueue);
        assert(hist1[i].ewQueue == hist2[i].ewQueue);
    }

    std::cout << "  -> Determinism (identical seeds -> identical log): PASS\n";
}

int main() {
    core::Logger::Init("logs/test_traffic_signals.log");
    std::cout << "=== RUNNING TRAFFIC SIGNAL CONTROLLER AUTOMATED TEST SUITE ===\n";

    TestTransitionCorrectness();
    TestConflictSafetyMatrix();
    TestAdaptiveResponsivenessAndStarvation();
    TestEmergencyPreemption();
    TestDeterminism();

    std::cout << "=== ALL TRAFFIC SIGNAL CONTROLLER TESTS PASSED SUCCESSFULLY! ===\n";
    return 0;
}
