#include "sim/Vehicle.h"
#include <algorithm>
#include <cmath>

namespace SmartCity::sim {

VehicleDimensions Vehicle::GetDefaultDimensions(VehicleType type) {
    switch (type) {
    case VehicleType::Sedan:
        return { 4.5f, 1.8f, 1.4f };
    case VehicleType::SUV:
        return { 5.0f, 2.0f, 1.8f };
    case VehicleType::Bus:
        return { 10.5f, 2.6f, 3.2f };
    case VehicleType::Emergency:
        return { 5.5f, 2.1f, 2.2f };
    }
    return { 4.5f, 1.8f, 1.4f };
}

VehiclePhysics Vehicle::GetDefaultPhysics(VehicleType type) {
    switch (type) {
    case VehicleType::Sedan:
        return { 13.88f, 5.55f, 2.5f, 2.0f, 2.0f, 1.2f, 8.0f };
    case VehicleType::SUV:
        return { 13.0f, 5.0f, 2.2f, 2.0f, 2.2f, 1.3f, 8.0f };
    case VehicleType::Bus:
        return { 10.0f, 4.5f, 1.2f, 1.5f, 3.0f, 1.8f, 6.0f };
    case VehicleType::Emergency:
        return { 18.0f, 7.0f, 3.5f, 3.0f, 1.8f, 1.0f, 9.0f };
    }
    return { 13.88f, 5.55f, 2.5f, 2.0f, 2.0f, 1.2f, 8.0f };
}

const char* Vehicle::GetVehicleTypeName(VehicleType type) {
    switch (type) {
    case VehicleType::Sedan: return "Sedan";
    case VehicleType::SUV: return "SUV";
    case VehicleType::Bus: return "Bus";
    case VehicleType::Emergency: return "Emergency";
    }
    return "Unknown";
}

Vehicle::Vehicle(int id, VehicleType type, int trajectoryId, const glm::vec4& color)
    : m_id(id)
    , m_type(type)
    , m_trajectoryId(trajectoryId)
    , m_dimensions(GetDefaultDimensions(type))
    , m_physics(GetDefaultPhysics(type))
    , m_color(color)
    , m_distanceAlongPath(0.0f)
    , m_speed(0.0f)
    , m_acceleration(0.0f)
    , m_position(0.0f)
    , m_tangent(0.0f, 0.0f, 1.0f)
    , m_headingRad(0.0f)
    , m_state(VehicleState::Approaching)
    , m_active(true)
    , m_reachedEnd(false)
    , m_sirenTimer(0.0f)
    , m_sirenFlash(false)
{
    // If color was default white and type is emergency, use high-visibility white/red
    if (type == VehicleType::Emergency && color == glm::vec4(1.0f)) {
        m_color = glm::vec4(0.92f, 0.15f, 0.15f, 1.0f); // Emergency red
    }
}

void Vehicle::Update(float dt, const LaneTrajectory* trajectory) {
    if (!m_active || !trajectory) return;

    // Siren strobe animation (4 Hz toggle)
    if (m_type == VehicleType::Emergency) {
        m_sirenTimer += dt;
        if (m_sirenTimer >= 0.125f) {
            m_sirenTimer = 0.0f;
            m_sirenFlash = !m_sirenFlash;
        }
    }

    // Integrate acceleration and speed
    m_speed = std::clamp(m_speed + m_acceleration * dt, 0.0f, m_physics.desiredSpeed);
    m_distanceAlongPath += m_speed * dt;

    // Sample spatial trajectory
    float totalLen = trajectory->GetTotalLength();
    if (m_distanceAlongPath >= totalLen) {
        m_distanceAlongPath = totalLen;
        m_active = false;
        m_reachedEnd = true;
    }

    PathPoint pt = trajectory->SampleAtDistance(m_distanceAlongPath);
    m_position = pt.position;
    // Lift vehicle slightly so tires contact the pavement at Y = 0
    m_position.y = m_dimensions.height * 0.5f;
    m_tangent = pt.tangent;
    m_headingRad = pt.headingRad;

    // Update state based on distance relative to stop line
    float stopDist = trajectory->GetStopLineDistance();
    if (m_distanceAlongPath < stopDist - 2.0f) {
        if (m_speed < 0.1f) {
            m_state = VehicleState::StoppingAtSignal;
        } else {
            m_state = VehicleState::Approaching;
        }
    } else if (m_distanceAlongPath <= stopDist + 26.0f) {
        m_state = VehicleState::TraversingIntersection;
    } else {
        m_state = VehicleState::Exiting;
    }
}

} // namespace SmartCity::sim

