#include "sim/IDMController.h"
#include "core/Logger.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace SmartCity::sim;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_idm.log");
    Logger::Info("Starting test_idm...");

    VehiclePhysics physics = Vehicle::GetDefaultPhysics(VehicleType::Sedan);
    physics.desiredSpeed = 14.0f;     // v0 = 14 m/s
    physics.maxAcceleration = 2.0f;  // a = 2.0 m/s^2
    physics.comfortableDecel = 2.0f; // b = 2.0 m/s^2
    physics.minGap = 2.0f;           // s0 = 2.0 m
    physics.timeHeadway = 1.5f;      // T = 1.5 s
    physics.maxBraking = 8.0f;

    // 1. Free road acceleration
    float aZero = IDMController::CalculateFreeRoadAcceleration(physics, 0.0f);
    assert(std::abs(aZero - physics.maxAcceleration) < 0.01f);

    float aHalf = IDMController::CalculateFreeRoadAcceleration(physics, 7.0f);
    assert(aHalf > 0.0f && aHalf < physics.maxAcceleration);

    float aMax = IDMController::CalculateFreeRoadAcceleration(physics, 14.0f);
    assert(std::abs(aMax) < 0.01f);

    float aOver = IDMController::CalculateFreeRoadAcceleration(physics, 16.0f);
    assert(aOver < 0.0f);
    Logger::Info("Assertion passed: Free road acceleration matches analytical IDM curve.");

    // 2. Car following behind slower leader
    // Driving at 14 m/s behind leader moving at 5 m/s at 15m gap
    float aBraking = IDMController::CalculateAcceleration(physics, 14.0f, 5.0f, 15.0f);
    assert(aBraking < 0.0f);
    Logger::Info("Assertion passed: Deceleration triggered when approaching slower leader (a = ", aBraking, ").");

    // 3. Equilibrium following
    // At equilibrium: v = v_lead = 10 m/s, gap = s0 + v * T = 2.0 + 10 * 1.5 = 17m
    float sEquil = physics.minGap + 10.0f * physics.timeHeadway;
    float aEquil = IDMController::CalculateAcceleration(physics, 10.0f, 10.0f, sEquil);
    // Should be slight or near zero (free term is 1 - (10/14)^4 ~ 0.74, interaction term is (17/17)^2 = 1, so a = 2.0 * (0.74 - 1) = -0.5)
    assert(aEquil >= -1.0f && aEquil <= 0.5f);
    Logger::Info("Assertion passed: Equilibrium following acceleration is stable (a = ", aEquil, ").");

    // 4. Standstill behind stopped vehicle
    // v = 0, v_lead = 0, gap = s0 = 2.0m -> a should be exactly 0
    float aStandstill = IDMController::CalculateAcceleration(physics, 0.0f, 0.0f, physics.minGap);
    assert(std::abs(aStandstill) < 0.01f);

    // If gap is smaller than s0 (e.g. 1.0m), vehicle must brake hard
    float aTooClose = IDMController::CalculateAcceleration(physics, 0.0f, 0.0f, 1.0f);
    assert(aTooClose < 0.0f);
    Logger::Info("Assertion passed: Standstill equilibrium holds at s0 gap.");

    // 5. Maximum braking clamp
    float aExtreme = IDMController::CalculateAcceleration(physics, 20.0f, 0.0f, 1.0f);
    assert(aExtreme >= -physics.maxBraking);
    Logger::Info("Assertion passed: Deceleration never exceeds maxBraking (-", physics.maxBraking, " m/s^2).");

    // 6. Stop Line Acceleration and Dilemma Zone
    // Stopped line 30m ahead at 10 m/s
    float aStop = IDMController::CalculateStopLineAcceleration(physics, 10.0f, 30.0f);
    assert(aStop < physics.maxAcceleration);

    // Dilemma zone: at 10 m/s, stopping dist = 100 / (2 * 2) = 25m.
    // If distance is 5m, in dilemma zone (cannot stop comfortably)
    assert(IDMController::IsInDilemmaZone(physics, 10.0f, 5.0f));
    // If distance is 40m, not in dilemma zone
    assert(!IDMController::IsInDilemmaZone(physics, 10.0f, 40.0f));
    Logger::Info("Assertion passed: Dilemma zone detection and stop line deceleration verified.");

    Logger::Info("test_idm PASSED successfully.");
    Logger::Shutdown();
    return 0;
}

