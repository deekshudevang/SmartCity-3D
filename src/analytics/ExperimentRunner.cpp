#include "analytics/ExperimentRunner.h"
#include "analytics/AnalyticsEngine.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <filesystem>

namespace SmartCity::analytics {

static void ConfigureDemand(sim::SimEngineConfig& cfg, const std::string& preset) {
    if (preset == "Low") {
        cfg.demand.northArrivalsPerHour = 360.0f;
        cfg.demand.southArrivalsPerHour = 360.0f;
        cfg.demand.eastArrivalsPerHour = 270.0f;
        cfg.demand.westArrivalsPerHour = 270.0f;
    } else if (preset == "High") {
        cfg.demand.northArrivalsPerHour = 1080.0f;
        cfg.demand.southArrivalsPerHour = 1080.0f;
        cfg.demand.eastArrivalsPerHour = 810.0f;
        cfg.demand.westArrivalsPerHour = 810.0f;
    } else { // "Medium" default
        cfg.demand.northArrivalsPerHour = 720.0f;
        cfg.demand.southArrivalsPerHour = 720.0f;
        cfg.demand.eastArrivalsPerHour = 540.0f;
        cfg.demand.westArrivalsPerHour = 540.0f;
    }
}

std::vector<ExperimentRunRecord> ExperimentRunner::RunBenchmark(const city::RoadNetwork& roadNetwork,
                                                               float durationSeconds,
                                                               const std::vector<uint32_t>& seeds,
                                                               const std::string& demandPreset) {
    std::vector<ExperimentRunRecord> results;
    std::vector<std::string> modes = { "FixedTime", "Adaptive" };

    const float dt = 1.0f / 60.0f;
    const int totalSteps = static_cast<int>(std::ceil(durationSeconds / dt));

    core::Logger::Info("Starting Experiment Benchmark: Duration=", durationSeconds, "s, Seeds=", seeds.size(), ", Demand=", demandPreset);

    for (const auto& modeStr : modes) {
        traffic::SignalMode mode = (modeStr == "Adaptive") ? traffic::SignalMode::Adaptive : traffic::SignalMode::FixedTime;

        for (uint32_t seed : seeds) {
            traffic::SignalConfig sigConfig;
            sigConfig.mode = mode;
            auto sigController = std::make_shared<traffic::TrafficSignalController>(sigConfig);

            ped::PedestrianConfig pedConfig;
            pedConfig.randomSeed = seed;
            ped::PedestrianManager pedManager(roadNetwork, sigController);
            pedManager.Reset(seed);

            auto vehManager = std::make_unique<sim::VehicleManager>(roadNetwork, sigController);
            vehManager->SetPedestrianManager(&pedManager);

            auto cfg = vehManager->GetConfig();
            ConfigureDemand(cfg, demandPreset);
            cfg.randomSeed = seed;
            vehManager->SetConfig(cfg);
            vehManager->Reset(seed);

            AnalyticsEngine analytics(*vehManager, *sigController, pedManager);

            int maxQueueOverall = 0;
            std::array<int, 4> maxQueuePerApproach = { 0, 0, 0, 0 };
            int signalSwitches = 0;
            traffic::SignalPhase prevPhase = sigController->GetCurrentPhase();

            for (int step = 0; step < totalSteps; ++step) {
                vehManager->Step(dt);
                pedManager.Step(dt);
                analytics.Step(dt);

                traffic::SignalPhase currentPhase = sigController->GetCurrentPhase();
                if (currentPhase != prevPhase) {
                    signalSwitches++;
                    prevPhase = currentPhase;
                }

                const auto& snap = analytics.GetSnapshot();
                int currentOverallQueue = 0;
                for (size_t a = 0; a < 4; ++a) {
                    currentOverallQueue += snap.queues[a].vehicleCount;
                    maxQueuePerApproach[a] = std::max(maxQueuePerApproach[a], snap.queues[a].vehicleCount);
                }
                maxQueueOverall = std::max(maxQueueOverall, currentOverallQueue);
            }

            const auto& finalSnap = analytics.GetSnapshot();

            ExperimentRunRecord record;
            record.mode = modeStr;
            record.seed = seed;
            record.durationSeconds = durationSeconds;
            record.demandLevel = demandPreset;
            record.throughput = finalSnap.totalCompleted;
            record.averageWaitingTime = finalSnap.tripWaitValid ? finalSnap.avgTripWaitingTime : finalSnap.avgLiveWaitingTime;
            record.maxQueueOverall = maxQueueOverall;
            record.maxQueuePerApproach = maxQueuePerApproach;
            record.signalSwitches = signalSwitches;
            record.safetyViolations = finalSnap.signalSafetyViolation ? 1 : 0;
            record.collisions = finalSnap.collisions;

            results.push_back(record);

            core::Logger::Info("Experiment Run Complete: Mode=", modeStr, ", Seed=", seed,
                               ", Throughput=", record.throughput, ", AvgWait=", record.averageWaitingTime, "s, Collisions=", record.collisions);
        }
    }

    return results;
}

bool ExperimentRunner::SaveToCsv(const std::vector<ExperimentRunRecord>& records,
                                 const std::string& filepath,
                                 const std::string& gitCommitHash) {
    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream out(filepath);
    if (!out) {
        core::Logger::Error("Failed to open CSV output file: ", filepath);
        return false;
    }

    out << "# SmartCity 3D — Experiment Benchmark Results\n";
    out << "# Git Commit: " << gitCommitHash << "\n";
    out << "# Generated on: Windows Desktop Engine\n";
    out << "Mode,Seed,DurationSec,DemandLevel,Throughput,AvgWaitSec,MaxQueueOverall,MaxQueueN,MaxQueueE,MaxQueueS,MaxQueueW,SignalSwitches,Violations,Collisions\n";

    for (const auto& r : records) {
        out << r.mode << ","
            << r.seed << ","
            << std::fixed << std::setprecision(1) << r.durationSeconds << ","
            << r.demandLevel << ","
            << r.throughput << ","
            << std::fixed << std::setprecision(2) << r.averageWaitingTime << ","
            << r.maxQueueOverall << ","
            << r.maxQueuePerApproach[0] << ","
            << r.maxQueuePerApproach[1] << ","
            << r.maxQueuePerApproach[2] << ","
            << r.maxQueuePerApproach[3] << ","
            << r.signalSwitches << ","
            << r.safetyViolations << ","
            << r.collisions << "\n";
    }

    core::Logger::Info("Saved experiment benchmark results to CSV: ", filepath);
    return true;
}

bool ExperimentRunner::SaveToJson(const std::vector<ExperimentRunRecord>& records,
                                  const std::string& filepath,
                                  const std::string& gitCommitHash) {
    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream out(filepath);
    if (!out) {
        core::Logger::Error("Failed to open JSON output file: ", filepath);
        return false;
    }

    out << "{\n";
    out << "  \"gitCommit\": \"" << gitCommitHash << "\",\n";
    out << "  \"records\": [\n";

    for (size_t i = 0; i < records.size(); ++i) {
        const auto& r = records[i];
        out << "    {\n";
        out << "      \"mode\": \"" << r.mode << "\",\n";
        out << "      \"seed\": " << r.seed << ",\n";
        out << "      \"durationSeconds\": " << r.durationSeconds << ",\n";
        out << "      \"demandLevel\": \"" << r.demandLevel << "\",\n";
        out << "      \"throughput\": " << r.throughput << ",\n";
        out << "      \"averageWaitingTime\": " << r.averageWaitingTime << ",\n";
        out << "      \"maxQueueOverall\": " << r.maxQueueOverall << ",\n";
        out << "      \"maxQueuePerApproach\": ["
            << r.maxQueuePerApproach[0] << ", "
            << r.maxQueuePerApproach[1] << ", "
            << r.maxQueuePerApproach[2] << ", "
            << r.maxQueuePerApproach[3] << "],\n";
        out << "      \"signalSwitches\": " << r.signalSwitches << ",\n";
        out << "      \"safetyViolations\": " << r.safetyViolations << ",\n";
        out << "      \"collisions\": " << r.collisions << "\n";
        out << "    }" << (i + 1 < records.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    core::Logger::Info("Saved experiment benchmark results to JSON: ", filepath);
    return true;
}

ExperimentSummary ExperimentRunner::ComputeSummary(const std::vector<ExperimentRunRecord>& records, const std::string& mode) {
    ExperimentSummary summary;
    summary.mode = mode;

    std::vector<float> throughs;
    std::vector<float> waits;
    std::vector<float> queues;
    std::vector<float> switches;

    for (const auto& r : records) {
        if (r.mode != mode) continue;
        summary.demandLevel = r.demandLevel;
        summary.runCount++;

        throughs.push_back(static_cast<float>(r.throughput));
        waits.push_back(r.averageWaitingTime);
        queues.push_back(static_cast<float>(r.maxQueueOverall));
        switches.push_back(static_cast<float>(r.signalSwitches));
        summary.totalSafetyViolations += r.safetyViolations;
        summary.totalCollisions += r.collisions;
    }

    if (summary.runCount == 0) return summary;

    auto calcStats = [](const std::vector<float>& data, float& outMean, float& outStd) {
        if (data.empty()) return;
        float sum = std::accumulate(data.begin(), data.end(), 0.0f);
        outMean = sum / static_cast<float>(data.size());
        float var = 0.0f;
        for (float val : data) {
            float diff = val - outMean;
            var += diff * diff;
        }
        outStd = (data.size() > 1) ? std::sqrt(var / static_cast<float>(data.size() - 1)) : 0.0f;
    };

    calcStats(throughs, summary.meanThroughput, summary.stdThroughput);
    calcStats(waits, summary.meanWaitingTime, summary.stdWaitingTime);
    calcStats(queues, summary.meanMaxQueue, summary.stdMaxQueue);
    calcStats(switches, summary.meanSignalSwitches, summary.stdThroughput); // mean switches

    return summary;
}

} // namespace SmartCity::analytics

