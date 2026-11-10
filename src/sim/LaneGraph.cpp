#include "sim/LaneGraph.h"
#include <cmath>
#include <algorithm>

namespace SmartCity::sim {

LaneTrajectory::LaneTrajectory(int id, ApproachDirection entryApproach, TurnManeuver maneuver, int entryLaneIndex)
    : m_id(id)
    , m_entryApproach(entryApproach)
    , m_maneuver(maneuver)
    , m_entryLaneIndex(entryLaneIndex)
    , m_totalLength(0.0f)
    , m_stopLineDistance(67.0f) // default: 80m approach - 13m stopline = 67m
{
}

void LaneTrajectory::AddPoint(const glm::vec3& point) {
    PathPoint pt;
    pt.position = point;
    pt.tangent = glm::vec3(0.0f, 0.0f, 1.0f);
    pt.headingRad = 0.0f;
    pt.distanceAlongPath = 0.0f;

    if (!m_points.empty()) {
        float segDist = glm::distance(point, m_points.back().position);
        pt.distanceAlongPath = m_points.back().distanceAlongPath + segDist;
    }
    m_points.push_back(pt);
}

void LaneTrajectory::Finalize() {
    if (m_points.size() < 2) return;

    m_totalLength = m_points.back().distanceAlongPath;

    for (size_t i = 0; i < m_points.size(); ++i) {
        glm::vec3 tangent(0.0f);
        if (i + 1 < m_points.size()) {
            tangent = glm::normalize(m_points[i + 1].position - m_points[i].position);
        } else if (i > 0) {
            tangent = m_points[i - 1].tangent;
        }

        m_points[i].tangent = tangent;
        // Yaw heading angle in radians in XZ plane (0 = +Z, PI/2 = +X)
        m_points[i].headingRad = std::atan2(tangent.x, tangent.z);
    }
}

PathPoint LaneTrajectory::SampleAtDistance(float distance) const {
    if (m_points.empty()) {
        return { glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.0f };
    }

    if (distance <= 0.0f) {
        return m_points.front();
    }
    if (distance >= m_totalLength) {
        return m_points.back();
    }

    // Binary search for segment enclosing 'distance'
    size_t low = 0;
    size_t high = m_points.size() - 1;

    while (low + 1 < high) {
        size_t mid = (low + high) / 2;
        if (m_points[mid].distanceAlongPath <= distance) {
            low = mid;
        } else {
            high = mid;
        }
    }

    float d0 = m_points[low].distanceAlongPath;
    float d1 = m_points[high].distanceAlongPath;
    float t = (d1 > d0) ? ((distance - d0) / (d1 - d0)) : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    PathPoint result;
    result.position = glm::mix(m_points[low].position, m_points[high].position, t);
    result.tangent = glm::normalize(glm::mix(m_points[low].tangent, m_points[high].tangent, t));
    result.headingRad = std::atan2(result.tangent.x, result.tangent.z);
    result.distanceAlongPath = distance;

    return result;
}

// ----------------- LaneGraph -----------------
LaneGraph::LaneGraph(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    BuildNetwork();
    ComputeConflicts();
}

const LaneTrajectory* LaneGraph::GetTrajectory(int id) const {
    if (id >= 0 && id < static_cast<int>(m_trajectories.size())) {
        return m_trajectories[id].get();
    }
    return nullptr;
}

int LaneGraph::GetTrajectoryId(ApproachDirection entry, TurnManeuver turn) const {
    for (const auto& traj : m_trajectories) {
        if (traj->GetEntryApproach() == entry && traj->GetManeuver() == turn) {
            return traj->GetId();
        }
    }
    return -1;
}

float LaneGraph::GetStopLineDistanceOnPath(int trajectoryId) const {
    const LaneTrajectory* traj = GetTrajectory(trajectoryId);
    if (traj) return traj->GetStopLineDistance();
    return 67.0f;
}

const std::vector<ConflictPoint>& LaneGraph::GetConflicts(int trajectoryId) const {
    static const std::vector<ConflictPoint> emptyConflicts;
    if (trajectoryId >= 0 && trajectoryId < static_cast<int>(m_conflicts.size())) {
        return m_conflicts[trajectoryId];
    }
    return emptyConflicts;
}

void LaneGraph::BuildNetwork() {
    m_trajectories.clear();

    float len = m_settings.approachLength;   // 80.0m
    float stop = m_settings.stopLineDistance; // 13.0m
    float hw = m_settings.roadWidth * 0.5f;   // 7.0m

    float innerOffset = hw * 0.25f; // 1.75m from center
    float outerOffset = hw * 0.75f; // 5.25m from center

    auto evaluateCubicBezier = [](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
        float u = 1.0f - t;
        return (u * u * u) * p0 + (3.0f * u * u * t) * p1 + (3.0f * u * t * t) * p2 + (t * t * t) * p3;
    };

    int currentId = 0;

    // Helper to generate a trajectory with straight inbound, Bézier/straight intersection, and straight outbound
    auto createTrajectory = [&](ApproachDirection entry, TurnManeuver maneuver, int laneIdx,
                                const glm::vec3& start, const glm::vec3& stopPoint,
                                const glm::vec3& turnP1, const glm::vec3& turnP2, const glm::vec3& turnExit,
                                const glm::vec3& end) {
        auto traj = std::make_unique<LaneTrajectory>(currentId++, entry, maneuver, laneIdx);

        // 1. Inbound segment (steps of 1.0m)
        int inboundSteps = static_cast<int>(std::ceil(glm::distance(start, stopPoint)));
        for (int i = 0; i <= inboundSteps; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(inboundSteps);
            traj->AddPoint(glm::mix(start, stopPoint, t));
        }

        // Set stop line distance
        float stopDist = glm::distance(start, stopPoint);
        traj->SetStopLineDistance(stopDist);

        // 2. Intersection turn / through segment
        if (maneuver == TurnManeuver::Straight) {
            int thruSteps = static_cast<int>(std::ceil(glm::distance(stopPoint, turnExit)));
            for (int i = 1; i <= thruSteps; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(thruSteps);
                traj->AddPoint(glm::mix(stopPoint, turnExit, t));
            }
        } else {
            // Cubic Bézier curve (24 sub-steps for smooth turning curvature)
            for (int i = 1; i <= 24; ++i) {
                float t = static_cast<float>(i) / 24.0f;
                traj->AddPoint(evaluateCubicBezier(stopPoint, turnP1, turnP2, turnExit, t));
            }
        }

        // 3. Outbound segment
        int outboundSteps = static_cast<int>(std::ceil(glm::distance(turnExit, end)));
        for (int i = 1; i <= outboundSteps; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(outboundSteps);
            traj->AddPoint(glm::mix(turnExit, end, t));
        }

        traj->Finalize();
        m_trajectories.push_back(std::move(traj));
    };

    // ================= 1. NORTH APPROACH (Entry: North, heading +Z, Right-Hand Traffic: X < 0) =================
    // Traj 0: North Straight (Lane 1, outer) -> South outbound
    createTrajectory(ApproachDirection::North, TurnManeuver::Straight, 1,
        { -innerOffset, 0.0f, -len }, { -innerOffset, 0.0f, -stop },
        { -innerOffset, 0.0f, 0.0f }, { -innerOffset, 0.0f, 0.0f }, { -innerOffset, 0.0f, stop },
        { -innerOffset, 0.0f, len });

    // Traj 1: North Right Turn (Lane 1, outer) -> West outbound (-X, right side is +Z)
    createTrajectory(ApproachDirection::North, TurnManeuver::Right, 1,
        { -outerOffset, 0.0f, -len }, { -outerOffset, 0.0f, -stop },
        { -outerOffset, 0.0f, -stop * 0.4f }, { -stop * 0.4f, 0.0f, outerOffset }, { -stop, 0.0f, outerOffset },
        { -len, 0.0f, outerOffset });

    // Traj 2: North Left Turn (Lane 0, inner) -> East outbound (+X, right side is -Z)
    createTrajectory(ApproachDirection::North, TurnManeuver::Left, 0,
        { -innerOffset, 0.0f, -len }, { -innerOffset, 0.0f, -stop },
        { -innerOffset, 0.0f, 2.0f }, { 2.0f, 0.0f, -innerOffset }, { stop, 0.0f, -innerOffset },
        { len, 0.0f, -innerOffset });

    // ================= 2. SOUTH APPROACH (Entry: South, heading -Z, Right-Hand Traffic: X > 0) =================
    // Traj 3: South Straight (Lane 1, outer) -> North outbound
    createTrajectory(ApproachDirection::South, TurnManeuver::Straight, 1,
        { innerOffset, 0.0f, len }, { innerOffset, 0.0f, stop },
        { innerOffset, 0.0f, 0.0f }, { innerOffset, 0.0f, 0.0f }, { innerOffset, 0.0f, -stop },
        { innerOffset, 0.0f, -len });

    // Traj 4: South Right Turn (Lane 1, outer) -> East outbound (+X, right side is -Z)
    createTrajectory(ApproachDirection::South, TurnManeuver::Right, 1,
        { outerOffset, 0.0f, len }, { outerOffset, 0.0f, stop },
        { outerOffset, 0.0f, stop * 0.4f }, { stop * 0.4f, 0.0f, -outerOffset }, { stop, 0.0f, -outerOffset },
        { len, 0.0f, -outerOffset });

    // Traj 5: South Left Turn (Lane 0, inner) -> West outbound (-X, right side is +Z)
    createTrajectory(ApproachDirection::South, TurnManeuver::Left, 0,
        { innerOffset, 0.0f, len }, { innerOffset, 0.0f, stop },
        { innerOffset, 0.0f, -2.0f }, { -2.0f, 0.0f, innerOffset }, { -stop, 0.0f, innerOffset },
        { -len, 0.0f, innerOffset });

    // ================= 3. EAST APPROACH (Entry: East, heading -X, Right-Hand Traffic: Z > 0) =================
    // Traj 6: East Straight (Lane 1, outer) -> West outbound
    createTrajectory(ApproachDirection::East, TurnManeuver::Straight, 1,
        { len, 0.0f, innerOffset }, { stop, 0.0f, innerOffset },
        { 0.0f, 0.0f, innerOffset }, { 0.0f, 0.0f, innerOffset }, { -stop, 0.0f, innerOffset },
        { -len, 0.0f, innerOffset });

    // Traj 7: East Right Turn (Lane 1, outer) -> North outbound (-Z, right side is +X)
    createTrajectory(ApproachDirection::East, TurnManeuver::Right, 1,
        { len, 0.0f, outerOffset }, { stop, 0.0f, outerOffset },
        { stop * 0.4f, 0.0f, outerOffset }, { outerOffset, 0.0f, stop * 0.4f }, { outerOffset, 0.0f, -stop },
        { outerOffset, 0.0f, -len });

    // Traj 8: East Left Turn (Lane 0, inner) -> South outbound (+Z, right side is -X)
    createTrajectory(ApproachDirection::East, TurnManeuver::Left, 0,
        { len, 0.0f, innerOffset }, { stop, 0.0f, innerOffset },
        { -2.0f, 0.0f, innerOffset }, { -innerOffset, 0.0f, 2.0f }, { -innerOffset, 0.0f, stop },
        { -innerOffset, 0.0f, len });

    // ================= 4. WEST APPROACH (Entry: West, heading +X, Right-Hand Traffic: Z < 0) =================
    // Traj 9: West Straight (Lane 1, outer) -> East outbound
    createTrajectory(ApproachDirection::West, TurnManeuver::Straight, 1,
        { -len, 0.0f, -innerOffset }, { -stop, 0.0f, -innerOffset },
        { 0.0f, 0.0f, -innerOffset }, { 0.0f, 0.0f, -innerOffset }, { stop, 0.0f, -innerOffset },
        { len, 0.0f, -innerOffset });

    // Traj 10: West Right Turn (Lane 1, outer) -> South outbound (+Z, right side is -X)
    createTrajectory(ApproachDirection::West, TurnManeuver::Right, 1,
        { -len, 0.0f, -outerOffset }, { -stop, 0.0f, -outerOffset },
        { -stop * 0.4f, 0.0f, -outerOffset }, { -outerOffset, 0.0f, -stop * 0.4f }, { -outerOffset, 0.0f, stop },
        { -outerOffset, 0.0f, len });

    // Traj 11: West Left Turn (Lane 0, inner) -> North outbound (-Z, right side is +X)
    createTrajectory(ApproachDirection::West, TurnManeuver::Left, 0,
        { -len, 0.0f, -innerOffset }, { -stop, 0.0f, -innerOffset },
        { 2.0f, 0.0f, -innerOffset }, { innerOffset, 0.0f, -2.0f }, { innerOffset, 0.0f, -stop },
        { innerOffset, 0.0f, -len });
}

void LaneGraph::ComputeConflicts() {
    m_conflicts.clear();
    m_conflicts.resize(m_trajectories.size());

    // Intersection crossing threshold distance (m)
    const float conflictThreshold = 3.2f;

    for (size_t i = 0; i < m_trajectories.size(); ++i) {
        const auto& trajA = m_trajectories[i];
        float stopA = trajA->GetStopLineDistance();
        float endA = stopA + (m_settings.stopLineDistance * 2.0f) + 4.0f;

        for (size_t j = i + 1; j < m_trajectories.size(); ++j) {
            const auto& trajB = m_trajectories[j];
            // Skip identical entry approach (parallel/branching lanes)
            if (trajA->GetEntryApproach() == trajB->GetEntryApproach()) continue;

            float stopB = trajB->GetStopLineDistance();
            float endB = stopB + (m_settings.stopLineDistance * 2.0f) + 4.0f;

            // Search for closest crossing point inside intersection
            float minDistance = 1e9f;
            float bestDistA = 0.0f;
            float bestDistB = 0.0f;

            for (float dA = stopA; dA <= endA; dA += 0.5f) {
                glm::vec3 posA = trajA->SampleAtDistance(dA).position;
                for (float dB = stopB; dB <= endB; dB += 0.5f) {
                    glm::vec3 posB = trajB->SampleAtDistance(dB).position;
                    float dist = glm::distance(posA, posB);
                    if (dist < minDistance) {
                        minDistance = dist;
                        bestDistA = dA;
                        bestDistB = dB;
                    }
                }
            }

            if (minDistance < conflictThreshold) {
                m_conflicts[i].push_back({ static_cast<int>(j), bestDistA, bestDistB });
                m_conflicts[j].push_back({ static_cast<int>(i), bestDistB, bestDistA });
            }
        }
    }
}

} // namespace SmartCity::sim
