#pragma once

#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>
#include "city/RoadNetwork.h"
#include "sim/ArcLengthLUT.h"
#include "sim/OBB.h"

namespace SmartCity::sim {

class LaneTrajectory;

enum class VehicleType {
    Sedan,
    Bus,
    Emergency,
    SUV
};

enum class VehicleState {
    Approaching,
    Stopping,
    Waiting,
    Crossing,
    Exiting,
    // Aliases for compatibility
    StoppingAtSignal = Stopping,
    YieldingForConflict = Waiting,
    TraversingIntersection = Crossing
};

struct VehicleDimensions {
    float length;
    float width;
    float height;
};

struct VehiclePhysics {
    float desiredSpeed;         // v0 (m/s)
    float turningDesiredSpeed;  // (m/s)
    float maxAcceleration;      // a (m/s^2)
    float comfortableDecel;     // b (m/s^2)
    float maxDecel;             // max deceleration limit (m/s^2)
    float minGap;               // s0 (m)
    float timeHeadway;          // T (s)
    float maxBraking;           // (m/s^2)
};

struct VehicleRoute {
    std::string inboundLaneId;
    std::string turningPathId;
    std::string outboundLaneId;

    city::ApproachId approachId{ city::ApproachId::North };
    city::MovementType movementType{ city::MovementType::Through };

    float inboundLength{ 0.0f };
    float turnLength{ 0.0f };
    float outboundLength{ 0.0f };
    float totalLength{ 0.0f };

    const city::Lane* inboundLane{ nullptr };
    const city::TurningPath* turningPath{ nullptr };
    const city::Lane* outboundLane{ nullptr };

    std::shared_ptr<ArcLengthLUT> turnLUT;

    float GetStopLineDistance() const { return inboundLength; }
    float GetExitTurnDistance() const { return inboundLength + turnLength; }

    void SampleAtDistance(float s, glm::vec3& outPos, glm::vec3& outTangent, float& outHeadingRad, float& outCurvature) const;
};

struct VehicleStateSnapshot {
    glm::vec3 position{ 0.0f };
    float headingRad{ 0.0f };
    float speed{ 0.0f };
};

class Vehicle {
public:
    Vehicle(int id, VehicleType type, const VehicleRoute& route,
            const VehicleDimensions& dims, const VehiclePhysics& physics,
            const glm::vec4& color = glm::vec4(1.0f), float spawnTime = 0.0f);

    // Legacy constructor for backward compatibility
    Vehicle(int id, VehicleType type, int legacyTrajectoryId, const glm::vec4& color = glm::vec4(1.0f));

    void Update(float dt, const LaneTrajectory* legacyTrajectory = nullptr);

    void SaveStateSnapshot();

    int GetId() const { return m_id; }
    VehicleType GetType() const { return m_type; }

    const VehicleRoute& GetRoute() const { return m_route; }
    void SetRoute(const VehicleRoute& route);

    float GetDistanceAlongRoute() const { return m_distanceAlongRoute; }
    void SetDistanceAlongRoute(float s) { m_distanceAlongRoute = s; }

    // Legacy alias
    float GetDistanceAlongPath() const { return m_distanceAlongRoute; }
    void SetDistanceAlongPath(float d) { m_distanceAlongRoute = d; }

    float GetSpeed() const { return m_speed; }
    void SetSpeed(float s) { m_speed = s; }

    float GetAcceleration() const { return m_acceleration; }
    void SetAcceleration(float a) { m_acceleration = a; }

    const glm::vec3& GetPosition() const { return m_position; }
    const glm::vec3& GetTangent() const { return m_tangent; }
    float GetHeadingRad() const { return m_headingRad; }
    float GetCurvature() const { return m_curvature; }

    const VehicleDimensions& GetDimensions() const { return m_dimensions; }
    const VehiclePhysics& GetPhysics() const { return m_physics; }
    const glm::vec4& GetColor() const { return m_color; }

    VehicleState GetState() const { return m_state; }
    void SetState(VehicleState state) { m_state = state; }

    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

    bool HasReachedEnd() const { return m_reachedEnd; }
    float GetSpawnTime() const { return m_spawnTime; }

    // 2D Oriented Bounding Box for SAT collision tests
    OrientedBoundingBox GetOBB() const;

    // Render interpolation
    glm::vec3 GetInterpolatedPosition(float alpha) const;
    float GetInterpolatedHeading(float alpha) const;
    float GetInterpolatedSpeed(float alpha) const;

    // Lateral acceleration speed limit: v_max = sqrt(a_lat_limit / max(kappa, eps))
    float GetEffectiveDesiredSpeed(float lateralAccelLimit) const;

    // Emergency vehicles
    bool IsEmergency() const { return m_type == VehicleType::Emergency; }
    bool GetSirenFlashState() const { return m_sirenFlash; }

    // Legacy helpers
    int GetTrajectoryId() const { return m_legacyTrajectoryId; }
    void SetTrajectoryId(int id) { m_legacyTrajectoryId = id; }

    static VehicleDimensions GetDefaultDimensions(VehicleType type);
    static VehiclePhysics GetDefaultPhysics(VehicleType type);
    static const char* GetVehicleTypeName(VehicleType type);

private:
    int m_id;
    VehicleType m_type;
    VehicleRoute m_route;
    VehicleDimensions m_dimensions;
    VehiclePhysics m_physics;
    glm::vec4 m_color;
    float m_spawnTime;

    float m_distanceAlongRoute;
    float m_speed;
    float m_acceleration;

    glm::vec3 m_position;
    glm::vec3 m_tangent;
    float m_headingRad;
    float m_curvature;

    VehicleState m_state;
    bool m_active;
    bool m_reachedEnd;

    float m_sirenTimer;
    bool m_sirenFlash;

    int m_legacyTrajectoryId{ -1 };

    VehicleStateSnapshot m_previousState;
    VehicleStateSnapshot m_currentState;
};

} // namespace SmartCity::sim
