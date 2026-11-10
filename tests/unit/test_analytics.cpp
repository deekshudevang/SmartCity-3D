#include <cassert>
#include <iostream>
#include <memory>
#include <cmath>
#include "analytics/AnalyticsEngine.h"
#include "city/RoadNetwork.h"
#include "sim/VehicleManager.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianManager.h"

using namespace SmartCity;

void Test_EmptySystemMetrics() {
    std::cout << "[Test 1] Empty System Metrics (Validation and N/A checks)..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    auto sigCtrl = std::make_shared<traffic::TrafficSignalController>();
    ped::PedestrianManager pedMgr(net, sigCtrl);
    sim::VehicleManager vehMgr(net, sigCtrl);

    analytics::AnalyticsEngine engine(vehMgr, *sigCtrl, pedMgr);

    // Initial state before any simulation step
    const auto& snap = engine.GetSnapshot();
    assert(snap.activeVehicles == 0);
    assert(snap.waitingVehicles == 0);
    assert(!snap.speedValid && "Speed must be invalid when no vehicles exist!");
    assert(!snap.liveWaitValid && "Live wait must be invalid when no vehicles exist!");
    assert(!snap.tripWaitValid && "Trip wait must be invalid when no trips completed!");
    assert(!snap.rollingThroughputValid && "Rolling throughput must be invalid during first 5 seconds!");
    assert(!snap.cumulativeThroughputValid && "Cumulative throughput must be invalid during first 5 seconds!");

    std::cout << "  PASS: Empty system metrics correctly return invalid / N/A state without division by zero." << std::endl;
}

void Test_ScriptedStateMetrics() {
    std::cout << "[Test 2] Scripted State Metrics (Known Vehicles, Speeds & Queues)..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    auto sigCtrl = std::make_shared<traffic::TrafficSignalController>();
    ped::PedestrianManager pedMgr(net, sigCtrl);
    sim::VehicleManager vehMgr(net, sigCtrl);

    analytics::AnalyticsEngine engine(vehMgr, *sigCtrl, pedMgr);

    // Spawn 5 vehicles manually on North approach, advancing each so entrance has clearance
    auto* v1 = vehMgr.SpawnVehicle(city::ApproachId::North, city::MovementType::Through, sim::VehicleType::Sedan);
    assert(v1);
    float stopDist = v1->GetRoute().GetStopLineDistance(); // ~120m
    v1->SetDistanceAlongRoute(stopDist - 2.0f);
    v1->SetSpeed(0.0f);

    auto* v2 = vehMgr.SpawnVehicle(city::ApproachId::North, city::MovementType::Through, sim::VehicleType::Sedan);
    assert(v2);
    v2->SetDistanceAlongRoute(stopDist - 9.0f); // 7m gap
    v2->SetSpeed(0.0f);

    auto* v3 = vehMgr.SpawnVehicle(city::ApproachId::North, city::MovementType::Through, sim::VehicleType::Sedan);
    assert(v3);
    v3->SetDistanceAlongRoute(stopDist - 16.0f); // 7m gap
    v3->SetSpeed(0.0f);

    auto* v4 = vehMgr.SpawnVehicle(city::ApproachId::North, city::MovementType::Through, sim::VehicleType::Sedan);
    assert(v4);
    v4->SetDistanceAlongRoute(30.0f);
    v4->SetSpeed(10.0f);

    auto* v5 = vehMgr.SpawnVehicle(city::ApproachId::North, city::MovementType::Through, sim::VehicleType::Sedan);
    assert(v5);
    v5->SetDistanceAlongRoute(10.0f);
    v5->SetSpeed(14.0f);

    // Step analytics engine
    engine.Step(1.0f / 60.0f);

    const auto& snap = engine.GetSnapshot();
    assert(snap.activeVehicles == 5);
    assert(snap.waitingVehicles == 3);

    // Speed checks: (0 + 0 + 0 + 10 + 14) / 5 = 24 / 5 = 4.8 m/s = 17.28 km/h
    assert(snap.speedValid);
    assert(std::abs(snap.avgSpeedMs - 4.8f) < 1e-4f);
    assert(std::abs(snap.avgSpeedKmh - 17.28f) < 1e-3f);

    // North queue check: 3 contiguous vehicles
    assert(snap.queues[0].vehicleCount == 3);
    assert(snap.queues[0].lengthMetres > 15.0f);

    // Other approach queues must be zero
    assert(snap.queues[1].vehicleCount == 0);
    assert(snap.queues[2].vehicleCount == 0);
    assert(snap.queues[3].vehicleCount == 0);

    std::cout << "  PASS: Scripted 5-vehicle state evaluated with exact mathematical precision." << std::endl;
}

void Test_ThroughputAndTripCompletions() {
    std::cout << "[Test 3] Throughput and Trip Completion Calculations..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    auto sigCtrl = std::make_shared<traffic::TrafficSignalController>();
    ped::PedestrianManager pedMgr(net, sigCtrl);
    sim::VehicleManager vehMgr(net, sigCtrl);

    analytics::AnalyticsEngine engine(vehMgr, *sigCtrl, pedMgr);

    // Disable background vehicle spawning to isolate trip completion mathematics
    auto cfg = vehMgr.GetConfig();
    cfg.demand.northArrivalsPerHour = 0.0f;
    cfg.demand.southArrivalsPerHour = 0.0f;
    cfg.demand.eastArrivalsPerHour = 0.0f;
    cfg.demand.westArrivalsPerHour = 0.0f;
    vehMgr.SetConfig(cfg);
    vehMgr.Reset(42);
    engine.Reset();

    // Advance simulation time to 60.0s
    for (int i = 0; i < 3600; ++i) {
        vehMgr.Step(1.0f / 60.0f);
        engine.Step(1.0f / 60.0f);
    }

    // Register 10 completed vehicle trips with varying waiting times
    for (int i = 1; i <= 10; ++i) {
        engine.RegisterTripCompletion(static_cast<float>(i * 2)); // 2, 4, 6, ..., 20 seconds wait
    }

    engine.Step(1.0f / 60.0f);

    const auto& snap = engine.GetSnapshot();
    assert(snap.tripWaitValid);
    // Average wait: (2 + 4 + ... + 20) / 10 = 110 / 10 = 11.0s
    assert(std::abs(snap.avgTripWaitingTime - 11.0f) < 1e-3f);

    // Rolling throughput: 10 exits in 60s -> (10 / 60) * 3600 = 600 veh/h
    assert(snap.rollingThroughputValid);
    assert(std::abs(snap.rollingThroughputPerHour - 600.0f) < 1e-3f);

    std::cout << "  PASS: Throughput (600 veh/h) and Average Trip Waiting Time (11.0s) verified." << std::endl;
}

void Test_ResetAndPauseInvariance() {
    std::cout << "[Test 4] Reset and Pause Invariance..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    auto sigCtrl = std::make_shared<traffic::TrafficSignalController>();
    ped::PedestrianManager pedMgr(net, sigCtrl);
    sim::VehicleManager vehMgr(net, sigCtrl);

    analytics::AnalyticsEngine engine(vehMgr, *sigCtrl, pedMgr);

    // Run active simulation
    for (int i = 0; i < 300; ++i) {
        vehMgr.Step(1.0f / 60.0f);
        engine.Step(1.0f / 60.0f);
    }

    // Pause simulation
    vehMgr.SetPaused(true);
    auto snapshotBeforePause = engine.GetSnapshot();

    for (int i = 0; i < 120; ++i) {
        vehMgr.Step(1.0f / 60.0f);
        engine.Step(1.0f / 60.0f);
    }

    auto snapshotDuringPause = engine.GetSnapshot();
    assert(snapshotBeforePause.simTime == snapshotDuringPause.simTime);
    assert(snapshotBeforePause.cumulativeFuelLiters == snapshotDuringPause.cumulativeFuelLiters);
    assert(snapshotBeforePause.cumulativeCO2Kg == snapshotDuringPause.cumulativeCO2Kg);

    // Unpause and Reset
    vehMgr.SetPaused(false);
    engine.Reset();

    auto snapshotAfterReset = engine.GetSnapshot();
    assert(snapshotAfterReset.activeVehicles == 0);
    assert(snapshotAfterReset.waitingVehicles == 0);
    assert(!snapshotAfterReset.speedValid);
    assert(!snapshotAfterReset.tripWaitValid);
    assert(snapshotAfterReset.cumulativeFuelLiters == 0.0f);

    std::cout << "  PASS: Simulation pause freezes metrics and Reset returns to initial state." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Real-Time Analytics Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    Test_EmptySystemMetrics();
    Test_ScriptedStateMetrics();
    Test_ThroughputAndTripCompletions();
    Test_ResetAndPauseInvariance();

    std::cout << "=====================================================" << std::endl;
    std::cout << "ALL ANALYTICS TESTS PASSED (4/4)" << std::endl;
    std::cout << "=====================================================" << std::endl;
    return 0;
}
