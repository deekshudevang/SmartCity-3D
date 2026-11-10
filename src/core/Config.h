#pragma once

#include <string>
#include <cstdint>

namespace SmartCity::core {

struct SimulationSettings {
    uint32_t randomSeed = 1337;
    double fixedTimestep = 0.0166666667;
    double maxDeltaTime = 0.25;

    // Traffic dynamics (in m, s, m/s, m/s^2)
    float speedLimit = 13.88f;         // 50 km/h in m/s
    float turningSpeedLimit = 5.55f;   // 20 km/h in m/s
    float idmDesiredVelocity = 13.88f;
    float idmSafeTimeHeadway = 1.2f;
    float idmMinGapDistance = 2.0f;
    float idmMaxAcceleration = 2.0f;
    float idmComfortableDecel = 2.5f;
    float idmDeltaExponent = 4.0f;

    // Signal timing (in seconds)
    float phaseGreenTime = 24.0f;
    float phaseYellowClearance = 3.5f;
    float phaseAllRedClearance = 1.5f;
    float leftTurnGreenTime = 10.0f;
    float pedestrianWalkTime = 15.0f;
    float minGreen = 7.0f;
    float maxGreen = 45.0f;

    // Demand (vehicles per minute)
    float northSpawnRate = 18.0f;
    float southSpawnRate = 18.0f;
    float eastSpawnRate = 14.0f;
    float westSpawnRate = 14.0f;
};

struct IntersectionSettings {
    bool rightHandTraffic = true;
    float laneWidth = 3.5f;
    int numThroughLanes = 2;
    float turnPocketLength = 30.0f;
    float medianWidth = 1.5f;
    float curbHeight = 0.15f;
    float sidewalkWidth = 3.0f;
    float crosswalkWidth = 4.0f;
    float stopLineBeforeCrosswalk = 1.5f;
    float approachLength = 120.0f;
    float cornerRadius = 8.0f;
    float crosswalkDistance = 14.0f;
    float stopLineDistance = 19.5f;

    // Backward-compatible fields
    float roadWidth = 14.0f;
    int numLanesPerApproach = 4;
};

class ConfigManager {
public:
    static bool LoadSimulationConfig(const std::string& path, SimulationSettings& outSettings);
    static bool LoadIntersectionConfig(const std::string& path, IntersectionSettings& outSettings);
};

} // namespace SmartCity::core
