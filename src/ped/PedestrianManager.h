#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <random>
#include <string>
#include "city/RoadNetwork.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianAgent.h"
#include "gfx/Mesh.h"
#include "gfx/Shader.h"

namespace SmartCity::ped {

struct PedestrianConfig {
    float speedMin{ 1.0f };
    float speedMax{ 1.5f };
    float speedMean{ 1.25f };
    float waitingBufferDistance{ 0.5f };
    float boundingRadius{ 0.3f };
    uint32_t randomSeed{ 42 };

    std::unordered_map<std::string, float> demandPerHour{
        { "CW_North", 180.0f },
        { "CW_South", 180.0f },
        { "CW_East",  140.0f },
        { "CW_West",  140.0f }
    };
};

class PedestrianManager {
public:
    PedestrianManager(const city::RoadNetwork& roadNetwork,
                      std::shared_ptr<traffic::TrafficSignalController> signalController = nullptr);
    ~PedestrianManager();

    bool LoadConfig(const std::string& configPath);
    void Reset(uint32_t seed = 0);

    // Fixed-timestep simulation update
    void Update(float deltaRealTime);
    void Step(float dt);

    // Manual spawner for tests
    PedestrianAgent* SpawnPedestrian(const std::string& crosswalkId, bool forward = true, float speed = 1.25f);

    // Vehicle-pedestrian conflict queries
    bool HasPedestriansCrossing(const std::string& crosswalkId) const;
    bool IsCrosswalkOccupied(const std::string& crosswalkId, const glm::vec3& checkPos, float corridorRadius = 3.5f) const;
    std::vector<glm::vec3> GetPedestrianPositionsOnCrosswalk(const std::string& crosswalkId) const;

    // Accessors
    const std::vector<std::unique_ptr<PedestrianAgent>>& GetActivePedestrians() const { return m_pedestrians; }
    size_t GetActiveCount() const { return m_pedestrians.size(); }
    int GetTotalSpawned() const { return m_totalSpawned; }
    int GetTotalCompleted() const { return m_totalCompleted; }
    int GetIllegalCrossingStarts() const { return m_illegalCrossingStarts; }
    float GetSimTime() const { return m_simTime; }

    void SetSignalController(std::shared_ptr<traffic::TrafficSignalController> ctrl) { m_signalController = ctrl; }
    traffic::TrafficSignalController* GetSignalController() { return m_signalController.get(); }

    void Render(gfx::Shader* shader);

private:
    void ProcessSpawning(float dt);
    void CreateProceduralMesh();

    const city::RoadNetwork& m_roadNetwork;
    std::shared_ptr<traffic::TrafficSignalController> m_signalController;

    PedestrianConfig m_config;
    std::vector<std::unique_ptr<PedestrianAgent>> m_pedestrians;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist01{ 0.0f, 1.0f };

    std::unordered_map<std::string, float> m_spawnTimers;

    float m_simTime{ 0.0f };
    float m_accumulator{ 0.0f };
    const float m_fixedDt{ 1.0f / 60.0f };

    int m_nextPedestrianId{ 1 };
    int m_totalSpawned{ 0 };
    int m_totalCompleted{ 0 };
    int m_illegalCrossingStarts{ 0 };

    // Procedural rendering mesh
    std::unique_ptr<gfx::Mesh> m_characterMesh;
};

} // namespace SmartCity::ped
