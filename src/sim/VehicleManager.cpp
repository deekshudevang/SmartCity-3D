#include "sim/VehicleManager.h"
#include "ped/PedestrianManager.h"
#include "traffic/TrafficSignalController.h"
#include "gfx/Shader.h"
#include "gfx/Mesh.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace SmartCity::sim {

static bool ParseFloat(const std::string& content, const std::string& key, float& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = std::stof(match[1].str());
        return true;
    }
    return false;
}

static bool ParseInt(const std::string& content, const std::string& key, int& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = std::stoi(match[1].str());
        return true;
    }
    return false;
}

static bool ParseUInt(const std::string& content, const std::string& key, uint32_t& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = static_cast<uint32_t>(std::stoul(match[1].str()));
        return true;
    }
    return false;
}

VehicleManager::VehicleManager(const city::RoadNetwork& roadNetwork,
                               std::shared_ptr<ISignalProvider> signalProvider)
    : m_roadNetwork(roadNetwork)
    , m_signalProvider(signalProvider)
    , m_conflictManager(roadNetwork)
{
    if (!m_signalProvider) {
        m_signalProvider = std::make_shared<FixedCycleSignalProvider>();
    }

    // Default vehicle specs
    m_typeDimensions[VehicleType::Sedan] = Vehicle::GetDefaultDimensions(VehicleType::Sedan);
    m_typeDimensions[VehicleType::Bus] = Vehicle::GetDefaultDimensions(VehicleType::Bus);
    m_typeDimensions[VehicleType::Emergency] = Vehicle::GetDefaultDimensions(VehicleType::Emergency);

    m_typePhysics[VehicleType::Sedan] = Vehicle::GetDefaultPhysics(VehicleType::Sedan);
    m_typePhysics[VehicleType::Bus] = Vehicle::GetDefaultPhysics(VehicleType::Bus);
    m_typePhysics[VehicleType::Emergency] = Vehicle::GetDefaultPhysics(VehicleType::Emergency);

    m_typeColors[VehicleType::Sedan] = glm::vec4(0.22f, 0.45f, 0.85f, 1.0f);
    m_typeColors[VehicleType::Bus] = glm::vec4(0.95f, 0.75f, 0.10f, 1.0f);
    m_typeColors[VehicleType::Emergency] = glm::vec4(0.95f, 0.15f, 0.15f, 1.0f);

    PrecomputeTurningLUTs();
    LoadConfig("assets/config/simulation.json", "assets/config/vehicles.json");
    Reset(m_config.randomSeed);
}

VehicleManager::~VehicleManager() = default;

void VehicleManager::PrecomputeTurningLUTs() {
    m_turnLUTs.clear();
    const auto& paths = m_roadNetwork.GetAllPaths();
    for (const auto& path : paths) {
        m_turnLUTs[path.id] = std::make_shared<ArcLengthLUT>(path, 100);
    }
}

bool VehicleManager::LoadConfig(const std::string& simConfigPath, const std::string& vehConfigPath) {
    bool simSuccess = false;
    std::ifstream simFile(simConfigPath);
    if (!simFile.is_open()) {
        simFile.open("../" + simConfigPath);
    }

    if (simFile.is_open()) {
        std::stringstream buf;
        buf << simFile.rdbuf();
        std::string content = buf.str();

        ParseFloat(content, "dt", m_config.dt);
        ParseInt(content, "max_steps_per_frame", m_config.maxStepsPerFrame);
        ParseFloat(content, "speed_multiplier", m_config.speedMultiplier);
        ParseUInt(content, "random_seed", m_config.randomSeed);

        ParseFloat(content, "lateral_acceleration_limit_mps2", m_config.dynamics.lateralAccelLimit);
        ParseFloat(content, "yellow_dilemma_reaction_time_s", m_config.dynamics.yellowReactionTime);

        ParseFloat(content, "north_arrivals_per_hour", m_config.demand.northArrivalsPerHour);
        ParseFloat(content, "south_arrivals_per_hour", m_config.demand.southArrivalsPerHour);
        ParseFloat(content, "east_arrivals_per_hour", m_config.demand.eastArrivalsPerHour);
        ParseFloat(content, "west_arrivals_per_hour", m_config.demand.westArrivalsPerHour);

        ParseFloat(content, "left", m_config.demand.leftPercent);
        ParseFloat(content, "through", m_config.demand.throughPercent);
        ParseFloat(content, "right", m_config.demand.rightPercent);

        ParseFloat(content, "sedan", m_config.demand.sedanPercent);
        ParseFloat(content, "bus", m_config.demand.busPercent);
        ParseFloat(content, "ambulance", m_config.demand.ambulancePercent);

        core::Logger::Info("Loaded simulation config from ", simConfigPath);
        simSuccess = true;
    } else {
        core::Logger::Warn("Could not open simulation config at ", simConfigPath, ". Using defaults.");
    }

    std::ifstream vehFile(vehConfigPath);
    if (!vehFile.is_open()) {
        vehFile.open("../" + vehConfigPath);
    }

    if (vehFile.is_open()) {
        core::Logger::Info("Loaded vehicle config from ", vehConfigPath);
    } else {
        core::Logger::Warn("Could not open vehicles config at ", vehConfigPath, ". Using defaults.");
    }

    return simSuccess;
}

void VehicleManager::Reset(uint32_t seed) {
    if (seed != 0) {
        m_config.randomSeed = seed;
    }
    m_rng.seed(m_config.randomSeed);

    m_vehicles.clear();
    m_conflictManager.Reset();

    m_northSpawnTimer = 0.5f;
    m_southSpawnTimer = 1.0f;
    m_eastSpawnTimer = 1.5f;
    m_westSpawnTimer = 2.0f;

    m_simTime = 0.0f;
    m_accumulator = 0.0f;
    m_renderAlpha = 0.0f;

    m_nextVehicleId = 1;
    m_totalSpawned = 0;
    m_totalCompleted = 0;
    m_deferredSpawnsCount = 0;
    m_collisionCount = 0;
}

VehicleRoute VehicleManager::CreateRoute(city::ApproachId approach, city::MovementType movement, int laneIndex) {
    VehicleRoute route;
    route.approachId = approach;
    route.movementType = movement;

    auto inboundLanes = m_roadNetwork.InboundLanesFor(approach);
    if (inboundLanes.empty()) return route;

    laneIndex = std::clamp(laneIndex, 0, static_cast<int>(inboundLanes.size()) - 1);
    route.inboundLane = inboundLanes[laneIndex];
    route.inboundLaneId = route.inboundLane->id;
    route.inboundLength = route.inboundLane->length;

    auto paths = m_roadNetwork.PathsFrom(route.inboundLaneId);
    for (const auto* p : paths) {
        if (p->movement == movement) {
            route.turningPath = p;
            route.turningPathId = p->id;
            break;
        }
    }

    if (!route.turningPath && !paths.empty()) {
        route.turningPath = paths.front();
        route.turningPathId = route.turningPath->id;
    }

    if (route.turningPath) {
        auto it = m_turnLUTs.find(route.turningPathId);
        if (it != m_turnLUTs.end()) {
            route.turnLUT = it->second;
            route.turnLength = route.turnLUT->GetTotalLength();
        } else {
            route.turnLength = route.turningPath->length;
        }
        route.outboundLane = m_roadNetwork.GetLane(route.turningPath->toLaneId);
        if (route.outboundLane) {
            route.outboundLaneId = route.outboundLane->id;
            route.outboundLength = route.outboundLane->length;
        }
    }

    route.totalLength = route.inboundLength + route.turnLength + route.outboundLength;
    return route;
}

VehicleType VehicleManager::SampleVehicleType() {
    float roll = m_dist01(m_rng) * 100.0f;
    if (roll < m_config.demand.sedanPercent) {
        return VehicleType::Sedan;
    } else if (roll < m_config.demand.sedanPercent + m_config.demand.busPercent) {
        return VehicleType::Bus;
    }
    return VehicleType::Emergency;
}

city::MovementType VehicleManager::SampleMovement() {
    float roll = m_dist01(m_rng) * 100.0f;
    if (roll < m_config.demand.leftPercent) {
        return city::MovementType::Left;
    } else if (roll < m_config.demand.leftPercent + m_config.demand.throughPercent) {
        return city::MovementType::Through;
    }
    return city::MovementType::Right;
}

Vehicle* VehicleManager::SpawnVehicle(city::ApproachId approach, city::MovementType movement, VehicleType type) {
    int laneIdx = 1;
    if (movement == city::MovementType::Left) {
        laneIdx = 0;
    } else if (movement == city::MovementType::Right) {
        laneIdx = 3;
    } else {
        // Balance through lanes 1 and 2
        int count1 = 0, count2 = 0;
        for (const auto& v : m_vehicles) {
            if (v->GetRoute().approachId == approach) {
                if (v->GetRoute().inboundLane && v->GetRoute().inboundLane->laneIndex == 1) count1++;
                else if (v->GetRoute().inboundLane && v->GetRoute().inboundLane->laneIndex == 2) count2++;
            }
        }
        laneIdx = (count1 <= count2) ? 1 : 2;
    }

    VehicleRoute route = CreateRoute(approach, movement, laneIdx);
    if (!route.inboundLane) return nullptr;

    const auto& dims = m_typeDimensions[type];
    const auto& physics = m_typePhysics[type];
    const auto& color = m_typeColors[type];

    // Clearance check at entry: ensure no vehicle is within entrance clearance zone
    float minBumperGap = 1e6f;
    for (const auto& v : m_vehicles) {
        if (v->GetRoute().inboundLaneId == route.inboundLaneId) {
            float rearBumperS = v->GetDistanceAlongRoute() - v->GetDimensions().length * 0.5f;
            float newFrontBumperS = dims.length * 0.5f;
            float gap = rearBumperS - newFrontBumperS;
            if (gap < minBumperGap) {
                minBumperGap = gap;
            }
        }
    }

    // Require at least 15m clearance to enter the approach lane
    if (minBumperGap < 15.0f) {
        m_deferredSpawnsCount++;
        return nullptr;
    }

    int id = m_nextVehicleId++;
    auto veh = std::make_unique<Vehicle>(id, type, route, dims, physics, color, m_simTime);
    // Safe spawn speed: guarantee comfortable deceleration within available gap
    float safeSpeed = std::sqrt(2.0f * physics.comfortableDecel * std::max(minBumperGap - physics.minGap - 3.0f, 0.0f));
    veh->SetSpeed(std::clamp(safeSpeed, 0.0f, physics.desiredSpeed * 0.7f));

    Vehicle* ptr = veh.get();
    m_vehicles.push_back(std::move(veh));
    m_totalSpawned++;
    return ptr;
}

void VehicleManager::ProcessSpawning(float dt) {
    auto processApproach = [this, dt](city::ApproachId app, float arrivalsPerHour, float& timer) {
        if (arrivalsPerHour <= 0.0f) return;
        timer -= dt;
        if (timer <= 0.0f) {
            city::MovementType mov = SampleMovement();
            VehicleType type = SampleVehicleType();
            Vehicle* spawned = SpawnVehicle(app, mov, type);

            float lambda = arrivalsPerHour / 3600.0f;
            float u = std::max(m_dist01(m_rng), 1e-4f);
            float nextInterval = -std::log(u) / lambda;

            if (spawned) {
                timer = nextInterval;
            } else {
                timer = 0.5f; // Retry soon if entry was blocked
            }
        }
    };

    processApproach(city::ApproachId::North, m_config.demand.northArrivalsPerHour, m_northSpawnTimer);
    processApproach(city::ApproachId::South, m_config.demand.southArrivalsPerHour, m_southSpawnTimer);
    processApproach(city::ApproachId::East, m_config.demand.eastArrivalsPerHour, m_eastSpawnTimer);
    processApproach(city::ApproachId::West, m_config.demand.westArrivalsPerHour, m_westSpawnTimer);
}

void VehicleManager::UpdateVehicleDynamics(Vehicle* vehicle, float dt) {
    if (!vehicle || !vehicle->IsActive()) return;

    const auto& physics = vehicle->GetPhysics();
    const auto& dims = vehicle->GetDimensions();
    const auto& route = vehicle->GetRoute();
    float s = vehicle->GetDistanceAlongRoute();
    float v = vehicle->GetSpeed();

    // 1. Calculate effective desired speed considering lateral acceleration on turn
    float v0 = vehicle->GetEffectiveDesiredSpeed(m_config.dynamics.lateralAccelLimit);

    // If approaching turn, ensure we decelerate before entering
    float stopDist = route.GetStopLineDistance();
    float exitDist = route.GetExitTurnDistance();
    if (s < stopDist && route.turnLUT && route.turnLUT->GetMaxCurvature() > 1e-4f) {
        float maxTurnSpeed = std::sqrt(m_config.dynamics.lateralAccelLimit / route.turnLUT->GetMaxCurvature());
        float distToTurn = stopDist - s;
        // Braking distance needed to reach maxTurnSpeed: (v^2 - v_turn^2) / (2 * b)
        if (v > maxTurnSpeed) {
            float brakeDist = (v * v - maxTurnSpeed * maxTurnSpeed) / (2.0f * physics.comfortableDecel);
            if (distToTurn <= brakeDist + 5.0f) {
                v0 = std::min(v0, maxTurnSpeed);
            }
        }
    }

    VehiclePhysics effPhysics = physics;
    effPhysics.desiredSpeed = v0;

    // 2. Identify leader vehicle
    const Vehicle* leader = nullptr;
    float minNetGap = 1e6f;

    for (const auto& other : m_vehicles) {
        if (other.get() == vehicle || !other->IsActive()) continue;

        const auto& otherRoute = other->GetRoute();
        float otherS = other->GetDistanceAlongRoute();

        // 1. Same inbound lane
        if (s < stopDist && otherRoute.inboundLaneId == route.inboundLaneId && otherS < otherRoute.GetStopLineDistance()) {
            if (otherS > s) {
                float otherRear = otherS - other->GetDimensions().length * 0.5f;
                float myFront = s + dims.length * 0.5f;
                float gap = otherRear - myFront;
                if (gap < minNetGap) {
                    minNetGap = gap;
                    leader = other.get();
                }
            }
        }
        // 2. Same turning path
        else if (s >= stopDist && s < exitDist && otherRoute.turningPathId == route.turningPathId && otherS < otherRoute.GetExitTurnDistance()) {
            float sTurn = s - stopDist;
            float otherSTurn = otherS - otherRoute.GetStopLineDistance();
            if (otherSTurn > sTurn) {
                float otherRear = otherSTurn - other->GetDimensions().length * 0.5f;
                float myFront = sTurn + dims.length * 0.5f;
                float gap = otherRear - myFront;
                if (gap < minNetGap) {
                    minNetGap = gap;
                    leader = other.get();
                }
            }
        }
        // 3. Same outbound lane (or follower approaching / on turn path entering same outbound lane)
        else if (!route.outboundLaneId.empty() && route.outboundLaneId == otherRoute.outboundLaneId) {
            float myOut = s - exitDist;
            float otherOut = otherS - otherRoute.GetExitTurnDistance();
            if (otherOut > myOut) {
                float otherRear = otherOut - other->GetDimensions().length * 0.5f;
                float myFront = myOut + dims.length * 0.5f;
                float gap = otherRear - myFront;
                if (gap < minNetGap) {
                    minNetGap = gap;
                    leader = other.get();
                }
            }
        }

        // 4. Physical forward safety corridor inside intersection:
        // construct a lookahead bounding box in front of vehicle to avoid T-bone / cut-off in intersection
        if (s >= stopDist - 2.0f && s < exitDist + 5.0f) {
            float lookaheadLen = std::max(v * 1.5f, 6.0f);
            glm::vec2 vehCenter(vehicle->GetPosition().x, vehicle->GetPosition().z);
            float h = vehicle->GetHeadingRad();
            glm::vec2 fwdDir(std::sin(h), std::cos(h));
            glm::vec2 boxCenter = vehCenter + fwdDir * (dims.length * 0.5f + lookaheadLen * 0.5f);
            glm::vec2 boxHalfExt(lookaheadLen * 0.5f, dims.width * 0.45f);
            OrientedBoundingBox safetyBox(boxCenter, boxHalfExt, h);

            if (CheckSATOverlap(safetyBox, other->GetOBB())) {
                // Obstacle in path! Determine physical gap along heading
                glm::vec3 rel = other->GetPosition() - vehicle->GetPosition();
                float fwdDist = glm::dot(rel, glm::vec3(fwdDir.x, 0.0f, fwdDir.y));
                const auto& otherDims = other->GetDimensions();
                float minRequiredFwd = dims.length * 0.5f + otherDims.length * 0.3f;
                if (fwdDist > minRequiredFwd) {
                    float physicalGap = std::max(fwdDist - (dims.length * 0.5f + otherDims.length * 0.5f), 0.1f);
                    if (physicalGap < minNetGap) {
                        minNetGap = physicalGap;
                        leader = other.get();
                    }
                }
            }
        }
    }

    float accel = IDMController::CalculateFreeRoadAcceleration(effPhysics, v);
    if (leader) {
        float leaderSpeed = leader->GetSpeed();
        accel = IDMController::CalculateAcceleration(effPhysics, v, leaderSpeed, minNetGap);
        if (minNetGap <= physics.minGap + 0.1f && leaderSpeed < 0.05f && v < 0.1f) {
            vehicle->SetSpeed(0.0f);
            accel = std::min(accel, 0.0f);
        }
    }

    // 3. Stop Line and Intersection Signal Compliance
    float frontBumperS = s + dims.length * 0.5f;
    float distToStopLine = stopDist - frontBumperS;

    // Emergency vehicle registration with signal controller
    if (vehicle->IsEmergency() && m_signalProvider) {
        if (auto* ctrl = dynamic_cast<traffic::TrafficSignalController*>(m_signalProvider.get())) {
            if (frontBumperS < stopDist && distToStopLine <= 100.0f && distToStopLine > 0.0f) {
                ctrl->RegisterEmergencyVehicle(vehicle->GetId(), route.approachId, distToStopLine);
            } else if (frontBumperS >= stopDist) {
                ctrl->UnregisterEmergencyVehicle(vehicle->GetId());
            }
        }
    }

    bool atOrBeforeStop = (frontBumperS < stopDist + 1.0f);
    if (atOrBeforeStop) {
        SignalAspect aspect = SignalAspect::Green;
        if (m_signalProvider) {
            aspect = m_signalProvider->GetState(route.approachId, route.movementType);
        }

        bool mustStop = false;
        if (aspect == SignalAspect::Red) {
            mustStop = true;
        } else if (aspect == SignalAspect::Yellow) {
            if (!IDMController::IsInDilemmaZone(physics, v, std::max(distToStopLine, 0.1f))) {
                mustStop = true;
            }
        }

        // Conflict zone reservation
        if (!mustStop && distToStopLine < 20.0f) {
            bool reserved = m_conflictManager.RequestReservation(vehicle->GetId(), route.turningPathId,
                                                                 m_simTime, vehicle->IsEmergency());
            if (!reserved) {
                mustStop = true;
                vehicle->SetState(VehicleState::Waiting);
            }
        }

        if (mustStop) {
            float stopLineAccel = IDMController::CalculateStopLineAcceleration(physics, v, std::max(distToStopLine, 0.05f));
            accel = std::min(accel, stopLineAccel);
            if (distToStopLine <= 0.5f) {
                if (v < 0.2f || distToStopLine <= 0.0f) {
                    vehicle->SetSpeed(0.0f);
                    accel = std::min(accel, 0.0f);
                }
            }
        }
    }

    // 4. Vehicle-Pedestrian Conflict Yielding
    // Vehicles turning or crossing crosswalks must yield to crossing pedestrians
    if (m_pedestrianManager) {
        std::string targetCrosswalkId;
        const city::Lane* depLane = route.outboundLane ? route.outboundLane : m_roadNetwork.GetLane(route.outboundLaneId);
        if (depLane) {
            switch (depLane->approach) {
                case city::ApproachId::North: targetCrosswalkId = "CW_North"; break;
                case city::ApproachId::South: targetCrosswalkId = "CW_South"; break;
                case city::ApproachId::East:  targetCrosswalkId = "CW_East";  break;
                case city::ApproachId::West:  targetCrosswalkId = "CW_West";  break;
            }
        }

        if (!targetCrosswalkId.empty() && m_pedestrianManager->HasPedestriansCrossing(targetCrosswalkId)) {
            auto pedPositions = m_pedestrianManager->GetPedestrianPositionsOnCrosswalk(targetCrosswalkId);
            glm::vec3 vehPos = vehicle->GetPosition();
            float h = vehicle->GetHeadingRad();
            glm::vec3 vehFwd(std::sin(h), 0.0f, std::cos(h));
            glm::vec3 vehLat(std::cos(h), 0.0f, -std::sin(h));

            float minPedDistance = 1e9f;
            for (const auto& pPos : pedPositions) {
                glm::vec3 rel = pPos - vehPos;
                float fwdDist = glm::dot(rel, vehFwd);
                float latDist = std::abs(glm::dot(rel, vehLat));

                // Corridor check: pedestrian is ahead within 12m and lateral gap < width/2 + 1.8m
                if (fwdDist > 0.0f && fwdDist < 12.0f && latDist < (dims.width * 0.5f + 1.8f)) {
                    if (fwdDist < minPedDistance) {
                        minPedDistance = fwdDist;
                    }
                }
            }

            if (minPedDistance < 15.0f) {
                float clearDist = std::max(0.1f, minPedDistance - dims.length * 0.5f - 1.0f);
                float pedBrakeAccel = IDMController::CalculateStopLineAcceleration(physics, v, clearDist);
                accel = std::min(accel, pedBrakeAccel);
                if (clearDist <= 0.8f && v < 0.2f) {
                    vehicle->SetSpeed(0.0f);
                    accel = std::min(accel, 0.0f);
                    vehicle->SetState(VehicleState::Waiting);
                }
            }
        }
    }

    vehicle->SetAcceleration(accel);
    vehicle->Update(dt);

    // Release reservations upon completely clearing the intersection into departure lane
    float clearIntersectionDist = exitDist + dims.length * 0.5f + 1.0f;
    if (vehicle->GetDistanceAlongRoute() >= clearIntersectionDist) {
        m_conflictManager.ReleaseReservation(vehicle->GetId());
        if (vehicle->IsEmergency() && m_signalProvider) {
            if (auto* ctrl = dynamic_cast<traffic::TrafficSignalController*>(m_signalProvider.get())) {
                ctrl->UnregisterEmergencyVehicle(vehicle->GetId());
            }
        }
    }
}

void VehicleManager::CheckCollisions() {
    size_t count = m_vehicles.size();
    for (size_t i = 0; i < count; ++i) {
        if (!m_vehicles[i]->IsActive()) continue;
        const auto& posA = m_vehicles[i]->GetPosition();
        OrientedBoundingBox obbA = m_vehicles[i]->GetOBB();

        for (size_t j = i + 1; j < count; ++j) {
            if (!m_vehicles[j]->IsActive()) continue;
            const auto& posB = m_vehicles[j]->GetPosition();

            // Broad-phase distance filter
            float dx = posA.x - posB.x;
            float dz = posA.z - posB.z;
            if (dx * dx + dz * dz > 225.0f) continue; // > 15m away

            // Narrow-phase SAT test
            if (CheckSATOverlap(obbA, m_vehicles[j]->GetOBB())) {
                m_collisionCount++;
                if (m_collisionCount <= 5) {
                    core::Logger::Warn("Collision: #", m_vehicles[i]->GetId(), " (", m_vehicles[i]->GetRoute().inboundLaneId,
                                       " pos=", posA.x, ",", posA.z, " s=", m_vehicles[i]->GetDistanceAlongRoute(),
                                       " h=", m_vehicles[i]->GetHeadingRad(), " len=", m_vehicles[i]->GetDimensions().length,
                                       ") vs #", m_vehicles[j]->GetId(), " (", m_vehicles[j]->GetRoute().inboundLaneId,
                                       " pos=", posB.x, ",", posB.z, " s=", m_vehicles[j]->GetDistanceAlongRoute(),
                                       " h=", m_vehicles[j]->GetHeadingRad(), " len=", m_vehicles[j]->GetDimensions().length, ")");
                }
            }
        }
    }
}

void VehicleManager::Step(float dt) {
    m_simTime += dt;

    for (auto& v : m_vehicles) {
        v->SaveStateSnapshot();
    }

    if (m_signalProvider) {
        m_signalProvider->Update(dt);
    }

    m_conflictManager.Update(m_simTime);

    ProcessSpawning(dt);

    for (auto& v : m_vehicles) {
        UpdateVehicleDynamics(v.get(), dt);
    }

    CheckCollisions();

    // Despawn completed vehicles
    auto it = std::remove_if(m_vehicles.begin(), m_vehicles.end(), [this](const std::unique_ptr<Vehicle>& v) {
        if (v->HasReachedEnd()) {
            m_conflictManager.ReleaseReservation(v->GetId());
            m_totalCompleted++;
            if (m_onVehicleCompleted) {
                m_onVehicleCompleted(*v);
            }
            return true;
        }
        return false;
    });
    m_vehicles.erase(it, m_vehicles.end());
}

void VehicleManager::Update(float deltaRealTime) {
    if (m_paused) return;

    m_accumulator += deltaRealTime * m_config.speedMultiplier;
    int steps = 0;
    while (m_accumulator >= m_config.dt && steps < m_config.maxStepsPerFrame) {
        Step(m_config.dt);
        m_accumulator -= m_config.dt;
        steps++;
    }

    if (steps >= m_config.maxStepsPerFrame) {
        m_accumulator = 0.0f; // Prevent spiral of death
    }

    m_renderAlpha = m_config.dt > 0.0f ? (m_accumulator / m_config.dt) : 0.0f;
}

float VehicleManager::GetAverageSpeed() const {
    if (m_vehicles.empty()) return 0.0f;
    float sum = 0.0f;
    for (const auto& v : m_vehicles) {
        sum += v->GetSpeed();
    }
    return sum / static_cast<float>(m_vehicles.size());
}

void VehicleManager::EnsureRenderingMeshes() {
    if (!m_cubeMesh) {
        m_cubeMesh = gfx::Mesh::CreateCube(glm::vec3(1.0f), glm::vec4(1.0f));
    }
    if (!m_wheelMesh) {
        m_wheelMesh = gfx::Mesh::CreateCylinder(0.35f, 0.25f, 16, glm::vec4(0.12f, 0.12f, 0.12f, 1.0f));
    }
}

void VehicleManager::DrawVehicle3D(const Vehicle* vehicle, gfx::Shader* sceneShader, gfx::Shader* emissiveShader) {
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
            emissiveShader->SetVec4("u_EmissiveColor", redSiren);
            glm::mat4 sLeft = glm::translate(base, glm::vec3(-0.35f, sirenY, 0.0f));
            sLeft = glm::scale(sLeft, glm::vec3(0.35f, 0.15f, 0.35f));
            emissiveShader->SetMat4("u_Model", sLeft);
            m_cubeMesh->Draw();

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

void VehicleManager::DrawVehicleShadow(const Vehicle* vehicle, gfx::Shader* shadowShader) {
    const auto& dims = vehicle->GetDimensions();
    glm::vec3 pos = vehicle->GetPosition();
    float heading = vehicle->GetHeadingRad();

    glm::mat4 base = glm::translate(glm::mat4(1.0f), pos);
    base = glm::rotate(base, heading, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 chassis = glm::translate(base, glm::vec3(0.0f, 0.0f, 0.0f));
    chassis = glm::scale(chassis, glm::vec3(dims.width, dims.height * 0.45f, dims.length));
    shadowShader->SetMat4("u_Model", chassis);
    m_cubeMesh->Draw();

    float cabinLength = (vehicle->GetType() == VehicleType::Bus) ? (dims.length * 0.85f) : (dims.length * 0.52f);
    float cabinZOffset = (vehicle->GetType() == VehicleType::Bus) ? 0.0f : (-dims.length * 0.06f);
    glm::mat4 cabin = glm::translate(base, glm::vec3(0.0f, dims.height * 0.36f, cabinZOffset));
    cabin = glm::scale(cabin, glm::vec3(dims.width * 0.82f, dims.height * 0.42f, cabinLength));
    shadowShader->SetMat4("u_Model", cabin);
    m_cubeMesh->Draw();
}

void VehicleManager::Render(gfx::Shader* sceneShader, gfx::Shader* emissiveShader) {
    if (!sceneShader) return;
    EnsureRenderingMeshes();

    for (const auto& vehicle : m_vehicles) {
        if (!vehicle || !vehicle->IsActive() || vehicle->HasReachedEnd()) continue;
        DrawVehicle3D(vehicle.get(), sceneShader, emissiveShader);
    }
}

void VehicleManager::RenderShadowDepth(gfx::Shader* shadowShader) {
    if (!shadowShader) return;
    EnsureRenderingMeshes();

    for (const auto& vehicle : m_vehicles) {
        if (!vehicle || !vehicle->IsActive() || vehicle->HasReachedEnd()) continue;
        DrawVehicleShadow(vehicle.get(), shadowShader);
    }
}

} // namespace SmartCity::sim
