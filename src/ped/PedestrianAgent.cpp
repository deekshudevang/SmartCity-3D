#include "ped/PedestrianAgent.h"
#include <cmath>
#include <algorithm>

namespace SmartCity::ped {

PedestrianAgent::PedestrianAgent(int id,
                                 const std::string& crosswalkId,
                                 const glm::vec3& startCurb,
                                 const glm::vec3& startStreet,
                                 const glm::vec3& endStreet,
                                 const glm::vec3& endCurb,
                                 float speed,
                                 float spawnTime)
    : m_id(id)
    , m_crosswalkId(crosswalkId)
    , m_startCurb(startCurb)
    , m_startStreet(startStreet)
    , m_endStreet(endStreet)
    , m_endCurb(endCurb)
    , m_speed(speed)
    , m_state(PedestrianState::Waiting)
    , m_currentPosition(startCurb)
    , m_spawnTime(spawnTime)
    , m_simTime(spawnTime)
{
    glm::vec3 dir = m_endStreet - m_startStreet;
    m_totalStreetDistance = glm::length(dir);
    if (m_totalStreetDistance > 0.001f) {
        dir = glm::normalize(dir);
        m_headingRad = std::atan2(dir.x, dir.z);
    }
}

void PedestrianAgent::Update(float dt, traffic::PedSignalAspect signalState) {
    m_simTime += dt;

    if (m_state == PedestrianState::Waiting) {
        // Pedestrian remains at curb waiting buffer
        m_currentPosition = m_startCurb;

        // RULE: Only start crossing when signal aspect is strictly WALK
        // Never start on FLASHING_DONT_WALK or DONT_WALK
        if (signalState == traffic::PedSignalAspect::Walk) {
            m_state = PedestrianState::Crossing;
            m_crossingStartTime = m_simTime;
            m_currentPosition = m_startStreet;
        }
    } else if (m_state == PedestrianState::Crossing) {
        // Advance along crosswalk path
        m_walkAnimTime += dt;
        m_distanceAlongStreet += m_speed * dt;

        float alpha = (m_totalStreetDistance > 0.001f) ? std::min(1.0f, m_distanceAlongStreet / m_totalStreetDistance) : 1.0f;
        glm::vec3 basePos = glm::mix(m_startStreet, m_endStreet, alpha);

        // Sine-driven vertical bobbing for procedural walk cycle:
        // stride frequency f ~= speed / 0.7m, amplitude 0.04m
        float strideFreq = m_speed / 0.7f;
        float bob = std::abs(std::sin(2.0f * 3.14159265f * strideFreq * m_walkAnimTime)) * 0.04f;

        m_currentPosition = basePos + glm::vec3(0.0f, bob, 0.0f);

        // Once across the roadway, step onto the destination sidewalk curb and finish
        if (m_distanceAlongStreet >= m_totalStreetDistance) {
            m_state = PedestrianState::Finished;
            m_crossingEndTime = m_simTime;
            m_currentPosition = m_endCurb;
        }
    }
}

} // namespace SmartCity::ped

