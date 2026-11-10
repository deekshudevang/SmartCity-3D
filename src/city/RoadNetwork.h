#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "core/Config.h"

namespace SmartCity::city {

enum class ApproachId {
    North,
    East,
    South,
    West
};

enum class MovementType {
    Left,
    Through,
    Right
};

enum class LaneDirection {
    Inbound,
    Outbound
};

std::string ToString(ApproachId approach);
ApproachId ApproachFromString(const std::string& str);

std::string ToString(MovementType movement);
MovementType MovementFromString(const std::string& str);

std::string ToString(LaneDirection dir);
LaneDirection DirectionFromString(const std::string& str);

struct EntryPoint {
    glm::vec3 position{ 0.0f };
    float headingRad{ 0.0f };
    std::string laneId;
};

struct ExitPoint {
    glm::vec3 position{ 0.0f };
    float headingRad{ 0.0f };
    std::string laneId;
};

struct StopLine {
    std::string id;
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f, 0.0f, 1.0f }; // pointing against traffic
    float width{ 14.0f };
    std::vector<std::string> laneIds;
    ApproachId approach{ ApproachId::North };
    float distanceToCrosswalk{ 1.5f };
};

struct Crosswalk {
    std::string id;
    ApproachId approach{ ApproachId::North };
    std::vector<glm::vec3> polyline;
    float width{ 4.0f };
    float distanceToStopLine{ 1.5f };
};

struct Lane {
    std::string id;
    ApproachId approach{ ApproachId::North };
    LaneDirection direction{ LaneDirection::Inbound };
    int laneIndex{ 0 }; // 0-indexed from median outward
    float width{ 3.5f };
    std::vector<MovementType> allowedMovements;
    std::vector<glm::vec3> centerline;
    float length{ 120.0f };
    glm::vec3 startPoint{ 0.0f };
    glm::vec3 endPoint{ 0.0f };
    float headingRad{ 0.0f };
};

struct TurningPath {
    std::string id;
    std::string fromLaneId;
    std::string toLaneId;
    MovementType movement{ MovementType::Through };
    glm::vec3 p0{ 0.0f };
    glm::vec3 p1{ 0.0f };
    glm::vec3 p2{ 0.0f };
    glm::vec3 p3{ 0.0f };
    float length{ 0.0f };
    float minRadius{ 100.0f };

    glm::vec3 Evaluate(float t) const;
    glm::vec3 EvaluateTangent(float t) const;
    float CalculateCurvature(float t) const;
    float CalculateMinRadius() const;
};

struct ConflictZone {
    std::string id;
    glm::vec3 aabbMin{ 0.0f };
    glm::vec3 aabbMax{ 0.0f };
    std::vector<std::string> conflictingPathIds;
    glm::vec3 center{ 0.0f };
};

struct Approach {
    ApproachId id{ ApproachId::North };
    std::string name;
    glm::vec3 forwardDir{ 0.0f, 0.0f, 1.0f }; // direction toward intersection center
    float headingRad{ 0.0f };
    std::vector<std::string> inboundLaneIds;
    std::vector<std::string> outboundLaneIds;
    std::string stopLineId;
    std::string crosswalkId;
};

struct Intersection {
    std::string id{ "Intersection_0" };
    glm::vec3 center{ 0.0f };
    float coreRadius{ 20.0f };
    std::vector<ApproachId> approaches{ ApproachId::North, ApproachId::East, ApproachId::South, ApproachId::West };
};

class RoadNetwork {
public:
    RoadNetwork();
    ~RoadNetwork() = default;

    void BuildFromSpec(const core::IntersectionSettings& settings);

    bool SaveToJson(const std::string& filepath) const;
    bool LoadFromJson(const std::string& filepath);

    bool IsEquivalent(const RoadNetwork& other, float tolerance = 0.01f) const;

    // Queries required by Phase 3
    std::vector<const Lane*> LanesFor(ApproachId approach) const;
    std::vector<const Lane*> InboundLanesFor(ApproachId approach) const;
    std::vector<const Lane*> OutboundLanesFor(ApproachId approach) const;

    std::vector<const TurningPath*> PathsFrom(const std::string& laneId) const;
    const TurningPath* GetPath(const std::string& pathId) const;

    std::vector<std::string> ConflictsOf(const std::string& pathId) const;

    const StopLine* NearestStopLine(const glm::vec3& position, float headingRad) const;
    bool IsInsideIntersection(const glm::vec3& point) const;

    const Lane* GetLane(const std::string& laneId) const;
    const StopLine* GetStopLine(const std::string& stopLineId) const;
    const Crosswalk* GetCrosswalk(const std::string& crosswalkId) const;
    const Approach* GetApproach(ApproachId id) const;
    const Intersection& GetIntersection() const { return m_intersection; }

    const std::vector<Lane>& GetAllLanes() const { return m_lanes; }
    const std::vector<TurningPath>& GetAllPaths() const { return m_paths; }
    const std::vector<StopLine>& GetAllStopLines() const { return m_stopLines; }
    const std::vector<Crosswalk>& GetAllCrosswalks() const { return m_crosswalks; }
    const std::vector<ConflictZone>& GetAllConflictZones() const { return m_conflictZones; }
    const std::vector<EntryPoint>& GetEntryPoints() const { return m_entryPoints; }
    const std::vector<ExitPoint>& GetExitPoints() const { return m_exitPoints; }

    const core::IntersectionSettings& GetSettings() const { return m_settings; }

private:
    void GenerateApproaches();
    void GenerateLanes();
    void GenerateCrosswalksAndStopLines();
    void GenerateTurningPaths();
    void GenerateConflictZones();
    void GenerateEntryExitPoints();

    core::IntersectionSettings m_settings;
    Intersection m_intersection;
    std::vector<Approach> m_approaches;
    std::vector<Lane> m_lanes;
    std::vector<TurningPath> m_paths;
    std::vector<StopLine> m_stopLines;
    std::vector<Crosswalk> m_crosswalks;
    std::vector<ConflictZone> m_conflictZones;
    std::vector<EntryPoint> m_entryPoints;
    std::vector<ExitPoint> m_exitPoints;

    std::unordered_map<std::string, size_t> m_laneIndexMap;
    std::unordered_map<std::string, size_t> m_pathIndexMap;
    std::unordered_map<std::string, size_t> m_stopLineIndexMap;
    std::unordered_map<std::string, size_t> m_crosswalkIndexMap;
    std::unordered_map<ApproachId, size_t> m_approachIndexMap;
};

} // namespace SmartCity::city

