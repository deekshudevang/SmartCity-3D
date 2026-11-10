#include "traffic/TrafficSignalController.h"
#include "sim/VehicleManager.h"
#include "core/Logger.h"
#include <algorithm>
#include <cassert>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cmath>

namespace SmartCity::traffic {

bool TrafficSignalController::IsValidTransition(SignalPhase from, SignalPhase to) {
    switch (from) {
        case SignalPhase::NS_GREEN:  return (to == SignalPhase::NS_YELLOW);
        case SignalPhase::NS_YELLOW: return (to == SignalPhase::ALL_RED_1);
        case SignalPhase::ALL_RED_1: return (to == SignalPhase::EW_GREEN);
        case SignalPhase::EW_GREEN:  return (to == SignalPhase::EW_YELLOW);
        case SignalPhase::EW_YELLOW: return (to == SignalPhase::ALL_RED_2);
        case SignalPhase::ALL_RED_2: return (to == SignalPhase::NS_GREEN);
    }
    return false;
}

TrafficSignalController::TrafficSignalController(const SignalConfig& config)
    : m_config(config)
    , m_currentPhase(SignalPhase::NS_GREEN)
    , m_phaseTimer(0.0f)
    , m_currentPhaseDuration(config.fixedGreenNS)
    , m_simTime(0.0f)
    , m_nsRedTimer(0.0f)
    , m_ewRedTimer(0.0f)
{
    // Derive pedestrian clearance time from road width / speed: 22.0m / 1.2 m/s = 18.33s
    if (m_config.pedWalkingSpeed > 0.1f) {
        m_config.pedClearanceTime = 22.0f / m_config.pedWalkingSpeed;
    }

    // Default pedestrian aspects
    m_pedAspects["CW_North"] = PedSignalAspect::DontWalk;
    m_pedAspects["CW_South"] = PedSignalAspect::DontWalk;
    m_pedAspects["CW_East"]  = PedSignalAspect::Walk;
    m_pedAspects["CW_West"]  = PedSignalAspect::Walk;
}

TrafficSignalController::~TrafficSignalController() {
    FlushLog();
}

bool TrafficSignalController::LoadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        core::Logger::Warn("Could not open signals config: ", configPath, ", using defaults.");
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

    auto extractInt = [&content](const std::string& key, int defaultVal) -> int {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return defaultVal;
        try {
            return std::stoi(content.substr(colon + 1));
        } catch (...) {
            return defaultVal;
        }
    };

    m_config.yellowDuration    = extractFloat("yellow_duration", m_config.yellowDuration);
    m_config.allRedClearance   = extractFloat("all_red_clearance", m_config.allRedClearance);
    m_config.minGreen          = extractFloat("min_green", m_config.minGreen);
    m_config.maxGreen          = extractFloat("max_green", m_config.maxGreen);
    m_config.fixedGreenNS      = extractFloat("fixed_green_ns", m_config.fixedGreenNS);
    m_config.fixedGreenEW      = extractFloat("fixed_green_ew", m_config.fixedGreenEW);
    m_config.pedWalkTime       = extractFloat("pedestrian_walk_time", m_config.pedWalkTime);
    m_config.pedWalkingSpeed   = extractFloat("pedestrian_walking_speed", m_config.pedWalkingSpeed);

    m_config.detectorStoplineDistance = extractFloat("detector_stopline_distance", m_config.detectorStoplineDistance);
    m_config.detectorUpstreamDistance = extractFloat("detector_upstream_distance", m_config.detectorUpstreamDistance);
    m_config.queueThreshold          = extractInt("queue_threshold", m_config.queueThreshold);
    m_config.opposingQueueMargin     = extractInt("opposing_queue_margin", m_config.opposingQueueMargin);
    m_config.unitExtensionTime       = extractFloat("unit_extension_time", m_config.unitExtensionTime);
    m_config.starvationMaxRed        = extractFloat("starvation_max_red", m_config.starvationMaxRed);

    m_config.emergencyDetectionDist    = extractFloat("detection_distance", m_config.emergencyDetectionDist);
    m_config.emergencyMaxExtensionCap = extractFloat("max_extension_cap", m_config.emergencyMaxExtensionCap);
    m_config.emergencyCooldownTime    = extractFloat("cooldown_time", m_config.emergencyCooldownTime);

    if (content.find("\"mode\": \"adaptive\"") != std::string::npos ||
        content.find("\"mode\":\"adaptive\"") != std::string::npos) {
        m_config.mode = SignalMode::Adaptive;
    } else {
        m_config.mode = SignalMode::FixedTime;
    }

    if (m_config.pedWalkingSpeed > 0.1f) {
        m_config.pedClearanceTime = 22.0f / m_config.pedWalkingSpeed;
    }

    m_currentPhaseDuration = (m_currentPhase == SignalPhase::NS_GREEN) ? m_config.fixedGreenNS : m_config.fixedGreenEW;
    core::Logger::Info("Loaded signal config successfully (mode: ", (m_config.mode == SignalMode::Adaptive ? "Adaptive" : "Fixed"), ").");
    return true;
}

void TrafficSignalController::EnableCsvLogging(const std::string& csvPath) {
    std::filesystem::path p(csvPath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    m_csvLogFile.open(csvPath, std::ios::out | std::ios::trunc);
    if (m_csvLogFile.is_open()) {
        m_csvLogFile << "sim_time,prev_phase,new_phase,trigger,ns_queue,ew_queue,reason\n";
        m_csvLogFile.flush();
    }
}

void TrafficSignalController::FlushLog() {
    if (m_csvLogFile.is_open()) {
        m_csvLogFile.flush();
    }
}

void TrafficSignalController::RegisterEmergencyVehicle(int vehicleId, city::ApproachId approach, float distToStopLine) {
    if (distToStopLine <= m_config.emergencyDetectionDist) {
        EmergencyVehicleRequest req;
        req.vehicleId = vehicleId;
        req.approach = approach;
        req.distToStopLine = distToStopLine;
        req.requestTime = m_simTime;
        m_emergencyRequests[vehicleId] = req;
    }
}

void TrafficSignalController::UnregisterEmergencyVehicle(int vehicleId) {
    m_emergencyRequests.erase(vehicleId);
}

void TrafficSignalController::RequestPedestrianCrossing(const std::string& crosswalkId) {
    m_pedestrianCallPending[crosswalkId] = true;
}

int TrafficSignalController::CountApproachQueue(city::ApproachId approach, float maxDistance) const {
    if (!m_vehicleManager) return 0;
    int count = 0;
    for (const auto& veh : m_vehicleManager->GetActiveVehicles()) {
        if (!veh || !veh->IsActive() || veh->HasReachedEnd()) continue;
        const auto& route = veh->GetRoute();
        if (route.approachId == approach && veh->GetState() != sim::VehicleState::Crossing &&
            veh->GetState() != sim::VehicleState::Exiting) {
            float distToStop = route.GetStopLineDistance() - (veh->GetDistanceAlongPath() + veh->GetDimensions().length * 0.5f);
            if (distToStop >= -1.0f && distToStop <= maxDistance) {
                count++;
            }
        }
    }
    return count;
}

void TrafficSignalController::Update(float dt) {
    if (m_permanentRed) {
        AssertConflictSafety();
        return;
    }

    m_simTime += dt;
    m_phaseTimer += dt;

    // Track red wait times for starvation protection
    if (m_currentPhase == SignalPhase::NS_GREEN || m_currentPhase == SignalPhase::NS_YELLOW || m_currentPhase == SignalPhase::ALL_RED_1) {
        m_ewRedTimer += dt;
        m_nsRedTimer = 0.0f;
    } else {
        m_nsRedTimer += dt;
        m_ewRedTimer = 0.0f;
    }

    // Step state machine logic based on mode
    if (m_config.mode == SignalMode::Adaptive) {
        StepAdaptiveLogic(dt);
    } else {
        StepFixedLogic(dt);
    }

    UpdatePedestrianSignals();
    AssertConflictSafety();
}

void TrafficSignalController::StepFixedLogic(float dt) {
    (void)dt;

    // Check emergency preemption even in fixed-time mode
    CheckEmergencyPreemption();

    const float eps = 1e-4f;
    if (m_phaseTimer >= m_currentPhaseDuration - eps) {
        switch (m_currentPhase) {
            case SignalPhase::NS_GREEN:
                ExecuteTransition(SignalPhase::NS_YELLOW, TransitionTrigger::Timer, "Fixed NS green expired");
                break;
            case SignalPhase::NS_YELLOW:
                ExecuteTransition(SignalPhase::ALL_RED_1, TransitionTrigger::Timer, "Yellow clearance interval expired");
                break;
            case SignalPhase::ALL_RED_1:
                ExecuteTransition(SignalPhase::EW_GREEN, TransitionTrigger::Timer, "All-red clearance interval expired");
                break;
            case SignalPhase::EW_GREEN:
                ExecuteTransition(SignalPhase::EW_YELLOW, TransitionTrigger::Timer, "Fixed EW green expired");
                break;
            case SignalPhase::EW_YELLOW:
                ExecuteTransition(SignalPhase::ALL_RED_2, TransitionTrigger::Timer, "Yellow clearance interval expired");
                break;
            case SignalPhase::ALL_RED_2:
                ExecuteTransition(SignalPhase::NS_GREEN, TransitionTrigger::Timer, "All-red clearance interval expired");
                break;
        }
    }
}

void TrafficSignalController::StepAdaptiveLogic(float dt) {
    (void)dt;

    // Check emergency preemption first
    CheckEmergencyPreemption();

    const float eps = 1e-4f;

    // Fixed clearance intervals are strictly preserved: Yellow and All-Red never shortened or modified
    if (m_currentPhase == SignalPhase::NS_YELLOW) {
        if (m_phaseTimer >= m_config.yellowDuration - eps) {
            ExecuteTransition(SignalPhase::ALL_RED_1, TransitionTrigger::Timer, "Yellow clearance completed");
        }
        return;
    } else if (m_currentPhase == SignalPhase::ALL_RED_1) {
        if (m_phaseTimer >= m_config.allRedClearance - eps) {
            ExecuteTransition(SignalPhase::EW_GREEN, TransitionTrigger::Timer, "All-red clearance completed");
        }
        return;
    } else if (m_currentPhase == SignalPhase::EW_YELLOW) {
        if (m_phaseTimer >= m_config.yellowDuration - eps) {
            ExecuteTransition(SignalPhase::ALL_RED_2, TransitionTrigger::Timer, "Yellow clearance completed");
        }
        return;
    } else if (m_currentPhase == SignalPhase::ALL_RED_2) {
        if (m_phaseTimer >= m_config.allRedClearance - eps) {
            ExecuteTransition(SignalPhase::NS_GREEN, TransitionTrigger::Timer, "All-red clearance completed");
        }
        return;
    }

    // Adaptive green phase decision (NS_GREEN or EW_GREEN)
    bool isNS = (m_currentPhase == SignalPhase::NS_GREEN);

    // Rule 1: Minimum green must ALWAYS be satisfied
    if (m_phaseTimer < m_config.minGreen - eps) {
        return;
    }

    // Measure queues from real simulation state
    int nsQueue = CountApproachQueue(city::ApproachId::North, m_config.detectorUpstreamDistance) +
                  CountApproachQueue(city::ApproachId::South, m_config.detectorUpstreamDistance);
    int ewQueue = CountApproachQueue(city::ApproachId::East, m_config.detectorUpstreamDistance) +
                  CountApproachQueue(city::ApproachId::West, m_config.detectorUpstreamDistance);

    int currentQueue = isNS ? nsQueue : ewQueue;
    int opposingQueue = isNS ? ewQueue : nsQueue;
    float opposingRedTime = isNS ? m_ewRedTimer : m_nsRedTimer;

    // Rule 2: Maximum green cap
    if (m_phaseTimer >= m_config.maxGreen - eps) {
        ExecuteTransition(isNS ? SignalPhase::NS_YELLOW : SignalPhase::EW_YELLOW,
                          TransitionTrigger::Timer, "Max green limit reached");
        return;
    }

    // Rule 3: Starvation protection - opposing approach has waited too long
    if (opposingRedTime >= m_config.starvationMaxRed && opposingQueue > 0) {
        ExecuteTransition(isNS ? SignalPhase::NS_YELLOW : SignalPhase::EW_YELLOW,
                          TransitionTrigger::Adaptive, "Starvation protection: opposing red threshold exceeded");
        return;
    }

    // Rule 4: Gap-out - current approach has no queued or approaching vehicles
    if (currentQueue == 0) {
        ExecuteTransition(isNS ? SignalPhase::NS_YELLOW : SignalPhase::EW_YELLOW,
                          TransitionTrigger::Adaptive, "Gap-out: current approach empty");
        return;
    }

    // Rule 5: Opposing demand significantly higher than current demand
    if (opposingQueue >= currentQueue + m_config.opposingQueueMargin) {
        ExecuteTransition(isNS ? SignalPhase::NS_YELLOW : SignalPhase::EW_YELLOW,
                          TransitionTrigger::Adaptive, "Opposing queue margin exceeded");
        return;
    }

    // Otherwise, current green is extended up to maxGreen
    m_currentPhaseDuration = std::min(m_config.maxGreen, m_phaseTimer + m_config.unitExtensionTime);
}

void TrafficSignalController::CheckEmergencyPreemption() {
    if (m_emergencyRequests.empty()) return;

    // Rate-limiting / cooldown check
    if (m_simTime - m_lastEmergencyServiceTime < m_config.emergencyCooldownTime) {
        return;
    }

    // Select highest priority ambulance (tie-break: smallest distance to stop line, then oldest request)
    int bestVehicleId = -1;
    float minDistance = 1e9f;
    city::ApproachId targetApproach = city::ApproachId::North;

    for (const auto& [id, req] : m_emergencyRequests) {
        if (req.distToStopLine < minDistance) {
            minDistance = req.distToStopLine;
            bestVehicleId = id;
            targetApproach = req.approach;
        }
    }

    if (bestVehicleId < 0) return;

    bool isNSApproach = (targetApproach == city::ApproachId::North || targetApproach == city::ApproachId::South);

    // Case A: Ambulance approach is currently GREEN
    if ((isNSApproach && m_currentPhase == SignalPhase::NS_GREEN) ||
        (!isNSApproach && m_currentPhase == SignalPhase::EW_GREEN))
    {
        // Hold green with extension cap
        float cap = m_config.maxGreen + m_config.emergencyMaxExtensionCap;
        if (m_currentPhaseDuration < cap) {
            m_currentPhaseDuration = cap;
        }
        return;
    }

    // Case B: Ambulance approach is currently RED
    // Must respect minimum green of current phase before triggering yellow
    if (m_currentPhase == SignalPhase::NS_GREEN) {
        if (m_phaseTimer >= m_config.minGreen) {
            m_lastEmergencyServiceTime = m_simTime;
            ExecuteTransition(SignalPhase::NS_YELLOW, TransitionTrigger::Emergency,
                              "Emergency preemption: ambulance requested EW service");
        }
    } else if (m_currentPhase == SignalPhase::EW_GREEN) {
        if (m_phaseTimer >= m_config.minGreen) {
            m_lastEmergencyServiceTime = m_simTime;
            ExecuteTransition(SignalPhase::EW_YELLOW, TransitionTrigger::Emergency,
                              "Emergency preemption: ambulance requested NS service");
        }
    }
}

void TrafficSignalController::ExecuteTransition(SignalPhase nextPhase, TransitionTrigger trigger, const std::string& reason) {
    if (!IsValidTransition(m_currentPhase, nextPhase)) {
        core::Logger::Error("ILLEGAL SIGNAL TRANSITION ATTEMPTED: ", ToString(m_currentPhase), " -> ", ToString(nextPhase));
        assert(false && "Illegal signal transition");
        // Safe fallback in release builds: force ALL_RED
        m_conflictViolation = true;
        nextPhase = SignalPhase::ALL_RED_1;
    }

    int nsQ = CountApproachQueue(city::ApproachId::North, m_config.detectorUpstreamDistance) +
              CountApproachQueue(city::ApproachId::South, m_config.detectorUpstreamDistance);
    int ewQ = CountApproachQueue(city::ApproachId::East, m_config.detectorUpstreamDistance) +
              CountApproachQueue(city::ApproachId::West, m_config.detectorUpstreamDistance);

    SignalTransitionRecord record;
    record.timestamp = m_simTime;
    record.prevPhase = m_currentPhase;
    record.newPhase = nextPhase;
    record.trigger = trigger;
    record.nsQueue = nsQ;
    record.ewQueue = ewQ;
    record.reason = reason;

    m_transitionHistory.push_back(record);

    if (m_csvLogFile.is_open()) {
        m_csvLogFile << m_simTime << ","
                     << ToString(m_currentPhase) << ","
                     << ToString(nextPhase) << ","
                     << ToString(trigger) << ","
                     << nsQ << ","
                     << ewQ << ",\""
                     << reason << "\"\n";
        m_csvLogFile.flush();
    }

    m_currentPhase = nextPhase;
    m_phaseTimer = 0.0f;

    // Set duration for the new phase
    switch (m_currentPhase) {
        case SignalPhase::NS_GREEN:
            m_currentPhaseDuration = (m_config.mode == SignalMode::FixedTime) ? m_config.fixedGreenNS : m_config.minGreen;
            break;
        case SignalPhase::NS_YELLOW:
            m_currentPhaseDuration = m_config.yellowDuration;
            break;
        case SignalPhase::ALL_RED_1:
            m_currentPhaseDuration = m_config.allRedClearance;
            break;
        case SignalPhase::EW_GREEN:
            m_currentPhaseDuration = (m_config.mode == SignalMode::FixedTime) ? m_config.fixedGreenEW : m_config.minGreen;
            break;
        case SignalPhase::EW_YELLOW:
            m_currentPhaseDuration = m_config.yellowDuration;
            break;
        case SignalPhase::ALL_RED_2:
            m_currentPhaseDuration = m_config.allRedClearance;
            break;
    }
}

void TrafficSignalController::UpdatePedestrianSignals() {
    // Pedestrians parallel to vehicle traffic get WALK during early green,
    // followed by FLASHING_DONT_WALK during clearance time, and DONT_WALK during yellow and red.

    // All crosswalks default to DontWalk
    m_pedAspects["CW_North"] = PedSignalAspect::DontWalk;
    m_pedAspects["CW_South"] = PedSignalAspect::DontWalk;
    m_pedAspects["CW_East"]  = PedSignalAspect::DontWalk;
    m_pedAspects["CW_West"]  = PedSignalAspect::DontWalk;

    if (m_currentPhase == SignalPhase::NS_GREEN) {
        // Parallel crosswalks are East and West
        PedSignalAspect aspect = PedSignalAspect::DontWalk;
        if (m_phaseTimer < m_config.pedWalkTime) {
            aspect = PedSignalAspect::Walk;
        } else if (m_phaseTimer < (m_config.pedWalkTime + m_config.pedClearanceTime)) {
            aspect = PedSignalAspect::FlashingDontWalk;
        }
        m_pedAspects["CW_East"] = aspect;
        m_pedAspects["CW_West"] = aspect;
    } else if (m_currentPhase == SignalPhase::EW_GREEN) {
        // Parallel crosswalks are North and South
        PedSignalAspect aspect = PedSignalAspect::DontWalk;
        if (m_phaseTimer < m_config.pedWalkTime) {
            aspect = PedSignalAspect::Walk;
        } else if (m_phaseTimer < (m_config.pedWalkTime + m_config.pedClearanceTime)) {
            aspect = PedSignalAspect::FlashingDontWalk;
        }
        m_pedAspects["CW_North"] = aspect;
        m_pedAspects["CW_South"] = aspect;
    }
}

void TrafficSignalController::AssertConflictSafety() const {
    if (m_permanentRed) return;

    sim::SignalAspect nsThrough = GetState(city::ApproachId::North, city::MovementType::Through);
    sim::SignalAspect ewThrough = GetState(city::ApproachId::East, city::MovementType::Through);
    sim::SignalAspect nsLeft    = GetState(city::ApproachId::North, city::MovementType::Left);
    sim::SignalAspect ewLeft    = GetState(city::ApproachId::East, city::MovementType::Left);

    // Conflict Check 1: Conflicting through greens
    bool conflict = false;
    if ((nsThrough == sim::SignalAspect::Green || nsThrough == sim::SignalAspect::Yellow) &&
        (ewThrough == sim::SignalAspect::Green || ewThrough == sim::SignalAspect::Yellow)) {
        conflict = true;
    }

    // Conflict Check 2: Conflicting through and left greens
    if (nsThrough == sim::SignalAspect::Green && ewLeft == sim::SignalAspect::Green) conflict = true;
    if (ewThrough == sim::SignalAspect::Green && nsLeft == sim::SignalAspect::Green) conflict = true;

    // Conflict Check 3: Pedestrian WALK conflict with perpendicular through green
    if (nsThrough == sim::SignalAspect::Green) {
        if (GetPedestrianSignal("CW_North") == PedSignalAspect::Walk ||
            GetPedestrianSignal("CW_South") == PedSignalAspect::Walk) {
            conflict = true;
        }
    }
    if (ewThrough == sim::SignalAspect::Green) {
        if (GetPedestrianSignal("CW_East") == PedSignalAspect::Walk ||
            GetPedestrianSignal("CW_West") == PedSignalAspect::Walk) {
            conflict = true;
        }
    }

    if (conflict) {
        core::Logger::Error("FATAL SAFETY MONITOR CONFLICT DETECTED AT TIME ", m_simTime, "!");
        assert(!conflict && "Conflict matrix safety violation: two conflicting movements active!");
    }
}

sim::SignalAspect TrafficSignalController::GetState(city::ApproachId approach, city::MovementType movement) const {
    if (m_permanentRed || m_conflictViolation) {
        return sim::SignalAspect::Red;
    }

    bool isNS = (approach == city::ApproachId::North || approach == city::ApproachId::South);

    switch (m_currentPhase) {
        case SignalPhase::NS_GREEN:
            if (isNS) {
                return sim::SignalAspect::Green;
            }
            return sim::SignalAspect::Red;

        case SignalPhase::NS_YELLOW:
            return isNS ? sim::SignalAspect::Yellow : sim::SignalAspect::Red;

        case SignalPhase::ALL_RED_1:
            return sim::SignalAspect::Red;

        case SignalPhase::EW_GREEN:
            if (!isNS) {
                return sim::SignalAspect::Green;
            }
            return sim::SignalAspect::Red;

        case SignalPhase::EW_YELLOW:
            return (!isNS) ? sim::SignalAspect::Yellow : sim::SignalAspect::Red;

        case SignalPhase::ALL_RED_2:
            return sim::SignalAspect::Red;
    }

    return sim::SignalAspect::Red;
}

PedSignalAspect TrafficSignalController::GetPedestrianSignal(const std::string& crosswalkId) const {
    if (m_permanentRed || m_conflictViolation) {
        return PedSignalAspect::DontWalk;
    }
    auto it = m_pedAspects.find(crosswalkId);
    if (it != m_pedAspects.end()) return it->second;
    return PedSignalAspect::DontWalk;
}

PedSignalAspect TrafficSignalController::GetPedestrianSignal(city::ApproachId approach) const {
    switch (approach) {
        case city::ApproachId::North: return GetPedestrianSignal("CW_North");
        case city::ApproachId::South: return GetPedestrianSignal("CW_South");
        case city::ApproachId::East:  return GetPedestrianSignal("CW_East");
        case city::ApproachId::West:  return GetPedestrianSignal("CW_West");
    }
    return PedSignalAspect::DontWalk;
}

bool TrafficSignalController::IsPedestrianWalk(const std::string& crosswalkId) const {
    return GetPedestrianSignal(crosswalkId) == PedSignalAspect::Walk;
}

bool TrafficSignalController::IsPedestrianFlashing(const std::string& crosswalkId) const {
    return GetPedestrianSignal(crosswalkId) == PedSignalAspect::FlashingDontWalk;
}

void TrafficSignalController::ForcePhase(SignalPhase phase) {
    m_currentPhase = phase;
    m_phaseTimer = 0.0f;
    UpdatePedestrianSignals();
}

} // namespace SmartCity::traffic
