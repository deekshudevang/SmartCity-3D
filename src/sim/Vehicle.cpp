#include "sim/Vehicle.h"
#include "sim/LaneGraph.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace SmartCity::sim {

void VehicleRoute::SampleAtDistance(float s, glm::vec3& outPos, glm::vec3& outTangent,
                                    float& outHeadingRad, float& outCurvature) const
{
    if (inboundLane && s < inboundLength) {
        float span = std::max(inboundLength, 0.001f);
        float tau = std::clamp(s / span, 0.0f, 1.0f);
        outPos = glm::mix(inboundLane->startPoint, inboundLane->endPoint, tau);
        glm::vec3 dir = inboundLane->endPoint - inboundLane->startPoint;
        float len = glm::length(dir);
        outTangent = len > 1e-4f ? dir / len : glm::vec3(0.0f, 0.0f, 1.0f);
        outHeadingRad = std::atan2(outTangent.x, outTangent.z);
        outCurvature = 0.0f;
        return;
    }

    float stopDist = inboundLength;
    float exitDist = inboundLength + turnLength;

    if (turningPath && s < exitDist) {
        float sTurn = s - stopDist;
        if (turnLUT) {
            turnLUT->Sample(sTurn, outPos, outTangent, outHeadingRad, outCurvature);
        } else {
            float tau = turnLength > 1e-4f ? std::clamp(sTurn / turnLength, 0.0f, 1.0f) : 0.0f;
            outPos = turningPath->Evaluate(tau);
            outTangent = turningPath->EvaluateTangent(tau);
            outHeadingRad = std::atan2(outTangent.x, outTangent.z);
            outCurvature = turningPath->CalculateCurvature(tau);
        }
        return;
    }

    if (outboundLane) {
        float sOut = s - exitDist;
        float span = std::max(outboundLength, 0.001f);
        float tau = std::clamp(sOut / span, 0.0f, 1.0f);
        outPos = glm::mix(outboundLane->startPoint, outboundLane->endPoint, tau);
        glm::vec3 dir = outboundLane->endPoint - outboundLane->startPoint;
        float len = glm::length(dir);
        outTangent = len > 1e-4f ? dir / len : glm::vec3(0.0f, 0.0f, 1.0f);
        outHeadingRad = std::atan2(outTangent.x, outTangent.z);
        outCurvature = 0.0f;
        return;
    }

    outPos = glm::vec3(0.0f);
    outTangent = glm::vec3(0.0f, 0.0f, 1.0f);
    outHeadingRad = 0.0f;
    outCurvature = 0.0f;
}

VehicleDimensions Vehicle::GetDefaultDimensions(VehicleType type) {
    switch (type) {
    case VehicleType::Sedan:
        return { 4.5f, 1.8f, 1.4f };
    case VehicleType::Bus:
        return { 12.0f, 2.5f, 3.2f };
    case VehicleType::Emergency:
        return { 5.5f, 2.0f, 2.2f };
    case VehicleType::SUV:
        return { 5.0f, 2.0f, 1.8f };
    }
    return { 4.5f, 1.8f, 1.4f };
}

VehiclePhysics Vehicle::GetDefaultPhysics(VehicleType type) {
    switch (type) {
    case VehicleType::Sedan:
        return { 16.67f, 6.0f, 2.6f, 2.5f, 6.5f, 2.0f, 1.2f, 8.0f };
    case VehicleType::Bus:
        return { 11.11f, 4.5f, 1.2f, 1.8f, 5.0f, 3.5f, 1.8f, 6.0f };
    case VehicleType::Emergency:
        return { 20.0f, 8.0f, 3.2f, 3.0f, 7.5f, 1.8f, 1.0f, 9.0f };
    case VehicleType::SUV:
        return { 15.0f, 5.5f, 2.2f, 2.2f, 6.0f, 2.2f, 1.3f, 8.0f };
    }
    return { 16.67f, 6.0f, 2.6f, 2.5f, 6.5f, 2.0f, 1.2f, 8.0f };
}

const char* Vehicle::GetVehicleTypeName(VehicleType type) {
    switch (type) {
    case VehicleType::Sedan: return "Sedan";
    case VehicleType::Bus: return "Bus";
    case VehicleType::Emergency: return "Emergency";
    case VehicleType::SUV: return "SUV";
    }
    return "Unknown";
}

Vehicle::Vehicle(int id, VehicleType type, const VehicleRoute& route,
                 const VehicleDimensions& dims, const VehiclePhysics& physics,
                 const glm::vec4& color, float spawnTime)
    : m_id(id)
    , m_type(type)
    , m_route(route)
    , m_dimensions(dims)
    , m_physics(physics)
    , m_color(color)
    , m_spawnTime(spawnTime)
    , m_distanceAlongRoute(0.0f)
    , m_speed(0.0f)
    , m_acceleration(0.0f)
    , m_position(0.0f)
    , m_tangent(0.0f, 0.0f, 1.0f)
    , m_headingRad(0.0f)
    , m_curvature(0.0f)
    , m_state(VehicleState::Approaching)
    , m_active(true)
    , m_reachedEnd(false)
    , m_sirenTimer(0.0f)
    , m_sirenFlash(false)
{
    if (type == VehicleType::Emergency && color == glm::vec4(1.0f)) {
        m_color = glm::vec4(0.95f, 0.15f, 0.15f, 1.0f);
    }

    m_route.SampleAtDistance(0.0f, m_position, m_tangent, m_headingRad, m_curvature);
    m_position.y = m_dimensions.height * 0.5f;

    m_currentState = { m_position, m_headingRad, m_speed };
    m_previousState = m_currentState;
}

Vehicle::Vehicle(int id, VehicleType type, int legacyTrajectoryId, const glm::vec4& color)
    : Vehicle(id, type, VehicleRoute{}, GetDefaultDimensions(type), GetDefaultPhysics(type), color, 0.0f)
{
    m_legacyTrajectoryId = legacyTrajectoryId;
}

void Vehicle::SetRoute(const VehicleRoute& route) {
    m_route = route;
    m_route.SampleAtDistance(m_distanceAlongRoute, m_position, m_tangent, m_headingRad, m_curvature);
    m_position.y = m_dimensions.height * 0.5f;
    m_currentState = { m_position, m_headingRad, m_speed };
    m_previousState = m_currentState;
}

void Vehicle::SaveStateSnapshot() {
    m_previousState = m_currentState;
    m_currentState = { m_position, m_headingRad, m_speed };
}

void Vehicle::Update(float dt, const LaneTrajectory* legacyTrajectory) {
    if (!m_active) return;

    if (m_type == VehicleType::Emergency) {
        m_sirenTimer += dt;
        if (m_sirenTimer >= 0.125f) {
            m_sirenTimer = 0.0f;
            m_sirenFlash = !m_sirenFlash;
        }
    }

    // Integrate acceleration and speed
    m_speed = std::clamp(m_speed + m_acceleration * dt, 0.0f, m_physics.desiredSpeed);
    m_distanceAlongRoute += m_speed * dt;

    if (legacyTrajectory) {
        float totalLen = legacyTrajectory->GetTotalLength();
        if (m_distanceAlongRoute >= totalLen) {
            m_distanceAlongRoute = totalLen;
            m_active = false;
            m_reachedEnd = true;
        }

        PathPoint pt = legacyTrajectory->SampleAtDistance(m_distanceAlongRoute);
        m_position = pt.position;
        m_position.y = m_dimensions.height * 0.5f;
        m_tangent = pt.tangent;
        m_headingRad = pt.headingRad;

        float stopDist = legacyTrajectory->GetStopLineDistance();
        if (m_distanceAlongRoute < stopDist - 2.0f) {
            m_state = (m_speed < 0.1f) ? VehicleState::Stopping : VehicleState::Approaching;
        } else if (m_distanceAlongRoute <= stopDist + 26.0f) {
            m_state = VehicleState::Crossing;
        } else {
            m_state = VehicleState::Exiting;
        }

        m_currentState = { m_position, m_headingRad, m_speed };
        return;
    }

    float totalLen = m_route.totalLength;
    if (totalLen > 0.0f && m_distanceAlongRoute >= totalLen) {
        m_distanceAlongRoute = totalLen;
        m_active = false;
        m_reachedEnd = true;
    }

    m_route.SampleAtDistance(m_distanceAlongRoute, m_position, m_tangent, m_headingRad, m_curvature);
    m_position.y = m_dimensions.height * 0.5f;

    // State classification
    float stopDist = m_route.GetStopLineDistance();
    float exitDist = m_route.GetExitTurnDistance();

    if (m_distanceAlongRoute < stopDist - 2.0f) {
        if (m_speed < 0.1f) {
            m_state = VehicleState::Stopping;
        } else {
            m_state = VehicleState::Approaching;
        }
    } else if (m_distanceAlongRoute < stopDist + 0.5f && m_speed < 0.1f) {
        m_state = VehicleState::Waiting;
    } else if (m_distanceAlongRoute < exitDist) {
        m_state = VehicleState::Crossing;
    } else {
        m_state = VehicleState::Exiting;
    }

    m_currentState = { m_position, m_headingRad, m_speed };
}

OrientedBoundingBox Vehicle::GetOBB() const {
    glm::vec2 center(m_position.x, m_position.z);
    glm::vec2 halfExt(m_dimensions.length * 0.5f, m_dimensions.width * 0.5f);
    return OrientedBoundingBox(center, halfExt, m_headingRad);
}

glm::vec3 Vehicle::GetInterpolatedPosition(float alpha) const {
    return glm::mix(m_previousState.position, m_currentState.position, std::clamp(alpha, 0.0f, 1.0f));
}

float Vehicle::GetInterpolatedHeading(float alpha) const {
    float diff = m_currentState.headingRad - m_previousState.headingRad;
    while (diff < -glm::pi<float>()) diff += glm::two_pi<float>();
    while (diff > glm::pi<float>()) diff -= glm::two_pi<float>();
    return m_previousState.headingRad + diff * std::clamp(alpha, 0.0f, 1.0f);
}

float Vehicle::GetInterpolatedSpeed(float alpha) const {
    return glm::mix(m_previousState.speed, m_currentState.speed, std::clamp(alpha, 0.0f, 1.0f));
}

float Vehicle::GetEffectiveDesiredSpeed(float lateralAccelLimit) const {
    if (m_curvature > 1e-4f) {
        float maxTurnSpeed = std::sqrt(lateralAccelLimit / m_curvature);
        return std::min(m_physics.desiredSpeed, maxTurnSpeed);
    }
    return m_physics.desiredSpeed;
}

} // namespace SmartCity::sim
