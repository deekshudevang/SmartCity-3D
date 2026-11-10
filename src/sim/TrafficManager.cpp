#include "sim/TrafficManager.h"
#include "core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace SmartCity::sim {

TrafficManager::TrafficManager(const core::SimulationSettings& simSettings, const core::IntersectionSettings& interSettings)
    : m_simSettings(simSettings)
    , m_interSettings(interSettings)
    , m_rng(simSettings.randomSeed)
    , m_dist01(0.0f, 1.0f)
    , m_northSpawnTimer(0.0f)
    , m_southSpawnTimer(0.0f)
    , m_eastSpawnTimer(0.0f)
    , m_westSpawnTimer(0.0f)
    , m_nextVehicleId(1)
    , m_totalSpawned(0)
    , m_totalCompleted(0)
    , m_emergencyActive(false)
    , m_emergencyApproach(-1)
{
    m_laneGraph = std::make_unique<LaneGraph>(interSettings);

    core::Logger::Info("TrafficManager initialized with 12 trajectories and IDM physics.");
}

void TrafficManager::EnsureRenderingMeshes() {
    if (!m_cubeMesh) {
        m_cubeMesh = gfx::Mesh::CreateCube(glm::vec3(1.0f), glm::vec4(1.0f));
    }
    if (!m_wheelMesh) {
        m_wheelMesh = gfx::Mesh::CreateCylinder(0.35f, 0.25f, 16, glm::vec4(0.12f, 0.12f, 0.12f, 1.0f));
    }
}

void TrafficManager::Clear() {
    m_vehicles.clear();
    m_emergencyActive = false;
    m_emergencyApproach = -1;
}

Vehicle* TrafficManager::SpawnVehicle(VehicleType type, ApproachDirection approach, TurnManeuver maneuver) {
    int trajId = m_laneGraph->GetTrajectoryId(approach, maneuver);
    if (trajId < 0) return nullptr;

    const LaneTrajectory* traj = m_laneGraph->GetTrajectory(trajId);
    if (!traj) return nullptr;

    // Check if spawn lane is clear (no vehicle within initial 12m)
    for (const auto& v : m_vehicles) {
        if (v->GetTrajectoryId() == trajId && v->GetDistanceAlongPath() < 12.0f) {
            return nullptr; // Blocked, cannot spawn safely
        }
    }

    // Automotive color palette
    static const glm::vec4 palette[] = {
        glm::vec4(0.12f, 0.35f, 0.68f, 1.0f), // Deep Blue
        glm::vec4(0.72f, 0.12f, 0.12f, 1.0f), // Crimson
        glm::vec4(0.85f, 0.85f, 0.88f, 1.0f), // Silver Metallic
        glm::vec4(0.18f, 0.18f, 0.19f, 1.0f), // Charcoal Black
        glm::vec4(0.88f, 0.72f, 0.15f, 1.0f), // Taxi Yellow
        glm::vec4(0.15f, 0.55f, 0.35f, 1.0f)  // Forest Green
    };

    int colorIdx = m_totalSpawned % 6;
    glm::vec4 col = (type == VehicleType::Emergency)
        ? glm::vec4(0.92f, 0.12f, 0.12f, 1.0f)
        : palette[colorIdx];

    auto vehicle = std::make_unique<Vehicle>(m_nextVehicleId++, type, trajId, col);
    // Initial velocity close to speed limit for smooth entry
    vehicle->SetSpeed(vehicle->GetPhysics().desiredSpeed * 0.85f);
    vehicle->Update(0.001f, traj);

    Vehicle* ptr = vehicle.get();
    m_vehicles.push_back(std::move(vehicle));
    m_totalSpawned++;
    return ptr;
}

void TrafficManager::ProcessSpawning(float dt) {
    auto handleApproach = [&](ApproachDirection app, float spawnRateVpm, float& timer) {
        if (spawnRateVpm <= 0.0f) return;
        float interval = 60.0f / spawnRateVpm;
        timer += dt;
        if (timer >= interval) {
            timer = 0.0f;

            // Roll vehicle type
            float rType = m_dist01(m_rng);
            VehicleType type = VehicleType::Sedan;
            if (rType < 0.65f) {
                type = VehicleType::Sedan;
            } else if (rType < 0.87f) {
                type = VehicleType::SUV;
            } else if (rType < 0.95f) {
                type = VehicleType::Bus;
            } else {
                type = VehicleType::Emergency;
            }

            // Roll maneuver (60% Straight, 20% Right, 20% Left)
            float rTurn = m_dist01(m_rng);
            TurnManeuver turn = TurnManeuver::Straight;
            if (rTurn < 0.60f) {
                turn = TurnManeuver::Straight;
            } else if (rTurn < 0.80f) {
                turn = TurnManeuver::Right;
            } else {
                turn = TurnManeuver::Left;
            }

            SpawnVehicle(type, app, turn);
        }
    };

    handleApproach(ApproachDirection::North, m_simSettings.northSpawnRate, m_northSpawnTimer);
    handleApproach(ApproachDirection::South, m_simSettings.southSpawnRate, m_southSpawnTimer);
    handleApproach(ApproachDirection::East, m_simSettings.eastSpawnRate, m_eastSpawnTimer);
    handleApproach(ApproachDirection::West, m_simSettings.westSpawnRate, m_westSpawnTimer);
}

void TrafficManager::UpdateVehiclePhysics(Vehicle* vehicle, float dt, int nsSignal, int ewSignal, int nsLeft, int ewLeft) {
    if (!vehicle->IsActive()) return;

    int trajId = vehicle->GetTrajectoryId();
    const LaneTrajectory* traj = m_laneGraph->GetTrajectory(trajId);
    if (!traj) return;

    const auto& physics = vehicle->GetPhysics();
    float myDist = vehicle->GetDistanceAlongPath();
    float mySpeed = vehicle->GetSpeed();

    // 1. Leader vehicle car-following (same lane)
    Vehicle* leader = nullptr;
    float minAheadGap = 1e9f;
    for (const auto& other : m_vehicles) {
        if (other.get() == vehicle || !other->IsActive()) continue;
        if (other->GetTrajectoryId() == trajId) {
            float gap = other->GetDistanceAlongPath() - myDist;
            if (gap > 0.0f && gap < minAheadGap) {
                minAheadGap = gap;
                leader = other.get();
            }
        }
    }

    float accel = 0.0f;
    if (leader) {
        float netGap = minAheadGap - leader->GetDimensions().length;
        accel = IDMController::CalculateAcceleration(physics, mySpeed, leader->GetSpeed(), netGap);
    } else {
        accel = IDMController::CalculateFreeRoadAcceleration(physics, mySpeed);
    }

    // 2. Traffic Signal Compliance at Stop Bar
    float stopLineDist = traj->GetStopLineDistance();
    if (myDist < stopLineDist) {
        // Determine signal aspect for this vehicle's approach and movement
        int signal = 2; // Default Green
        ApproachDirection entry = traj->GetEntryApproach();
        TurnManeuver maneuver = traj->GetManeuver();

        if (entry == ApproachDirection::North || entry == ApproachDirection::South) {
            if (maneuver == TurnManeuver::Left && nsLeft >= 0) {
                signal = (nsLeft == 2) ? 2 : (nsSignal == 2 ? 2 : (nsSignal == 1 ? 1 : 0));
            } else {
                signal = nsSignal;
            }
        } else {
            if (maneuver == TurnManeuver::Left && ewLeft >= 0) {
                signal = (ewLeft == 2) ? 2 : (ewSignal == 2 ? 2 : (ewSignal == 1 ? 1 : 0));
            } else {
                signal = ewSignal;
            }
        }

        // Emergency vehicle siren preemption overrides signals
        if (vehicle->IsEmergency()) {
            signal = 2; // Emergency vehicle commands green
        }

        float distToStop = stopLineDist - myDist;
        if (signal == 0) {
            // RED: Stop before line
            float stopAccel = IDMController::CalculateStopLineAcceleration(physics, mySpeed, distToStop);
            accel = std::min(accel, stopAccel);
        } else if (signal == 1) {
            // YELLOW: Stop if outside dilemma zone
            if (!IDMController::IsInDilemmaZone(physics, mySpeed, distToStop)) {
                float stopAccel = IDMController::CalculateStopLineAcceleration(physics, mySpeed, distToStop);
                accel = std::min(accel, stopAccel);
            }
        }
    }

    // 3. Unprotected Left Turn Conflict Yielding (REQ-04-05)
    if (traj->GetManeuver() == TurnManeuver::Left && myDist >= (stopLineDist - 4.0f) && myDist <= (stopLineDist + 16.0f)) {
        if (!vehicle->IsEmergency()) {
            const auto& conflicts = m_laneGraph->GetConflicts(trajId);
            for (const auto& conf : conflicts) {
                float myDistToCrossing = conf.myDistance - myDist;
                if (myDistToCrossing > -1.0f && myDistToCrossing < 12.0f) {
                    // Check opposing trajectory for oncoming straight traffic
                    for (const auto& other : m_vehicles) {
                        if (other.get() == vehicle || !other->IsActive()) continue;
                        if (other->GetTrajectoryId() == conf.conflictingTrajectoryId) {
                            float otherDistToCrossing = conf.conflictingDistance - other->GetDistanceAlongPath();
                            if (otherDistToCrossing > -3.0f && otherDistToCrossing < 35.0f) {
                                float otherSpeed = std::max(other->GetSpeed(), 3.0f);
                                float ttc = otherDistToCrossing / otherSpeed;
                                // If opposing vehicle arrives within 3.5 seconds, yield!
                                if (ttc >= 0.0f && ttc < 3.5f) {
                                    float yieldAccel = IDMController::CalculateStopLineAcceleration(physics, mySpeed, std::max(myDistToCrossing, 0.5f));
                                    accel = std::min(accel, yieldAccel);
                                    vehicle->SetState(VehicleState::YieldingForConflict);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 4. Emergency Vehicle Preemption Corridor Clearing (REQ-04-06)
    if (vehicle->IsEmergency() && myDist < stopLineDist + 10.0f) {
        m_emergencyActive = true;
        m_emergencyApproach = static_cast<int>(traj->GetEntryApproach());
    }

    vehicle->SetAcceleration(accel);
    vehicle->Update(dt, traj);
}

void TrafficManager::Update(float dt, int nsSignal, int ewSignal, int nsLeft, int ewLeft) {
    // 1. Spawning
    ProcessSpawning(dt);

    // 2. Reset emergency state before scan
    m_emergencyActive = false;
    m_emergencyApproach = -1;

    // 3. Update physics for all vehicles
    for (auto& vehicle : m_vehicles) {
        UpdateVehiclePhysics(vehicle.get(), dt, nsSignal, ewSignal, nsLeft, ewLeft);
    }

    // 4. Prune vehicles that reached trajectory end
    auto it = std::remove_if(m_vehicles.begin(), m_vehicles.end(), [&](const std::unique_ptr<Vehicle>& v) {
        if (v->HasReachedEnd()) {
            m_totalCompleted++;
            return true;
        }
        return false;
    });
    m_vehicles.erase(it, m_vehicles.end());
}

float TrafficManager::GetAverageSpeed() const {
    if (m_vehicles.empty()) return 0.0f;
    float sum = 0.0f;
    for (const auto& v : m_vehicles) {
        sum += v->GetSpeed();
    }
    return sum / static_cast<float>(m_vehicles.size());
}

void TrafficManager::DrawVehicle3D(const Vehicle* vehicle, gfx::Shader* sceneShader, gfx::Shader* emissiveShader) {
    const auto& dims = vehicle->GetDimensions();
    glm::vec3 pos = vehicle->GetPosition();
    float heading = vehicle->GetHeadingRad();

    glm::mat4 base = glm::translate(glm::mat4(1.0f), pos);
    base = glm::rotate(base, heading, glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. Main Chassis
    glm::mat4 chassis = glm::translate(base, glm::vec3(0.0f, 0.0f, 0.0f));
    chassis = glm::scale(chassis, glm::vec3(dims.width, dims.height * 0.45f, dims.length));
    sceneShader->SetVec4("u_ColorOverride", vehicle->GetColor());
    sceneShader->SetInt("u_UseColorOverride", 1);
    sceneShader->SetMat4("u_Model", chassis);
    m_cubeMesh->Draw();

    // 2. Cabin / Roof (Dark tinted glass)
    float cabinLength = (vehicle->GetType() == VehicleType::Bus) ? (dims.length * 0.85f) : (dims.length * 0.52f);
    float cabinZOffset = (vehicle->GetType() == VehicleType::Bus) ? 0.0f : (-dims.length * 0.06f);
    glm::mat4 cabin = glm::translate(base, glm::vec3(0.0f, dims.height * 0.36f, cabinZOffset));
    cabin = glm::scale(cabin, glm::vec3(dims.width * 0.82f, dims.height * 0.42f, cabinLength));
    sceneShader->SetVec4("u_ColorOverride", glm::vec4(0.12f, 0.16f, 0.22f, 1.0f));
    sceneShader->SetMat4("u_Model", cabin);
    m_cubeMesh->Draw();

    // 3. Wheels
    float wheelRadius = 0.35f;
    float wheelY = -dims.height * 0.5f + wheelRadius;
    float halfW = dims.width * 0.48f;
    float axleZ = dims.length * 0.32f;

    auto drawWheel = [&](float x, float z) {
        glm::mat4 w = glm::translate(base, glm::vec3(x, wheelY, z));
        w = glm::rotate(w, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
        w = glm::scale(w, glm::vec3(1.0f, 1.0f, 1.0f));
        sceneShader->SetVec4("u_ColorOverride", glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
        sceneShader->SetMat4("u_Model", w);
        m_wheelMesh->Draw();
    };

    drawWheel(-halfW,  axleZ);
    drawWheel( halfW,  axleZ);
    drawWheel(-halfW, -axleZ);
    drawWheel( halfW, -axleZ);

    if (vehicle->GetType() == VehicleType::Bus) {
        drawWheel(-halfW, -axleZ * 0.3f);
        drawWheel( halfW, -axleZ * 0.3f);
    }

    // 4. Emissive lights (Headlights, Taillights, Emergency Siren)
    if (emissiveShader) {
        emissiveShader->Bind();

        float lightY = 0.0f;
        float frontZ = dims.length * 0.51f;
        float backZ = -dims.length * 0.51f;
        float lightSpacing = dims.width * 0.36f;

        // Front Headlights (Warm white)
        glm::vec4 headColor(1.0f, 0.96f, 0.85f, 1.0f);
        emissiveShader->SetVec4("u_EmissiveColor", headColor);

        glm::mat4 hlLeft = glm::translate(base, glm::vec3(-lightSpacing, lightY, frontZ));
        hlLeft = glm::scale(hlLeft, glm::vec3(0.25f, 0.15f, 0.06f));
        emissiveShader->SetMat4("u_Model", hlLeft);
        m_cubeMesh->Draw();

        glm::mat4 hlRight = glm::translate(base, glm::vec3(lightSpacing, lightY, frontZ));
        hlRight = glm::scale(hlRight, glm::vec3(0.25f, 0.15f, 0.06f));
        emissiveShader->SetMat4("u_Model", hlRight);
        m_cubeMesh->Draw();

        // Rear Taillights (Red)
        glm::vec4 tailColor(0.95f, 0.08f, 0.08f, 1.0f);
        emissiveShader->SetVec4("u_EmissiveColor", tailColor);

        glm::mat4 tlLeft = glm::translate(base, glm::vec3(-lightSpacing, lightY, backZ));
        tlLeft = glm::scale(tlLeft, glm::vec3(0.25f, 0.15f, 0.06f));
        emissiveShader->SetMat4("u_Model", tlLeft);
        m_cubeMesh->Draw();

        glm::mat4 tlRight = glm::translate(base, glm::vec3(lightSpacing, lightY, backZ));
        tlRight = glm::scale(tlRight, glm::vec3(0.25f, 0.15f, 0.06f));
        emissiveShader->SetMat4("u_Model", tlRight);
        m_cubeMesh->Draw();

        // Emergency Siren Strobes
        if (vehicle->IsEmergency()) {
            bool flash = vehicle->GetSirenFlashState();
            glm::vec4 redSiren = flash ? glm::vec4(1.0f, 0.05f, 0.05f, 1.0f) : glm::vec4(0.2f, 0.01f, 0.01f, 1.0f);
            glm::vec4 blueSiren = (!flash) ? glm::vec4(0.05f, 0.4f, 1.0f, 1.0f) : glm::vec4(0.01f, 0.04f, 0.2f, 1.0f);

            float sirenY = dims.height * 0.58f;
            // Left Red Light
            emissiveShader->SetVec4("u_EmissiveColor", redSiren);
            glm::mat4 sLeft = glm::translate(base, glm::vec3(-0.35f, sirenY, 0.0f));
            sLeft = glm::scale(sLeft, glm::vec3(0.35f, 0.15f, 0.35f));
            emissiveShader->SetMat4("u_Model", sLeft);
            m_cubeMesh->Draw();

            // Right Blue Light
            emissiveShader->SetVec4("u_EmissiveColor", blueSiren);
            glm::mat4 sRight = glm::translate(base, glm::vec3(0.35f, sirenY, 0.0f));
            sRight = glm::scale(sRight, glm::vec3(0.35f, 0.15f, 0.35f));
            emissiveShader->SetMat4("u_Model", sRight);
            m_cubeMesh->Draw();
        }

        sceneShader->Bind();
    }

    sceneShader->SetInt("u_UseColorOverride", 0);
}

void TrafficManager::DrawVehicleShadow(const Vehicle* vehicle, gfx::Shader* shadowShader) {
    const auto& dims = vehicle->GetDimensions();
    glm::vec3 pos = vehicle->GetPosition();
    float heading = vehicle->GetHeadingRad();

    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::rotate(model, heading, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(dims.width, dims.height, dims.length));

    shadowShader->SetMat4("u_Model", model);
    m_cubeMesh->Draw();
}

void TrafficManager::Render(gfx::Shader* sceneShader, gfx::Shader* emissiveShader) {
    EnsureRenderingMeshes();
    for (const auto& vehicle : m_vehicles) {
        if (vehicle->IsActive()) {
            DrawVehicle3D(vehicle.get(), sceneShader, emissiveShader);
        }
    }
}

void TrafficManager::RenderShadowDepth(gfx::Shader* shadowShader) {
    EnsureRenderingMeshes();
    for (const auto& vehicle : m_vehicles) {
        if (vehicle->IsActive()) {
            DrawVehicleShadow(vehicle.get(), shadowShader);
        }
    }
}

} // namespace SmartCity::sim
