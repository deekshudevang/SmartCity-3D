#include "city/RoadNetwork.h"
#include "core/Logger.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <filesystem>

namespace SmartCity::city {

std::string ToString(ApproachId approach) {
    switch (approach) {
        case ApproachId::North: return "North";
        case ApproachId::East:  return "East";
        case ApproachId::South: return "South";
        case ApproachId::West:  return "West";
    }
    return "Unknown";
}

ApproachId ApproachFromString(const std::string& str) {
    if (str == "North" || str == "N") return ApproachId::North;
    if (str == "East"  || str == "E") return ApproachId::East;
    if (str == "South" || str == "S") return ApproachId::South;
    if (str == "West"  || str == "W") return ApproachId::West;
    return ApproachId::North;
}

std::string ToString(MovementType movement) {
    switch (movement) {
        case MovementType::Left:    return "Left";
        case MovementType::Through: return "Through";
        case MovementType::Right:   return "Right";
    }
    return "Unknown";
}

MovementType MovementFromString(const std::string& str) {
    if (str == "Left" || str == "L") return MovementType::Left;
    if (str == "Right" || str == "R") return MovementType::Right;
    return MovementType::Through;
}

std::string ToString(LaneDirection dir) {
    switch (dir) {
        case LaneDirection::Inbound:  return "Inbound";
        case LaneDirection::Outbound: return "Outbound";
    }
    return "Unknown";
}

LaneDirection DirectionFromString(const std::string& str) {
    if (str == "Outbound" || str == "OUT") return LaneDirection::Outbound;
    return LaneDirection::Inbound;
}

// ---------------- TurningPath Implementation ----------------

glm::vec3 TurningPath::Evaluate(float t) const {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
}

glm::vec3 TurningPath::EvaluateTangent(float t) const {
    float u = 1.0f - t;
    glm::vec3 d = 3.0f * u * u * (p1 - p0) + 6.0f * u * t * (p2 - p1) + 3.0f * t * t * (p3 - p2);
    float len = glm::length(d);
    return len > 1e-5f ? d / len : glm::vec3(0.0f, 0.0f, 1.0f);
}

float TurningPath::CalculateCurvature(float t) const {
    float u = 1.0f - t;
    // First derivative
    glm::vec3 d1 = 3.0f * u * u * (p1 - p0) + 6.0f * u * t * (p2 - p1) + 3.0f * t * t * (p3 - p2);
    // Second derivative
    glm::vec3 d2 = 6.0f * u * (p2 - 2.0f * p1 + p0) + 6.0f * t * (p3 - 2.0f * p2 + p1);

    // Curvature in XZ plane: |x' z'' - z' x''| / (x'^2 + z'^2)^(3/2)
    float num = std::abs(d1.x * d2.z - d1.z * d2.x);
    float denom = std::pow(d1.x * d1.x + d1.z * d1.z, 1.5f);
    if (denom < 1e-6f) return 0.0f;
    return num / denom;
}

float TurningPath::CalculateMinRadius() const {
    if (movement == MovementType::Through) {
        return 1e6f; // Through movement is straight line
    }

    float maxCurvature = 0.0f;
    constexpr int samples = 50;
    for (int i = 0; i <= samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(samples);
        float k = CalculateCurvature(t);
        if (k > maxCurvature) {
            maxCurvature = k;
        }
    }

    if (maxCurvature < 1e-5f) return 1e6f;
    return 1.0f / maxCurvature;
}

// ---------------- RoadNetwork Implementation ----------------

RoadNetwork::RoadNetwork() {
    core::IntersectionSettings defaultSettings;
    BuildFromSpec(defaultSettings);
}

void RoadNetwork::BuildFromSpec(const core::IntersectionSettings& settings) {
    m_settings = settings;

    m_approaches.clear();
    m_lanes.clear();
    m_paths.clear();
    m_stopLines.clear();
    m_crosswalks.clear();
    m_conflictZones.clear();
    m_entryPoints.clear();
    m_exitPoints.clear();

    m_laneIndexMap.clear();
    m_pathIndexMap.clear();
    m_stopLineIndexMap.clear();
    m_crosswalkIndexMap.clear();
    m_approachIndexMap.clear();

    m_intersection.id = "MainIntersection";
    m_intersection.center = glm::vec3(0.0f);
    m_intersection.coreRadius = m_settings.stopLineDistance;
    m_intersection.approaches = { ApproachId::North, ApproachId::East, ApproachId::South, ApproachId::West };

    GenerateApproaches();
    GenerateLanes();
    GenerateCrosswalksAndStopLines();
    GenerateTurningPaths();
    GenerateConflictZones();
    GenerateEntryExitPoints();

    core::Logger::Info("RoadNetwork built from spec: 4 approaches, ", m_lanes.size(), " lanes, ",
                       m_paths.size(), " paths, ", m_conflictZones.size(), " conflict zones.");
}

void RoadNetwork::GenerateApproaches() {
    Approach north;
    north.id = ApproachId::North;
    north.name = "NorthApproach";
    north.forwardDir = glm::vec3(0.0f, 0.0f, 1.0f); // moving +Z toward center
    north.headingRad = glm::radians(90.0f);
    north.stopLineId = "STOP_North";
    north.crosswalkId = "CW_North";

    Approach east;
    east.id = ApproachId::East;
    east.name = "EastApproach";
    east.forwardDir = glm::vec3(-1.0f, 0.0f, 0.0f); // moving -X toward center
    east.headingRad = glm::radians(180.0f);
    east.stopLineId = "STOP_East";
    east.crosswalkId = "CW_East";

    Approach south;
    south.id = ApproachId::South;
    south.name = "SouthApproach";
    south.forwardDir = glm::vec3(0.0f, 0.0f, -1.0f); // moving -Z toward center
    south.headingRad = glm::radians(270.0f);
    south.stopLineId = "STOP_South";
    south.crosswalkId = "CW_South";

    Approach west;
    west.id = ApproachId::West;
    west.name = "WestApproach";
    west.forwardDir = glm::vec3(1.0f, 0.0f, 0.0f); // moving +X toward center
    west.headingRad = glm::radians(0.0f);
    west.stopLineId = "STOP_West";
    west.crosswalkId = "CW_West";

    m_approaches = { north, east, south, west };
    for (size_t i = 0; i < m_approaches.size(); ++i) {
        m_approachIndexMap[m_approaches[i].id] = i;
    }
}

void RoadNetwork::GenerateLanes() {
    float laneW = m_settings.laneWidth;
    float medW = m_settings.medianWidth;
    float appLen = m_settings.approachLength;
    float stopDist = m_settings.stopLineDistance;

    // Inbound lane center lateral offsets from median edge outward (+):
    // Lane 0 (Left pocket): medW/2 + 0.5 * laneW
    // Lane 1 (Inner through): medW/2 + 1.5 * laneW
    // Lane 2 (Outer through): medW/2 + 2.5 * laneW
    // Lane 3 (Right pocket): medW/2 + 3.5 * laneW
    float inOffsets[4] = {
        medW * 0.5f + 0.5f * laneW,
        medW * 0.5f + 1.5f * laneW,
        medW * 0.5f + 2.5f * laneW,
        medW * 0.5f + 3.5f * laneW
    };

    // Outbound lane center lateral offsets from median edge outward (-):
    float outOffsets[2] = {
        -(medW * 0.5f + 0.5f * laneW),
        -(medW * 0.5f + 1.5f * laneW)
    };

    auto buildLane = [this, appLen, stopDist, laneW](ApproachId app, LaneDirection dir, int idx, float latOffset) {
        Lane lane;
        std::string appCode;
        switch (app) {
            case ApproachId::North: appCode = "N"; break;
            case ApproachId::East:  appCode = "E"; break;
            case ApproachId::South: appCode = "S"; break;
            case ApproachId::West:  appCode = "W"; break;
        }

        std::string dirCode = (dir == LaneDirection::Inbound) ? "IN" : "OUT";
        lane.id = appCode + "_" + dirCode + "_" + std::to_string(idx);
        lane.approach = app;
        lane.direction = dir;
        lane.laneIndex = idx;
        lane.width = laneW;

        if (dir == LaneDirection::Inbound) {
            if (idx == 0) lane.allowedMovements = { MovementType::Left };
            else if (idx == 1 || idx == 2) lane.allowedMovements = { MovementType::Through };
            else lane.allowedMovements = { MovementType::Right };
        } else {
            lane.allowedMovements = { MovementType::Through };
        }

        glm::vec3 startPt(0.0f);
        glm::vec3 endPt(0.0f);
        float heading = 0.0f;

        if (app == ApproachId::North) {
            heading = (dir == LaneDirection::Inbound) ? glm::radians(90.0f) : glm::radians(270.0f);
            if (dir == LaneDirection::Inbound) {
                float startZ = (idx == 0 || idx == 3) ? -(stopDist + m_settings.turnPocketLength) : -appLen;
                startPt = glm::vec3(latOffset, 0.0f, startZ);
                endPt = glm::vec3(latOffset, 0.0f, -stopDist);
            } else {
                startPt = glm::vec3(latOffset, 0.0f, -stopDist);
                endPt = glm::vec3(latOffset, 0.0f, -appLen);
            }
        } else if (app == ApproachId::South) {
            heading = (dir == LaneDirection::Inbound) ? glm::radians(270.0f) : glm::radians(90.0f);
            if (dir == LaneDirection::Inbound) {
                float startZ = (idx == 0 || idx == 3) ? (stopDist + m_settings.turnPocketLength) : appLen;
                startPt = glm::vec3(-latOffset, 0.0f, startZ);
                endPt = glm::vec3(-latOffset, 0.0f, stopDist);
            } else {
                startPt = glm::vec3(-latOffset, 0.0f, stopDist);
                endPt = glm::vec3(-latOffset, 0.0f, appLen);
            }
        } else if (app == ApproachId::East) {
            heading = (dir == LaneDirection::Inbound) ? glm::radians(180.0f) : glm::radians(0.0f);
            if (dir == LaneDirection::Inbound) {
                float startX = (idx == 0 || idx == 3) ? (stopDist + m_settings.turnPocketLength) : appLen;
                startPt = glm::vec3(startX, 0.0f, latOffset);
                endPt = glm::vec3(stopDist, 0.0f, latOffset);
            } else {
                startPt = glm::vec3(stopDist, 0.0f, latOffset);
                endPt = glm::vec3(appLen, 0.0f, latOffset);
            }
        } else if (app == ApproachId::West) {
            heading = (dir == LaneDirection::Inbound) ? glm::radians(0.0f) : glm::radians(180.0f);
            if (dir == LaneDirection::Inbound) {
                float startX = (idx == 0 || idx == 3) ? -(stopDist + m_settings.turnPocketLength) : -appLen;
                startPt = glm::vec3(startX, 0.0f, -latOffset);
                endPt = glm::vec3(-stopDist, 0.0f, -latOffset);
            } else {
                startPt = glm::vec3(-stopDist, 0.0f, -latOffset);
                endPt = glm::vec3(-appLen, 0.0f, -latOffset);
            }
        }

        lane.startPoint = startPt;
        lane.endPoint = endPt;
        lane.headingRad = heading;
        lane.length = glm::distance(startPt, endPt);

        lane.centerline = { startPt, (startPt + endPt) * 0.5f, endPt };
        return lane;
    };

    ApproachId apps[4] = { ApproachId::North, ApproachId::East, ApproachId::South, ApproachId::West };
    for (auto app : apps) {
        auto& approachObj = m_approaches[m_approachIndexMap[app]];
        for (int i = 0; i < 4; ++i) {
            Lane l = buildLane(app, LaneDirection::Inbound, i, inOffsets[i]);
            approachObj.inboundLaneIds.push_back(l.id);
            m_lanes.push_back(l);
            m_laneIndexMap[l.id] = m_lanes.size() - 1;
        }
        for (int i = 0; i < 2; ++i) {
            Lane l = buildLane(app, LaneDirection::Outbound, i, outOffsets[i]);
            approachObj.outboundLaneIds.push_back(l.id);
            m_lanes.push_back(l);
            m_laneIndexMap[l.id] = m_lanes.size() - 1;
        }
    }
}

void RoadNetwork::GenerateCrosswalksAndStopLines() {
    float stopDist = m_settings.stopLineDistance; // 19.5m
    float cwDist = m_settings.crosswalkDistance;   // 14.0m
    float cwWidth = m_settings.crosswalkWidth;     // 4.0m

    ApproachId apps[4] = { ApproachId::North, ApproachId::East, ApproachId::South, ApproachId::West };

    for (auto app : apps) {
        StopLine sl;
        Crosswalk cw;
        std::string appName = ToString(app);

        sl.id = "STOP_" + appName;
        sl.approach = app;
        sl.width = 4.0f * m_settings.laneWidth; // Covers all 4 inbound lanes (14.0m)
        sl.distanceToCrosswalk = m_settings.stopLineBeforeCrosswalk; // 1.5m

        cw.id = "CW_" + appName;
        cw.approach = app;
        cw.width = cwWidth;
        cw.distanceToStopLine = m_settings.stopLineBeforeCrosswalk; // 1.5m

        const auto& appObj = m_approaches[m_approachIndexMap[app]];
        sl.laneIds = appObj.inboundLaneIds;

        if (app == ApproachId::North) {
            sl.position = glm::vec3(7.75f, 0.0f, -stopDist);
            sl.normal = glm::vec3(0.0f, 0.0f, -1.0f); // opposing +Z traffic

            cw.polyline = {
                glm::vec3(-7.75f, 0.0f, -(cwDist + cwWidth * 0.5f)),
                glm::vec3( 14.75f, 0.0f, -(cwDist + cwWidth * 0.5f))
            };
        } else if (app == ApproachId::South) {
            sl.position = glm::vec3(-7.75f, 0.0f, stopDist);
            sl.normal = glm::vec3(0.0f, 0.0f, 1.0f); // opposing -Z traffic

            cw.polyline = {
                glm::vec3(-14.75f, 0.0f, cwDist + cwWidth * 0.5f),
                glm::vec3(  7.75f, 0.0f, cwDist + cwWidth * 0.5f)
            };
        } else if (app == ApproachId::East) {
            sl.position = glm::vec3(stopDist, 0.0f, 7.75f);
            sl.normal = glm::vec3(1.0f, 0.0f, 0.0f); // opposing -X traffic

            cw.polyline = {
                glm::vec3(cwDist + cwWidth * 0.5f, 0.0f, -7.75f),
                glm::vec3(cwDist + cwWidth * 0.5f, 0.0f,  14.75f)
            };
        } else if (app == ApproachId::West) {
            sl.position = glm::vec3(-stopDist, 0.0f, -7.75f);
            sl.normal = glm::vec3(-1.0f, 0.0f, 0.0f); // opposing +X traffic

            cw.polyline = {
                glm::vec3(-(cwDist + cwWidth * 0.5f), 0.0f, -14.75f),
                glm::vec3(-(cwDist + cwWidth * 0.5f), 0.0f,   7.75f)
            };
        }

        m_stopLines.push_back(sl);
        m_stopLineIndexMap[sl.id] = m_stopLines.size() - 1;

        m_crosswalks.push_back(cw);
        m_crosswalkIndexMap[cw.id] = m_crosswalks.size() - 1;
    }
}

void RoadNetwork::GenerateTurningPaths() {
    auto createPath = [this](const std::string& fromId, const std::string& toId, MovementType mov) {
        const Lane* from = GetLane(fromId);
        const Lane* to = GetLane(toId);
        if (!from || !to) return;

        TurningPath path;
        path.id = "PATH_" + fromId + "_TO_" + toId;
        path.fromLaneId = fromId;
        path.toLaneId = toId;
        path.movement = mov;

        // Path endpoints coincide exactly with lane endpoints (tolerance 0cm)
        path.p0 = from->endPoint;
        path.p3 = to->startPoint;

        glm::vec3 v0 = from->direction == LaneDirection::Inbound ?
            (from->endPoint - from->startPoint) : glm::vec3(0.0f);
        v0 = glm::normalize(glm::vec3(v0.x, 0.0f, v0.z));

        glm::vec3 v3 = (to->endPoint - to->startPoint);
        v3 = glm::normalize(glm::vec3(v3.x, 0.0f, v3.z));

        float chord = glm::distance(path.p0, path.p3);

        if (mov == MovementType::Through) {
            path.p1 = path.p0 + (chord / 3.0f) * v0;
            path.p2 = path.p3 - (chord / 3.0f) * v3;
            path.length = chord;
            path.minRadius = 1e6f;
        } else if (mov == MovementType::Right) {
            float handle = chord * 0.38f;
            path.p1 = path.p0 + handle * v0;
            path.p2 = path.p3 - handle * v3;

            // Compute arc length by numerical integration
            float len = 0.0f;
            glm::vec3 prev = path.p0;
            constexpr int steps = 30;
            for (int s = 1; s <= steps; ++s) {
                glm::vec3 curr = path.Evaluate(static_cast<float>(s) / static_cast<float>(steps));
                len += glm::distance(prev, curr);
                prev = curr;
            }
            path.length = len;
            path.minRadius = path.CalculateMinRadius();
        } else if (mov == MovementType::Left) {
            float handle = chord * 0.45f;
            path.p1 = path.p0 + handle * v0;
            path.p2 = path.p3 - handle * v3;

            float len = 0.0f;
            glm::vec3 prev = path.p0;
            constexpr int steps = 30;
            for (int s = 1; s <= steps; ++s) {
                glm::vec3 curr = path.Evaluate(static_cast<float>(s) / static_cast<float>(steps));
                len += glm::distance(prev, curr);
                prev = curr;
            }
            path.length = len;
            path.minRadius = path.CalculateMinRadius();
        }

        m_paths.push_back(path);
        m_pathIndexMap[path.id] = m_paths.size() - 1;
    };

    // North Inbound
    createPath("N_IN_0", "E_OUT_0", MovementType::Left);
    createPath("N_IN_1", "S_OUT_0", MovementType::Through);
    createPath("N_IN_2", "S_OUT_1", MovementType::Through);
    createPath("N_IN_3", "W_OUT_1", MovementType::Right);

    // East Inbound
    createPath("E_IN_0", "S_OUT_0", MovementType::Left);
    createPath("E_IN_1", "W_OUT_0", MovementType::Through);
    createPath("E_IN_2", "W_OUT_1", MovementType::Through);
    createPath("E_IN_3", "N_OUT_1", MovementType::Right);

    // South Inbound
    createPath("S_IN_0", "W_OUT_0", MovementType::Left);
    createPath("S_IN_1", "N_OUT_0", MovementType::Through);
    createPath("S_IN_2", "N_OUT_1", MovementType::Through);
    createPath("S_IN_3", "E_OUT_1", MovementType::Right);

    // West Inbound
    createPath("W_IN_0", "N_OUT_0", MovementType::Left);
    createPath("W_IN_1", "E_OUT_0", MovementType::Through);
    createPath("W_IN_2", "E_OUT_1", MovementType::Through);
    createPath("W_IN_3", "S_OUT_1", MovementType::Right);
}

void RoadNetwork::GenerateConflictZones() {
    // 1. Unprotected Left Turn vs Opposing Through conflicts
    auto addConflict = [this](const std::string& p1, const std::string& p2, const glm::vec3& center, float radius) {
        ConflictZone cz;
        cz.id = "CZ_" + p1 + "_X_" + p2;
        cz.center = center;
        cz.aabbMin = center - glm::vec3(radius, 1.0f, radius);
        cz.aabbMax = center + glm::vec3(radius, 1.0f, radius);
        cz.conflictingPathIds = { p1, p2 };
        m_conflictZones.push_back(cz);
    };

    // North Left vs South Through
    addConflict("PATH_N_IN_0_TO_E_OUT_0", "PATH_S_IN_1_TO_N_OUT_0", glm::vec3(2.5f, 0.0f, -2.5f), 3.0f);
    addConflict("PATH_N_IN_0_TO_E_OUT_0", "PATH_S_IN_2_TO_N_OUT_1", glm::vec3(6.0f, 0.0f, -2.5f), 3.0f);

    // South Left vs North Through
    addConflict("PATH_S_IN_0_TO_W_OUT_0", "PATH_N_IN_1_TO_S_OUT_0", glm::vec3(-2.5f, 0.0f, 2.5f), 3.0f);
    addConflict("PATH_S_IN_0_TO_W_OUT_0", "PATH_N_IN_2_TO_S_OUT_1", glm::vec3(-6.0f, 0.0f, 2.5f), 3.0f);

    // East Left vs West Through
    addConflict("PATH_E_IN_0_TO_S_OUT_0", "PATH_W_IN_1_TO_E_OUT_0", glm::vec3(-2.5f, 0.0f, 2.5f), 3.0f);
    addConflict("PATH_E_IN_0_TO_S_OUT_0", "PATH_W_IN_2_TO_E_OUT_1", glm::vec3(-2.5f, 0.0f, 6.0f), 3.0f);

    // West Left vs East Through
    addConflict("PATH_W_IN_0_TO_N_OUT_0", "PATH_E_IN_1_TO_W_OUT_0", glm::vec3(2.5f, 0.0f, -2.5f), 3.0f);
    addConflict("PATH_W_IN_0_TO_N_OUT_0", "PATH_E_IN_2_TO_W_OUT_1", glm::vec3(2.5f, 0.0f, -6.0f), 3.0f);

    // 2. Cross-street Through vs Through conflicts
    addConflict("PATH_N_IN_1_TO_S_OUT_0", "PATH_E_IN_1_TO_W_OUT_0", glm::vec3(0.0f), 4.0f);
    addConflict("PATH_N_IN_2_TO_S_OUT_1", "PATH_E_IN_2_TO_W_OUT_1", glm::vec3(0.0f), 4.0f);
    addConflict("PATH_S_IN_1_TO_N_OUT_0", "PATH_W_IN_1_TO_E_OUT_0", glm::vec3(0.0f), 4.0f);
    addConflict("PATH_S_IN_2_TO_N_OUT_1", "PATH_W_IN_2_TO_E_OUT_1", glm::vec3(0.0f), 4.0f);
}

void RoadNetwork::GenerateEntryExitPoints() {
    for (const auto& lane : m_lanes) {
        if (lane.direction == LaneDirection::Inbound) {
            EntryPoint ep;
            ep.position = lane.startPoint;
            ep.headingRad = lane.headingRad;
            ep.laneId = lane.id;
            m_entryPoints.push_back(ep);
        } else {
            ExitPoint xp;
            xp.position = lane.endPoint;
            xp.headingRad = lane.headingRad;
            xp.laneId = lane.id;
            m_exitPoints.push_back(xp);
        }
    }
}

// ---------------- Queries ----------------

std::vector<const Lane*> RoadNetwork::LanesFor(ApproachId approach) const {
    std::vector<const Lane*> res;
    for (const auto& l : m_lanes) {
        if (l.approach == approach) res.push_back(&l);
    }
    return res;
}

std::vector<const Lane*> RoadNetwork::InboundLanesFor(ApproachId approach) const {
    std::vector<const Lane*> res;
    for (const auto& l : m_lanes) {
        if (l.approach == approach && l.direction == LaneDirection::Inbound) res.push_back(&l);
    }
    return res;
}

std::vector<const Lane*> RoadNetwork::OutboundLanesFor(ApproachId approach) const {
    std::vector<const Lane*> res;
    for (const auto& l : m_lanes) {
        if (l.approach == approach && l.direction == LaneDirection::Outbound) res.push_back(&l);
    }
    return res;
}

std::vector<const TurningPath*> RoadNetwork::PathsFrom(const std::string& laneId) const {
    std::vector<const TurningPath*> res;
    for (const auto& p : m_paths) {
        if (p.fromLaneId == laneId) res.push_back(&p);
    }
    return res;
}

const TurningPath* RoadNetwork::GetPath(const std::string& pathId) const {
    auto it = m_pathIndexMap.find(pathId);
    if (it != m_pathIndexMap.end()) return &m_paths[it->second];
    return nullptr;
}

std::vector<std::string> RoadNetwork::ConflictsOf(const std::string& pathId) const {
    std::vector<std::string> res;
    for (const auto& cz : m_conflictZones) {
        bool contains = false;
        for (const auto& pid : cz.conflictingPathIds) {
            if (pid == pathId) {
                contains = true;
                break;
            }
        }
        if (contains) {
            for (const auto& pid : cz.conflictingPathIds) {
                if (pid != pathId) {
                    if (std::find(res.begin(), res.end(), pid) == res.end()) {
                        res.push_back(pid);
                    }
                }
            }
        }
    }
    return res;
}

const StopLine* RoadNetwork::NearestStopLine(const glm::vec3& position, float headingRad) const {
    const StopLine* best = nullptr;
    float bestDistSq = 1e9f;

    for (const auto& sl : m_stopLines) {
        glm::vec2 diff(position.x - sl.position.x, position.z - sl.position.z);
        float dSq = glm::dot(diff, diff);
        if (dSq < bestDistSq) {
            bestDistSq = dSq;
            best = &sl;
        }
    }
    return best;
}

bool RoadNetwork::IsInsideIntersection(const glm::vec3& point) const {
    float stopDist = m_settings.stopLineDistance;
    return std::abs(point.x) <= stopDist && std::abs(point.z) <= stopDist;
}

const Lane* RoadNetwork::GetLane(const std::string& laneId) const {
    auto it = m_laneIndexMap.find(laneId);
    if (it != m_laneIndexMap.end()) return &m_lanes[it->second];
    return nullptr;
}

const StopLine* RoadNetwork::GetStopLine(const std::string& stopLineId) const {
    auto it = m_stopLineIndexMap.find(stopLineId);
    if (it != m_stopLineIndexMap.end()) return &m_stopLines[it->second];
    return nullptr;
}

const Crosswalk* RoadNetwork::GetCrosswalk(const std::string& crosswalkId) const {
    auto it = m_crosswalkIndexMap.find(crosswalkId);
    if (it != m_crosswalkIndexMap.end()) return &m_crosswalks[it->second];
    return nullptr;
}

const Approach* RoadNetwork::GetApproach(ApproachId id) const {
    auto it = m_approachIndexMap.find(id);
    if (it != m_approachIndexMap.end()) return &m_approaches[it->second];
    return nullptr;
}

// ---------------- JSON Serialization ----------------

bool RoadNetwork::SaveToJson(const std::string& filepath) const {
    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::filesystem::path p2("../" + filepath);
        if (p2.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p2.parent_path(), ec);
        }
        file.open("../" + filepath);
    }
    if (!file.is_open()) {
        core::Logger::Error("Failed to open file for network export: ", filepath);
        return false;
    }

    file << std::fixed << std::setprecision(4);
    file << "{\n";
    file << "  \"network_version\": \"1.0\",\n";
    file << "  \"settings\": {\n";
    file << "    \"road_width\": " << m_settings.roadWidth << ",\n";
    file << "    \"lane_width\": " << m_settings.laneWidth << ",\n";
    file << "    \"approach_length\": " << m_settings.approachLength << ",\n";
    file << "    \"stop_line_distance\": " << m_settings.stopLineDistance << ",\n";
    file << "    \"crosswalk_distance\": " << m_settings.crosswalkDistance << ",\n";
    file << "    \"crosswalk_width\": " << m_settings.crosswalkWidth << ",\n";
    file << "    \"median_width\": " << m_settings.medianWidth << "\n";
    file << "  },\n";

    // Lanes
    file << "  \"lanes\": [\n";
    for (size_t i = 0; i < m_lanes.size(); ++i) {
        const auto& l = m_lanes[i];
        file << "    {\n";
        file << "      \"id\": \"" << l.id << "\",\n";
        file << "      \"approach\": \"" << ToString(l.approach) << "\",\n";
        file << "      \"direction\": \"" << ToString(l.direction) << "\",\n";
        file << "      \"lane_index\": " << l.laneIndex << ",\n";
        file << "      \"width\": " << l.width << ",\n";
        file << "      \"length\": " << l.length << ",\n";
        file << "      \"start\": [" << l.startPoint.x << ", " << l.startPoint.y << ", " << l.startPoint.z << "],\n";
        file << "      \"end\": [" << l.endPoint.x << ", " << l.endPoint.y << ", " << l.endPoint.z << "],\n";
        file << "      \"heading\": " << l.headingRad << "\n";
        file << "    }" << (i + 1 < m_lanes.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Turning Paths
    file << "  \"paths\": [\n";
    for (size_t i = 0; i < m_paths.size(); ++i) {
        const auto& p = m_paths[i];
        file << "    {\n";
        file << "      \"id\": \"" << p.id << "\",\n";
        file << "      \"from_lane\": \"" << p.fromLaneId << "\",\n";
        file << "      \"to_lane\": \"" << p.toLaneId << "\",\n";
        file << "      \"movement\": \"" << ToString(p.movement) << "\",\n";
        file << "      \"p0\": [" << p.p0.x << ", " << p.p0.y << ", " << p.p0.z << "],\n";
        file << "      \"p1\": [" << p.p1.x << ", " << p.p1.y << ", " << p.p1.z << "],\n";
        file << "      \"p2\": [" << p.p2.x << ", " << p.p2.y << ", " << p.p2.z << "],\n";
        file << "      \"p3\": [" << p.p3.x << ", " << p.p3.y << ", " << p.p3.z << "],\n";
        file << "      \"length\": " << p.length << ",\n";
        file << "      \"min_radius\": " << p.minRadius << "\n";
        file << "    }" << (i + 1 < m_paths.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Stop Lines
    file << "  \"stop_lines\": [\n";
    for (size_t i = 0; i < m_stopLines.size(); ++i) {
        const auto& sl = m_stopLines[i];
        file << "    {\n";
        file << "      \"id\": \"" << sl.id << "\",\n";
        file << "      \"approach\": \"" << ToString(sl.approach) << "\",\n";
        file << "      \"position\": [" << sl.position.x << ", " << sl.position.y << ", " << sl.position.z << "],\n";
        file << "      \"normal\": [" << sl.normal.x << ", " << sl.normal.y << ", " << sl.normal.z << "],\n";
        file << "      \"width\": " << sl.width << "\n";
        file << "    }" << (i + 1 < m_stopLines.size() ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Conflict Zones
    file << "  \"conflict_zones\": [\n";
    for (size_t i = 0; i < m_conflictZones.size(); ++i) {
        const auto& cz = m_conflictZones[i];
        file << "    {\n";
        file << "      \"id\": \"" << cz.id << "\",\n";
        file << "      \"center\": [" << cz.center.x << ", " << cz.center.y << ", " << cz.center.z << "],\n";
        file << "      \"path_a\": \"" << cz.conflictingPathIds[0] << "\",\n";
        file << "      \"path_b\": \"" << cz.conflictingPathIds[1] << "\"\n";
        file << "    }" << (i + 1 < m_conflictZones.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";

    core::Logger::Info("Exported road network to JSON: ", filepath);
    return true;
}

bool RoadNetwork::LoadFromJson(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        file.open("../" + filepath);
    }
    if (!file.is_open()) {
        core::Logger::Error("Failed to open network JSON file: ", filepath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Verify key fields are present
    if (content.find("\"network_version\"") == std::string::npos ||
        content.find("\"lanes\"") == std::string::npos ||
        content.find("\"paths\"") == std::string::npos) {
        core::Logger::Error("Invalid or corrupted network JSON file: ", filepath);
        return false;
    }

    // Parse settings
    auto extractFloat = [&content](const std::string& key, float defaultVal) -> float {
        std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
        std::smatch m;
        if (std::regex_search(content, m, re) && m.size() > 1) {
            return std::stof(m[1].str());
        }
        return defaultVal;
    };

    core::IntersectionSettings loadedSettings;
    loadedSettings.roadWidth = extractFloat("road_width", 14.0f);
    loadedSettings.laneWidth = extractFloat("lane_width", 3.5f);
    loadedSettings.approachLength = extractFloat("approach_length", 120.0f);
    loadedSettings.stopLineDistance = extractFloat("stop_line_distance", 19.5f);
    loadedSettings.crosswalkDistance = extractFloat("crosswalk_distance", 14.0f);
    loadedSettings.crosswalkWidth = extractFloat("crosswalk_width", 4.0f);
    loadedSettings.medianWidth = extractFloat("median_width", 1.5f);

    BuildFromSpec(loadedSettings);
    core::Logger::Info("Loaded road network from JSON: ", filepath);
    return true;
}

bool RoadNetwork::IsEquivalent(const RoadNetwork& other, float tolerance) const {
    if (m_lanes.size() != other.m_lanes.size()) return false;
    if (m_paths.size() != other.m_paths.size()) return false;
    if (m_stopLines.size() != other.m_stopLines.size()) return false;
    if (m_conflictZones.size() != other.m_conflictZones.size()) return false;

    for (size_t i = 0; i < m_lanes.size(); ++i) {
        const auto& l1 = m_lanes[i];
        const auto* l2 = other.GetLane(l1.id);
        if (!l2) return false;
        if (glm::distance(l1.startPoint, l2->startPoint) > tolerance) return false;
        if (glm::distance(l1.endPoint, l2->endPoint) > tolerance) return false;
    }

    for (size_t i = 0; i < m_paths.size(); ++i) {
        const auto& p1 = m_paths[i];
        const auto* p2 = other.GetPath(p1.id);
        if (!p2) return false;
        if (glm::distance(p1.p0, p2->p0) > tolerance) return false;
        if (glm::distance(p1.p1, p2->p1) > tolerance) return false;
        if (glm::distance(p1.p2, p2->p2) > tolerance) return false;
        if (glm::distance(p1.p3, p2->p3) > tolerance) return false;
    }

    return true;
}

} // namespace SmartCity::city

