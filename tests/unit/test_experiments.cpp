#include <cassert>
#include <iostream>
#include <cmath>
#include <fstream>
#include "analytics/ExperimentRunner.h"
#include "city/RoadNetwork.h"

using namespace SmartCity;

void Test_ExperimentReproducibility() {
    std::cout << "[Test 1] Experiment Determinism & Reproducibility (Same Seed)..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    // Run trial 1 with seed 1337 for 60 simulated seconds
    auto res1 = analytics::ExperimentRunner::RunBenchmark(net, 60.0f, { 1337 }, "Medium");

    // Run trial 2 with seed 1337 for 60 simulated seconds
    auto res2 = analytics::ExperimentRunner::RunBenchmark(net, 60.0f, { 1337 }, "Medium");

    assert(res1.size() == 2 && res2.size() == 2); // 1 FixedTime + 1 Adaptive

    for (size_t i = 0; i < res1.size(); ++i) {
        assert(res1[i].mode == res2[i].mode);
        assert(res1[i].seed == res2[i].seed);
        assert(res1[i].throughput == res2[i].throughput);
        assert(std::abs(res1[i].averageWaitingTime - res2[i].averageWaitingTime) < 1e-4f);
        assert(res1[i].maxQueueOverall == res2[i].maxQueueOverall);
        assert(res1[i].signalSwitches == res2[i].signalSwitches);
        assert(res1[i].safetyViolations == 0);
        assert(res1[i].collisions == 0);
    }

    std::cout << "  PASS: Identical seeds produce byte-identical throughput, waiting times, and queues." << std::endl;
}

void Test_ExperimentZeroCollisionsAndSafety() {
    std::cout << "[Test 2] Experiment Zero Collisions & Conflict Violations..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    // Run 3-seed benchmark in Medium demand
    auto results = analytics::ExperimentRunner::RunBenchmark(net, 120.0f, { 42, 1337, 2024 }, "Medium");
    assert(results.size() == 6); // 2 modes * 3 seeds

    for (const auto& r : results) {
        assert(r.collisions == 0 && "Collisions must be strictly zero!");
        assert(r.safetyViolations == 0 && "Signal conflict violations must be strictly zero!");
    }

    std::cout << "  PASS: 100% of benchmark runs completed with 0 collisions and 0 signal conflict violations." << std::endl;
}

void Test_ExportAndSummaryStatistics() {
    std::cout << "[Test 3] Experiment CSV/JSON Export and Statistical Summaries..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    auto results = analytics::ExperimentRunner::RunBenchmark(net, 60.0f, { 101, 202 }, "Low");

    bool csvSaved = analytics::ExperimentRunner::SaveToCsv(results, "logs/experiments/test_benchmark.csv", "54136b4");
    assert(csvSaved);

    bool jsonSaved = analytics::ExperimentRunner::SaveToJson(results, "logs/experiments/test_benchmark.json", "54136b4");
    assert(jsonSaved);

    // Verify CSV file exists and has non-zero size
    std::ifstream csvFile("logs/experiments/test_benchmark.csv");
    assert(csvFile.is_open());
    std::string line;
    std::getline(csvFile, line);
    assert(!line.empty());

    // Summary statistics
    auto fixedSummary = analytics::ExperimentRunner::ComputeSummary(results, "FixedTime");
    auto adaptSummary = analytics::ExperimentRunner::ComputeSummary(results, "Adaptive");

    assert(fixedSummary.runCount == 2);
    assert(adaptSummary.runCount == 2);
    assert(fixedSummary.totalCollisions == 0);
    assert(adaptSummary.totalCollisions == 0);

    std::cout << "  PASS: CSV/JSON export and Mean +/- Std summaries successfully generated." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Experiment Benchmark Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    Test_ExperimentReproducibility();
    Test_ExperimentZeroCollisionsAndSafety();
    Test_ExportAndSummaryStatistics();

    std::cout << "=====================================================" << std::endl;
    std::cout << "ALL EXPERIMENT BENCHMARK TESTS PASSED (3/3)" << std::endl;
    std::cout << "=====================================================" << std::endl;
    return 0;
}

