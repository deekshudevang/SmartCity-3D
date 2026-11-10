#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "core/Config.h"

namespace SmartCity::sim {

enum class ApproachDirection {
    North, // -Z
    South, // +Z
    East,  // +X
    West   // -X
};

enum class TurnManeuver {
    Left,
    Straight,
    Right
};

struct PathPoint {
    glm::vec3 position;
    glm::vec3 tangent;
    float headingRad;
    float distanceAlongPath;
};

struct ConflictPoint {
    int conflictingTrajectoryId;
    float myDistance;
    float conflictingDistance;
};

class LaneTrajectory {
public:
    LaneTrajectory(int id, ApproachDirection entryApproach, TurnManeuver maneuver, int entryLaneIndex);

    void AddPoint(const glm::vec3& point);
    void Finalize();

    float GetTotalLength() const { return m_totalLength; }
    PathPoint SampleAtDistance(float distance) const;

    int GetId() const { return m_id; }
    ApproachDirection GetEntryApproach() const { return m_entryApproach; }
    TurnManeuver GetManeuver() const { return m_maneuver; }
    int GetEntryLaneIndex() const { return m_entryLaneIndex; }
    float GetStopLineDistance() const { return m_stopLineDistance; }
    void SetStopLineDistance(float d) { m_stopLineDistance = d; }

    const std::vector<PathPoint>& GetPoints() const { return m_points; }

private:
    int m_id;
    ApproachDirection m_entryApproach;
    TurnManeuver m_maneuver;
    int m_entryLaneIndex;
    float m_totalLength;
    float m_stopLineDistance;

    std::vector<PathPoint> m_points;
};

class LaneGraph {
public:
    LaneGraph(const core::IntersectionSettings& settings);

    const LaneTrajectory* GetTrajectory(int id) const;
    const std::vector<std::unique_ptr<LaneTrajectory>>& GetAllTrajectories() const { return m_trajectories; }

    int GetTrajectoryId(ApproachDirection entry, TurnManeuver turn) const;

    float GetStopLineDistanceOnPath(int trajectoryId) const;

    const std::vector<ConflictPoint>& GetConflicts(int trajectoryId) const;

private:
    void BuildNetwork();
    void ComputeConflicts();

    core::IntersectionSettings m_settings;
    std::vector<std::unique_ptr<LaneTrajectory>> m_trajectories;
    std::vector<std::vector<ConflictPoint>> m_conflicts; // index = trajectoryId
};

} // namespace SmartCity::sim
