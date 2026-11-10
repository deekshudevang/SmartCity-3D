#pragma once

#include <glm/glm.hpp>
#include <string>
#include "sim/LaneGraph.h"

namespace SmartCity::sim {

enum class VehicleType {
    Sedan,
    SUV,
    Bus,
    Emergency
};

enum class VehicleState {
    Approaching,
    StoppingAtSignal,
    YieldingForConflict,
    TraversingIntersection,
    Exiting
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
    float minGap;               // s0 (m)
    float timeHeadway;          // T (s)
    float maxBraking;           // (m/s^2)
};

class Vehicle {
public:
    Vehicle(int id, VehicleType type, int trajectoryId, const glm::vec4& color = glm::vec4(1.0f));

    void Update(float dt, const LaneTrajectory* trajectory);

    int GetId() const { return m_id; }
    VehicleType GetType() const { return m_type; }
    int GetTrajectoryId() const { return m_trajectoryId; }
    void SetTrajectoryId(int id) { m_trajectoryId = id; }

    float GetDistanceAlongPath() const { return m_distanceAlongPath; }
    void SetDistanceAlongPath(float d) { m_distanceAlongPath = d; }

    float GetSpeed() const { return m_speed; }
    void SetSpeed(float s) { m_speed = s; }

    float GetAcceleration() const { return m_acceleration; }
    void SetAcceleration(float a) { m_acceleration = a; }

    const glm::vec3& GetPosition() const { return m_position; }
    const glm::vec3& GetTangent() const { return m_tangent; }
    float GetHeadingRad() const { return m_headingRad; }

    const VehicleDimensions& GetDimensions() const { return m_dimensions; }
    const VehiclePhysics& GetPhysics() const { return m_physics; }
    const glm::vec4& GetColor() const { return m_color; }

    VehicleState GetState() const { return m_state; }
    void SetState(VehicleState state) { m_state = state; }

    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

    bool HasReachedEnd() const { return m_reachedEnd; }

    // Emergency vehicles
    bool IsEmergency() const { return m_type == VehicleType::Emergency; }
    bool GetSirenFlashState() const { return m_sirenFlash; }

    static VehicleDimensions GetDefaultDimensions(VehicleType type);
    static VehiclePhysics GetDefaultPhysics(VehicleType type);
    static const char* GetVehicleTypeName(VehicleType type);

private:
    int m_id;
    VehicleType m_type;
    int m_trajectoryId;
    VehicleDimensions m_dimensions;
    VehiclePhysics m_physics;
    glm::vec4 m_color;

    float m_distanceAlongPath;
    float m_speed;
    float m_acceleration;

    glm::vec3 m_position;
    glm::vec3 m_tangent;
    float m_headingRad;

    VehicleState m_state;
    bool m_active;
    bool m_reachedEnd;

    float m_sirenTimer;
    bool m_sirenFlash;
};

} // namespace SmartCity::sim

