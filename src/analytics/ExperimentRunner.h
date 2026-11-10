#pragma once

#include <vector>
#include <string>
#include <memory>
#include "city/RoadNetwork.h"
#include "sim/VehicleManager.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianManager.h"

namespace SmartCity::analytics {

struct ExperimentRunRecord {
    std::string mode;
    uint32_t seed{ 0 };
    float durationSeconds{ 0.0f };
    std::string demandLevel;
    int throughput{ 0 };
    float averageWaitingTime{ 0.0f };
    int maxQueueOverall{ 0 };
    std::array<int, 4> maxQueuePerApproach{ 0, 0, 0, 0 };
    int signalSwitches{ 0 };
    int safetyViolations{ 0 };
    int collisions{ 0 };
};

struct ExperimentSummary {
    std::string mode;
    std::string demandLevel;
    int runCount{ 0 };

    float meanThroughput{ 0.0f };
    float stdThroughput{ 0.0f };

    float meanWaitingTime{ 0.0f };
    float stdWaitingTime{ 0.0f };

    float meanMaxQueue{ 0.0f };
    float stdMaxQueue{ 0.0f };

    float meanSignalSwitches{ 0.0f };
    int totalSafetyViolations{ 0 };
    int totalCollisions{ 0 };
};

class ExperimentRunner {
public:
    static std::vector<ExperimentRunRecord> RunBenchmark(const city::RoadNetwork& roadNetwork,
                                                         float durationSeconds = 600.0f,
                                                         const std::vector<uint32_t>& seeds = { 42, 1337, 2024 },
                                                         const std::string& demandPreset = "Medium");

    static bool SaveToCsv(const std::vector<ExperimentRunRecord>& records,
                          const std::string& filepath,
                          const std::string& gitCommitHash = "HEAD");

    static bool SaveToJson(const std::vector<ExperimentRunRecord>& records,
                           const std::string& filepath,
                           const std::string& gitCommitHash = "HEAD");

    static ExperimentSummary ComputeSummary(const std::vector<ExperimentRunRecord>& records, const std::string& mode);
};

} // namespace SmartCity::analytics

