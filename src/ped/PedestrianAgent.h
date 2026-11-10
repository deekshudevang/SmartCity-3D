#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "traffic/SignalTypes.h"

namespace SmartCity::ped {

enum class PedestrianState {
    Waiting,
    Crossing,
    Finished
};

inline const char* ToString(PedestrianState state) {
    switch (state) {
        case PedestrianState::Waiting:  return "Waiting";
        case PedestrianState::Crossing: return "Crossing";
        case PedestrianState::Finished: return "Finished";
    }
    return "UNKNOWN";
}

class PedestrianAgent {
public:
    PedestrianAgent(int id,
                    const std::string& crosswalkId,
                    const glm::vec3& startCurb,
                    const glm::vec3& startStreet,
                    const glm::vec3& endStreet,
                    const glm::vec3& endCurb,
                    float speed,
                    float spawnTime);

    void Update(float dt, traffic::PedSignalAspect signalState);

    int GetId() const { return m_id; }
    PedestrianState GetState() const { return m_state; }
    const std::string& GetCrosswalkId() const { return m_crosswalkId; }
    const glm::vec3& GetPosition() const { return m_currentPosition; }
    float GetHeading() const { return m_headingRad; }
    float GetSpeed() const { return m_speed; }
    float GetBoundingRadius() const { return m_boundingRadius; }
    float GetWalkAnimTime() const { return m_walkAnimTime; }

    bool HasPressedButton() const { return m_callButtonPressed; }
    void MarkButtonPressed() { m_callButtonPressed = true; }

    float GetSpawnTime() const { return m_spawnTime; }
    float GetCrossingStartTime() const { return m_crossingStartTime; }
    float GetCrossingEndTime() const { return m_crossingEndTime; }
    float GetCrossingDuration() const { return m_crossingEndTime - m_crossingStartTime; }

    bool IsInCrosswalk() const { return m_state == PedestrianState::Crossing; }

private:
    int m_id;
    std::string m_crosswalkId;
    glm::vec3 m_startCurb;
    glm::vec3 m_startStreet;
    glm::vec3 m_endStreet;
    glm::vec3 m_endCurb;

    float m_speed;
    float m_boundingRadius{ 0.3f };
    PedestrianState m_state{ PedestrianState::Waiting };

    float m_distanceAlongStreet{ 0.0f };
    float m_totalStreetDistance{ 0.0f };

    glm::vec3 m_currentPosition;
    float m_headingRad{ 0.0f };
    float m_walkAnimTime{ 0.0f };

    bool m_callButtonPressed{ false };
    float m_spawnTime{ 0.0f };
    float m_crossingStartTime{ -1.0f };
    float m_crossingEndTime{ -1.0f };
    float m_simTime{ 0.0f };
};

} // namespace SmartCity::ped
