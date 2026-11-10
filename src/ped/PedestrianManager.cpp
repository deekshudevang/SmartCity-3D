#include "ped/PedestrianManager.h"
#include "core/Logger.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cassert>
#include <algorithm>

namespace SmartCity::ped {

PedestrianManager::PedestrianManager(const city::RoadNetwork& roadNetwork,
                                     std::shared_ptr<traffic::TrafficSignalController> signalController)
    : m_roadNetwork(roadNetwork)
    , m_signalController(signalController)
    , m_rng(42)
{
    CreateProceduralMesh();
}

PedestrianManager::~PedestrianManager() = default;

bool PedestrianManager::LoadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        core::Logger::Warn("Could not open pedestrian config: ", configPath, ", using defaults.");
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    auto extractFloat = [&content](const std::string& key, float defaultVal) -> float {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return defaultVal;
        try {
            return std::stof(content.substr(colon + 1));
        } catch (...) {
            return defaultVal;
        }
    };

    m_config.speedMin = extractFloat("speed_min", m_config.speedMin);
    m_config.speedMax = extractFloat("speed_max", m_config.speedMax);
    m_config.speedMean = extractFloat("speed_mean", m_config.speedMean);
    m_config.waitingBufferDistance = extractFloat("waiting_buffer_distance", m_config.waitingBufferDistance);
    m_config.boundingRadius = extractFloat("bounding_radius", m_config.boundingRadius);

    auto extractDemand = [&content](const std::string& cwId, float defaultVal) -> float {
        size_t pos = content.find("\"" + cwId + "\"");
        if (pos == std::string::npos) return defaultVal;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return defaultVal;
        try {
            return std::stof(content.substr(colon + 1));
        } catch (...) {
            return defaultVal;
        }
    };

    m_config.demandPerHour["CW_North"] = extractDemand("CW_North", 180.0f);
    m_config.demandPerHour["CW_South"] = extractDemand("CW_South", 180.0f);
    m_config.demandPerHour["CW_East"]  = extractDemand("CW_East", 140.0f);
    m_config.demandPerHour["CW_West"]  = extractDemand("CW_West", 140.0f);

    size_t seedPos = content.find("\"random_seed\"");
    if (seedPos != std::string::npos) {
        size_t colon = content.find(':', seedPos);
        if (colon != std::string::npos) {
            try {
                m_config.randomSeed = static_cast<uint32_t>(std::stoul(content.substr(colon + 1)));
            } catch (...) {}
        }
    }

    Reset(m_config.randomSeed);
    core::Logger::Info("Loaded pedestrian config successfully (Seed: ", m_config.randomSeed, ").");
    return true;
}

void PedestrianManager::Reset(uint32_t seed) {
    if (seed != 0) {
        m_config.randomSeed = seed;
    }
    m_rng.seed(m_config.randomSeed);
    m_pedestrians.clear();
    m_spawnTimers.clear();
    m_simTime = 0.0f;
    m_accumulator = 0.0f;
    m_nextPedestrianId = 1;
    m_totalSpawned = 0;
    m_totalCompleted = 0;
    m_illegalCrossingStarts = 0;
}

void PedestrianManager::Update(float deltaRealTime) {
    m_accumulator += deltaRealTime;
    int maxSteps = 5;
    while (m_accumulator >= m_fixedDt && maxSteps-- > 0) {
        Step(m_fixedDt);
        m_accumulator -= m_fixedDt;
    }
}

void PedestrianManager::Step(float dt) {
    m_simTime += dt;

    // 1. Spawning
    ProcessSpawning(dt);

    // 2. Pedestrian updates and safety assertions
    for (auto it = m_pedestrians.begin(); it != m_pedestrians.end();) {
        auto& ped = *it;
        if (!ped) {
            it = m_pedestrians.erase(it);
            continue;
        }

        traffic::PedSignalAspect aspect = traffic::PedSignalAspect::DontWalk;
        if (m_signalController) {
            aspect = m_signalController->GetPedestrianSignal(ped->GetCrosswalkId());
        }

        // Actuation: press call button when waiting
        if (ped->GetState() == PedestrianState::Waiting && !ped->HasPressedButton()) {
            if (m_signalController) {
                m_signalController->RequestPedestrianCrossing(ped->GetCrosswalkId());
            }
            ped->MarkButtonPressed();
        }

        PedestrianState prevState = ped->GetState();
        ped->Update(dt, aspect);

        // Verification assertion: Zero pedestrians start crossing outside WALK
        if (prevState == PedestrianState::Waiting && ped->GetState() == PedestrianState::Crossing) {
            if (aspect != traffic::PedSignalAspect::Walk) {
                m_illegalCrossingStarts++;
                core::Logger::Error("ILLEGAL PEDESTRIAN CROSSING START! Pedestrian ", ped->GetId(),
                                    " started crossing on aspect: ", traffic::ToString(aspect));
                assert(aspect == traffic::PedSignalAspect::Walk && "Pedestrian started crossing outside WALK!");
            }
        }

        if (ped->GetState() == PedestrianState::Finished) {
            m_totalCompleted++;
            it = m_pedestrians.erase(it);
        } else {
            ++it;
        }
    }
}

void PedestrianManager::ProcessSpawning(float dt) {
    for (const auto& [cwId, demand] : m_config.demandPerHour) {
        if (demand <= 0.0f) continue;

        m_spawnTimers[cwId] += dt;
        float expectedInterval = 3600.0f / demand;

        if (m_spawnTimers[cwId] >= expectedInterval) {
            m_spawnTimers[cwId] -= expectedInterval;

            // Seeded random speed in [speedMin, speedMax]
            float speed = m_config.speedMin + m_dist01(m_rng) * (m_config.speedMax - m_config.speedMin);
            bool forward = (m_dist01(m_rng) >= 0.5f);

            SpawnPedestrian(cwId, forward, speed);
        }
    }
}

PedestrianAgent* PedestrianManager::SpawnPedestrian(const std::string& crosswalkId, bool forward, float speed) {
    const city::Crosswalk* cw = m_roadNetwork.GetCrosswalk(crosswalkId);
    if (!cw || cw->polyline.size() < 2) return nullptr;

    glm::vec3 p0 = cw->polyline[0];
    glm::vec3 p1 = cw->polyline[1];
    glm::vec3 dir = p1 - p0;
    float len = glm::length(dir);
    if (len < 0.001f) return nullptr;
    dir = glm::normalize(dir);

    float buffer = m_config.waitingBufferDistance;
    float curbHeight = 0.15f;

    glm::vec3 startCurb, startStreet, endStreet, endCurb;

    if (forward) {
        startCurb   = p0 - dir * buffer + glm::vec3(0.0f, curbHeight, 0.0f);
        startStreet = p0 + glm::vec3(0.0f, 0.02f, 0.0f);
        endStreet   = p1 + glm::vec3(0.0f, 0.02f, 0.0f);
        endCurb     = p1 + dir * buffer + glm::vec3(0.0f, curbHeight, 0.0f);
    } else {
        startCurb   = p1 + dir * buffer + glm::vec3(0.0f, curbHeight, 0.0f);
        startStreet = p1 + glm::vec3(0.0f, 0.02f, 0.0f);
        endStreet   = p0 + glm::vec3(0.0f, 0.02f, 0.0f);
        endCurb     = p0 - dir * buffer + glm::vec3(0.0f, curbHeight, 0.0f);
    }

    auto agent = std::make_unique<PedestrianAgent>(m_nextPedestrianId++, crosswalkId,
                                                   startCurb, startStreet, endStreet, endCurb,
                                                   speed, m_simTime);
    PedestrianAgent* ptr = agent.get();
    m_pedestrians.push_back(std::move(agent));
    m_totalSpawned++;
    return ptr;
}

bool PedestrianManager::HasPedestriansCrossing(const std::string& crosswalkId) const {
    for (const auto& ped : m_pedestrians) {
        if (ped && ped->GetCrosswalkId() == crosswalkId && ped->IsInCrosswalk()) {
            return true;
        }
    }
    return false;
}

bool PedestrianManager::IsCrosswalkOccupied(const std::string& crosswalkId, const glm::vec3& checkPos, float corridorRadius) const {
    for (const auto& ped : m_pedestrians) {
        if (!ped || ped->GetCrosswalkId() != crosswalkId || !ped->IsInCrosswalk()) continue;

        glm::vec3 pedPos = ped->GetPosition();
        float distSq = (pedPos.x - checkPos.x) * (pedPos.x - checkPos.x) +
                       (pedPos.z - checkPos.z) * (pedPos.z - checkPos.z);
        if (distSq <= corridorRadius * corridorRadius) {
            return true;
        }
    }
    return false;
}

std::vector<glm::vec3> PedestrianManager::GetPedestrianPositionsOnCrosswalk(const std::string& crosswalkId) const {
    std::vector<glm::vec3> positions;
    for (const auto& ped : m_pedestrians) {
        if (ped && ped->GetCrosswalkId() == crosswalkId && ped->IsInCrosswalk()) {
            positions.push_back(ped->GetPosition());
        }
    }
    return positions;
}

void PedestrianManager::CreateProceduralMesh() {
    std::vector<gfx::Vertex> verts;
    std::vector<uint32_t> inds;

    auto addBox = [&](const glm::vec3& min, const glm::vec3& max, const glm::vec4& col) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        glm::vec3 p[8] = {
            { min.x, min.y, min.z }, { max.x, min.y, min.z },
            { max.x, max.y, min.z }, { min.x, max.y, min.z },
            { min.x, min.y, max.z }, { max.x, min.y, max.z },
            { max.x, max.y, max.z }, { min.x, max.y, max.z }
        };

        // Front
        verts.push_back({ p[0], {0,0,-1}, {0,0}, col });
        verts.push_back({ p[1], {0,0,-1}, {1,0}, col });
        verts.push_back({ p[2], {0,0,-1}, {1,1}, col });
        verts.push_back({ p[3], {0,0,-1}, {0,1}, col });
        inds.push_back(base); inds.push_back(base+1); inds.push_back(base+2);
        inds.push_back(base+2); inds.push_back(base+3); inds.push_back(base);

        // Back
        base += 4;
        verts.push_back({ p[5], {0,0,1}, {0,0}, col });
        verts.push_back({ p[4], {0,0,1}, {1,0}, col });
        verts.push_back({ p[7], {0,0,1}, {1,1}, col });
        verts.push_back({ p[6], {0,0,1}, {0,1}, col });
        inds.push_back(base); inds.push_back(base+1); inds.push_back(base+2);
        inds.push_back(base+2); inds.push_back(base+3); inds.push_back(base);

        // Top
        base += 4;
        verts.push_back({ p[3], {0,1,0}, {0,0}, col });
        verts.push_back({ p[2], {0,1,0}, {1,0}, col });
        verts.push_back({ p[6], {0,1,0}, {1,1}, col });
        verts.push_back({ p[7], {0,1,0}, {0,1}, col });
        inds.push_back(base); inds.push_back(base+1); inds.push_back(base+2);
        inds.push_back(base+2); inds.push_back(base+3); inds.push_back(base);

        // Bottom
        base += 4;
        verts.push_back({ p[4], {0,-1,0}, {0,0}, col });
        verts.push_back({ p[5], {0,-1,0}, {1,0}, col });
        verts.push_back({ p[1], {0,-1,0}, {1,1}, col });
        verts.push_back({ p[0], {0,-1,0}, {0,1}, col });
        inds.push_back(base); inds.push_back(base+1); inds.push_back(base+2);
        inds.push_back(base+2); inds.push_back(base+3); inds.push_back(base);

        // Right
        base += 4;
        verts.push_back({ p[1], {1,0,0}, {0,0}, col });
        verts.push_back({ p[5], {1,0,0}, {1,0}, col });
        verts.push_back({ p[6], {1,0,0}, {1,1}, col });
        verts.push_back({ p[2], {1,0,0}, {0,1}, col });
        inds.push_back(base); inds.push_back(base+1); inds.push_back(base+2);
        inds.push_back(base+2); inds.push_back(base+3); inds.push_back(base);

        // Left
        base += 4;
        verts.push_back({ p[4], {-1,0,0}, {0,0}, col });
        verts.push_back({ p[0], {-1,0,0}, {1,0}, col });
        verts.push_back({ p[3], {-1,0,0}, {1,1}, col });
        verts.push_back({ p[7], {-1,0,0}, {0,1}, col });
        inds.push_back(base); inds.push_back(base+1); inds.push_back(base+2);
        inds.push_back(base+2); inds.push_back(base+3); inds.push_back(base);
    };

    glm::vec4 shirtColor(0.25f, 0.45f, 0.85f, 1.0f);  // Blue shirt
    glm::vec4 skinColor(0.88f, 0.72f, 0.58f, 1.0f);   // Skin tone
    glm::vec4 pantsColor(0.20f, 0.22f, 0.25f, 1.0f);  // Dark pants

    // Torso: width 0.4m, depth 0.25m, height 0.65m (from y=0.8 to y=1.45)
    addBox({ -0.20f, 0.80f, -0.12f }, { 0.20f, 1.45f, 0.12f }, shirtColor);

    // Head: 0.22m cube (from y=1.45 to y=1.70)
    addBox({ -0.11f, 1.45f, -0.11f }, { 0.11f, 1.70f, 0.11f }, skinColor);

    // Left leg: (from y=0.0 to y=0.8)
    addBox({ -0.18f, 0.0f, -0.10f }, { -0.02f, 0.80f, 0.10f }, pantsColor);

    // Right leg:
    addBox({  0.02f, 0.0f, -0.10f }, {  0.18f, 0.80f, 0.10f }, pantsColor);

    if (glad_glGenVertexArrays != nullptr) {
        m_characterMesh = std::make_unique<gfx::Mesh>(verts, inds);
    }
}

void PedestrianManager::Render(gfx::Shader* shader) {
    if (!m_characterMesh) {
        CreateProceduralMesh();
    }
    if (!shader || !m_characterMesh) return;

    for (const auto& ped : m_pedestrians) {
        if (!ped || ped->GetState() == PedestrianState::Finished) continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, ped->GetPosition());
        model = glm::rotate(model, ped->GetHeading(), glm::vec3(0.0f, 1.0f, 0.0f));

        shader->SetMat4("u_Model", model);
        m_characterMesh->Draw();
    }
}

} // namespace SmartCity::ped
