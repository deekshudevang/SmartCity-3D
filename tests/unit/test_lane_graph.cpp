#include "sim/LaneGraph.h"
#include "core/Logger.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace SmartCity::sim;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_lane_graph.log");
    Logger::Info("Starting test_lane_graph...");

    IntersectionSettings settings;
    settings.approachLength = 80.0f;
    settings.stopLineDistance = 13.0f;
    settings.roadWidth = 14.0f;

    LaneGraph laneGraph(settings);

    // 1. Verify all 12 trajectories exist
    const auto& trajectories = laneGraph.GetAllTrajectories();
    assert(trajectories.size() == 12);
    Logger::Info("Assertion passed: 12 trajectories generated.");

    // 2. Verify each trajectory properties
    for (int i = 0; i < 12; ++i) {
        const LaneTrajectory* traj = laneGraph.GetTrajectory(i);
        assert(traj != nullptr);
        assert(traj->GetId() == i);

        float totalLen = traj->GetTotalLength();
        // Approach is 80m, intersection is ~26m, outbound is 80m -> total >= 150m
        assert(totalLen > 140.0f);

        float stopDist = traj->GetStopLineDistance();
        // Stop line is 80m - 13m = 67m
        assert(std::abs(stopDist - 67.0f) < 1.0f);

        // 3. Sampling continuity and bounds
        PathPoint pStart = traj->SampleAtDistance(0.0f);
        PathPoint pStop = traj->SampleAtDistance(stopDist);
        PathPoint pEnd = traj->SampleAtDistance(totalLen);

        assert(std::abs(pStart.distanceAlongPath - 0.0f) < 0.001f);
        assert(std::abs(pStop.distanceAlongPath - stopDist) < 0.001f);
        assert(std::abs(pEnd.distanceAlongPath - totalLen) < 0.001f);

        // Check tangents are unit length
        assert(std::abs(glm::length(pStart.tangent) - 1.0f) < 0.05f);
        assert(std::abs(glm::length(pStop.tangent) - 1.0f) < 0.05f);
        assert(std::abs(glm::length(pEnd.tangent) - 1.0f) < 0.05f);
    }
    Logger::Info("Assertion passed: All trajectories have valid lengths and normalized tangents.");

    // 4. Verify ID lookup by approach and maneuver
    int nThru = laneGraph.GetTrajectoryId(ApproachDirection::North, TurnManeuver::Straight);
    int nRight = laneGraph.GetTrajectoryId(ApproachDirection::North, TurnManeuver::Right);
    int nLeft = laneGraph.GetTrajectoryId(ApproachDirection::North, TurnManeuver::Left);

    assert(nThru == 0);
    assert(nRight == 1);
    assert(nLeft == 2);
    Logger::Info("Assertion passed: Trajectory ID lookups match expected indices.");

    // 5. Verify Conflict Detection
    // North Left (Traj 2) must conflict with South Straight (Traj 3)
    const auto& nLeftConflicts = laneGraph.GetConflicts(2);
    bool foundOpposingStraight = false;
    for (const auto& conf : nLeftConflicts) {
        if (conf.conflictingTrajectoryId == 3) {
            foundOpposingStraight = true;
            // Conflict occurs after stop line
            assert(conf.myDistance >= 67.0f);
            assert(conf.conflictingDistance >= 67.0f);
            break;
        }
    }
    assert(foundOpposingStraight);
    Logger::Info("Assertion passed: Conflict detected between North Left and South Straight.");

    Logger::Info("test_lane_graph PASSED successfully.");
    Logger::Shutdown();
    return 0;
}

