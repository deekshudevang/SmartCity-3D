#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <memory>
#include "city/RoadNetwork.h"
#include "sim/ISignalProvider.h"
#include "traffic/SignalTypes.h"

namespace SmartCity::sim {
class VehicleManager;
}

namespace SmartCity::traffic {

struct SignalConfig {
    SignalMode mode{ SignalMode::FixedTime };
    float yellowDuration{ 3.0f };
    float allRedClearance{ 2.0f };
    float minGreen{ 10.0f };
    float maxGreen{ 60.0f };
    float fixedGreenNS{ 24.0f };
    float fixedGreenEW{ 20.0f };
    float pedWalkTime{ 7.0f };
    float pedWalkingSpeed{ 1.2f };
    float pedClearanceTime{ 18.3f }; // crosswalkLength (22m) / 1.2 m/s

    // Adaptive queue parameters
    float detectorStoplineDistance{ 15.0f };
    float detectorUpstreamDistance{ 80.0f };
    int queueThreshold{ 2 };
    int opposingQueueMargin{ 2 };
    float unitExtensionTime{ 3.0f };
    float starvationMaxRed{ 60.0f };

    // Emergency parameters
    float emergencyDetectionDist{ 100.0f };
    float emergencyMaxExtensionCap{ 15.0f };
    float emergencyCooldownTime{ 10.0f };
};

struct EmergencyVehicleRequest {
    int vehicleId{ 0 };
    city::ApproachId approach{ city::ApproachId::North };
    float distToStopLine{ 0.0f };
    float requestTime{ 0.0f };
};

class TrafficSignalController : public sim::ISignalProvider {
public:
    explicit TrafficSignalController(const SignalConfig& config = SignalConfig());
    virtual ~TrafficSignalController();

    bool LoadConfig(const std::string& configPath);

    void Update(float dt) override;

    // ISignalProvider interface
    sim::SignalAspect GetState(city::ApproachId approach, city::MovementType movement) const override;

    // Pedestrian signals
    PedSignalAspect GetPedestrianSignal(const std::string& crosswalkId) const;
    PedSignalAspect GetPedestrianSignal(city::ApproachId approach) const;
    bool IsPedestrianWalk(const std::string& crosswalkId) const;
    bool IsPedestrianFlashing(const std::string& crosswalkId) const;

    // Phase and state queries
    SignalPhase GetCurrentPhase() const { return m_currentPhase; }
    float GetPhaseElapsed() const { return m_phaseTimer; }
    float GetPhaseDuration() const { return m_currentPhaseDuration; }
    float GetPhaseRemaining() const { return (m_currentPhaseDuration > m_phaseTimer) ? (m_currentPhaseDuration - m_phaseTimer) : 0.0f; }
    float GetSimTime() const { return m_simTime; }

    SignalMode GetMode() const { return m_config.mode; }
    void SetMode(SignalMode mode) { m_config.mode = mode; }

    void SetVehicleManager(sim::VehicleManager* vm) { m_vehicleManager = vm; }

    // Actuation & Priorities
    void RequestPedestrianCrossing(const std::string& crosswalkId);
    void RegisterEmergencyVehicle(int vehicleId, city::ApproachId approach, float distToStopLine);
    void UnregisterEmergencyVehicle(int vehicleId);

    // Transition validation & safety
    static bool IsValidTransition(SignalPhase from, SignalPhase to);
    void AssertConflictSafety() const;
    bool HasConflictViolation() const { return m_conflictViolation; }

    // Logging & Diagnostics
    void EnableCsvLogging(const std::string& csvPath);
    void FlushLog();
    const std::vector<SignalTransitionRecord>& GetTransitionHistory() const { return m_transitionHistory; }
    const SignalConfig& GetConfig() const { return m_config; }

    // Testing and manual overrides
    void ForcePhase(SignalPhase phase);
    void SetPermanentRed(bool permRed) { m_permanentRed = permRed; }
    bool IsPermanentRed() const { return m_permanentRed; }

private:
    void ExecuteTransition(SignalPhase nextPhase, TransitionTrigger trigger, const std::string& reason);
    void StepAdaptiveLogic(float dt);
    void StepFixedLogic(float dt);
    void CheckEmergencyPreemption();
    void UpdatePedestrianSignals();
    int CountApproachQueue(city::ApproachId approach, float maxDistance) const;

    SignalConfig m_config;
    SignalPhase m_currentPhase{ SignalPhase::NS_GREEN };
    float m_phaseTimer{ 0.0f };
    float m_currentPhaseDuration{ 24.0f };
    float m_simTime{ 0.0f };

    // Starvation tracking
    float m_nsRedTimer{ 0.0f };
    float m_ewRedTimer{ 0.0f };

    // Emergency tracking
    std::unordered_map<int, EmergencyVehicleRequest> m_emergencyRequests;
    float m_lastEmergencyServiceTime{ -100.0f };
    float m_currentGreenEmergencyExtension{ 0.0f };

    // Pedestrian requests and states
    std::unordered_map<std::string, bool> m_pedestrianCallPending;
    std::unordered_map<std::string, PedSignalAspect> m_pedAspects;

    // Simulation context
    sim::VehicleManager* m_vehicleManager{ nullptr };
    bool m_permanentRed{ false };
    bool m_conflictViolation{ false };

    // Logging
    std::ofstream m_csvLogFile;
    std::vector<SignalTransitionRecord> m_transitionHistory;
};

} // namespace SmartCity::traffic

