#pragma once

#include "sim/Vehicle.h"

namespace SmartCity::sim {

class IDMController {
public:
    // Calculates acceleration given vehicle physics, current speed, leader speed, and net bumper-to-bumper gap
    static float CalculateAcceleration(const VehiclePhysics& physics, float currentSpeed, float leaderSpeed, float netGap);

    // Free road acceleration when no obstacle or leader is ahead
    static float CalculateFreeRoadAcceleration(const VehiclePhysics& physics, float currentSpeed);

    // Calculates acceleration to stop at a designated stop line
    static float CalculateStopLineAcceleration(const VehiclePhysics& physics, float currentSpeed, float distanceToStopLine);

    // Checks if vehicle is in dilemma zone (too close to stop safely at yellow light)
    static bool IsInDilemmaZone(const VehiclePhysics& physics, float currentSpeed, float distanceToStopLine);
};

} // namespace SmartCity::sim

