#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <random>
#include <string>
#include "city/RoadNetwork.h"
#include "sim/Vehicle.h"
#include "sim/ArcLengthLUT.h"
#include "sim/ConflictManager.h"
#include "sim/ISignalProvider.h"
#include "sim/IDMController.h"

namespace SmartCity::ped {
class PedestrianManager;
}

namespace SmartCity::sim {

struct SimDemandConfig {
    float northArrivalsPerHour{ 720.0f };
    float southArrivalsPerHour{ 720.0f };
    float eastArrivalsPerHour{ 540.0f };
    float westArrivalsPerHour{ 540.0f };

    float leftPercent{ 20.0f };
    float throughPercent{ 65.0f };
    float rightPercent{ 15.0f };

    float sedanPercent{ 75.0f };
    float busPercent{ 15.0f };
    float ambulancePercent{ 10.0f };
};

struct SimDynamicsConfig {
    float lateralAccelLimit{ 2.8f };
    float yellowReactionTime{ 1.0f };
};

struct SimEngineConfig {
    float dt{ 0.0166666667f };
    int maxStepsPerFrame{ 5 };
    float speedMultiplier{ 1.0f };
    uint32_t randomSeed{ 1337 };

    SimDynamicsConfig dynamics;
    SimDemandConfig demand;
};

class VehicleManager {
public:
    explicit VehicleManager(const city::RoadNetwork& roadNetwork,
                            std::shared_ptr<ISignalProvider> signalProvider = nullptr);

    bool LoadConfig(const std::string& simConfigPath, const std::string& vehConfigPath);

    void Reset(uint32_t seed = 0);

    // Fixed-timestep simulation update (accumulator loop)
    void Update(float deltaRealTime);

    // Single step of exact fixed dt (for headless & determinism tests)
    void Step(float dt);

    // Manual spawner for tests & UI
    Vehicle* SpawnVehicle(city::ApproachId approach, city::MovementType movement, VehicleType type);

    // Accessors
    const std::vector<std::unique_ptr<Vehicle>>& GetActiveVehicles() const { return m_vehicles; }
    size_t GetActiveVehicleCount() const { return m_vehicles.size(); }
    int GetTotalSpawned() const { return m_totalSpawned; }
    int GetTotalCompleted() const { return m_totalCompleted; }
    int GetDeferredSpawnCount() const { return m_deferredSpawnsCount; }
    int GetCollisionCount() const { return m_collisionCount; }
    float GetSimTime() const { return m_simTime; }
    float GetRenderAlpha() const { return m_renderAlpha; }
    float GetAverageSpeed() const;

    bool IsPaused() const { return m_paused; }
    void SetPaused(bool paused) { m_paused = paused; }

    float GetSpeedMultiplier() const { return m_config.speedMultiplier; }
    void SetSpeedMultiplier(float mult) { m_config.speedMultiplier = mult; }

    ConflictManager& GetConflictManager() { return m_conflictManager; }
    const ConflictManager& GetConflictManager() const { return m_conflictManager; }

    void SetSignalProvider(std::shared_ptr<ISignalProvider> provider) { m_signalProvider = provider; }
    ISignalProvider* GetSignalProvider() { return m_signalProvider.get(); }

    void SetPedestrianManager(ped::PedestrianManager* pm) { m_pedestrianManager = pm; }
    ped::PedestrianManager* GetPedestrianManager() { return m_pedestrianManager; }

    const SimEngineConfig& GetConfig() const { return m_config; }
    SimEngineConfig& GetConfigMutable() { return m_config; }
    void SetConfig(const SimEngineConfig& cfg) { m_config = cfg; }

    // Builds a composite route from RoadNetwork
    VehicleRoute CreateRoute(city::ApproachId approach, city::MovementType movement, int laneIndex);

private:
    void PrecomputeTurningLUTs();
    void ProcessSpawning(float dt);
    void UpdateVehicleDynamics(Vehicle* vehicle, float dt);
    void CheckCollisions();

    VehicleType SampleVehicleType();
    city::MovementType SampleMovement();

    const city::RoadNetwork& m_roadNetwork;
    std::shared_ptr<ISignalProvider> m_signalProvider;
    ped::PedestrianManager* m_pedestrianManager{ nullptr };
    ConflictManager m_conflictManager;

    SimEngineConfig m_config;
    std::unordered_map<VehicleType, VehicleDimensions> m_typeDimensions;
    std::unordered_map<VehicleType, VehiclePhysics> m_typePhysics;
    std::unordered_map<VehicleType, glm::vec4> m_typeColors;

    std::unordered_map<std::string, std::shared_ptr<ArcLengthLUT>> m_turnLUTs;

    std::vector<std::unique_ptr<Vehicle>> m_vehicles;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist01{ 0.0f, 1.0f };

    float m_northSpawnTimer{ 0.0f };
    float m_southSpawnTimer{ 0.0f };
    float m_eastSpawnTimer{ 0.0f };
    float m_westSpawnTimer{ 0.0f };

    float m_simTime{ 0.0f };
    float m_accumulator{ 0.0f };
    float m_renderAlpha{ 0.0f };
    bool m_paused{ false };

    int m_nextVehicleId{ 1 };
    int m_totalSpawned{ 0 };
    int m_totalCompleted{ 0 };
    int m_deferredSpawnsCount{ 0 };
    int m_collisionCount{ 0 };
};

} // namespace SmartCity::sim

