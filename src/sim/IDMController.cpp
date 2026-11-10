#include "sim/IDMController.h"
#include <algorithm>
#include <cmath>

namespace SmartCity::sim {

float IDMController::CalculateFreeRoadAcceleration(const VehiclePhysics& physics, float currentSpeed) {
    float v0 = std::max(physics.desiredSpeed, 0.1f);
    float vRatio = currentSpeed / v0;
    float freeTerm = 1.0f - std::pow(vRatio, 4.0f);
    return std::clamp(physics.maxAcceleration * freeTerm, -physics.maxBraking, physics.maxAcceleration);
}

float IDMController::CalculateAcceleration(const VehiclePhysics& physics, float currentSpeed, float leaderSpeed, float netGap) {
    float a = physics.maxAcceleration;
    float b = physics.comfortableDecel;
    float v0 = std::max(physics.desiredSpeed, 0.1f);
    float s0 = physics.minGap;
    float T = physics.timeHeadway;

    // Delta velocity (closing speed to leader)
    float deltaV = currentSpeed - leaderSpeed;

    // Dynamic desired distance s*(v, deltaV)
    float sqrtAB = 2.0f * std::sqrt(a * b);
    float dynamicTerm = (currentSpeed * deltaV) / std::max(sqrtAB, 0.001f);
    float desiredGap = s0 + currentSpeed * T + dynamicTerm;
    desiredGap = std::max(desiredGap, s0);

    float effectiveGap = std::max(netGap, 0.1f);
    float interactionRatio = desiredGap / effectiveGap;

    float vRatio = currentSpeed / v0;
    float freeTerm = 1.0f - std::pow(vRatio, 4.0f);
    float brakingTerm = std::pow(interactionRatio, 2.0f);

    float accel = a * (freeTerm - brakingTerm);
    return std::clamp(accel, -physics.maxBraking, a);
}

float IDMController::CalculateStopLineAcceleration(const VehiclePhysics& physics, float currentSpeed, float distanceToStopLine) {
    if (distanceToStopLine <= 0.0f) {
        // Already past the stop line
        return CalculateFreeRoadAcceleration(physics, currentSpeed);
    }
    // Stop line acts as stationary obstacle (leaderSpeed = 0, netGap = distanceToStopLine)
    return CalculateAcceleration(physics, currentSpeed, 0.0f, distanceToStopLine);
}

bool IDMController::IsInDilemmaZone(const VehiclePhysics& physics, float currentSpeed, float distanceToStopLine) {
    // Distance needed to stop comfortably: d_stop = v^2 / (2 * b)
    float minStoppingDistance = (currentSpeed * currentSpeed) / (2.0f * physics.comfortableDecel);
    // If distance to line is less than comfortable stopping distance plus a margin, cannot stop safely
    return distanceToStopLine < (minStoppingDistance + 1.0f);
}

} // namespace SmartCity::sim

