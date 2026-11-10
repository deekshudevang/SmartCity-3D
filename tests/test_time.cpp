#include <iostream>
#include <cassert>
#include <cmath>
#include <thread>
#include "core/Time.h"
#include "core/Logger.h"

int main() {
    SmartCity::Logger::Info("Running test_time...");

    double fixedDt = 1.0 / 60.0; // 0.016666...
    SmartCity::Time timer(fixedDt);

    // Initial state check
    assert(timer.GetTickCount() == 0);
    assert(timer.GetTotalSimTime() == 0.0);
    assert(!timer.IsPaused());
    assert(timer.GetTimeScale() == 1.0);
    SmartCity::Logger::Info("[PASS] Timer initial state");

    // Simulate elapsed time by sleeping a known short duration or running iterations
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    timer.Update();

    assert(timer.GetFrameDeltaTime() > 0.030); // At least 30ms elapsed
    assert(timer.ShouldSimulateStep());

    int simulatedSteps = 0;
    while (timer.ShouldSimulateStep()) {
        timer.ConsumeSimulateStep();
        simulatedSteps++;
    }

    assert(simulatedSteps >= 2); // 50ms should produce 2 or 3 16.6ms steps
    assert(timer.GetTickCount() == static_cast<uint64_t>(simulatedSteps));
    assert(std::abs(timer.GetTotalSimTime() - (simulatedSteps * fixedDt)) < 1e-6);
    SmartCity::Logger::Info("[PASS] Fixed timestep accumulation (", simulatedSteps, " steps)");

    // Test Paused state
    timer.SetPaused(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    timer.Update();
    assert(!timer.ShouldSimulateStep()); // No accumulator increase when paused
    SmartCity::Logger::Info("[PASS] Simulation pause behavior");

    // Test Interpolation Alpha bounds
    double alpha = timer.GetInterpolationAlpha();
    assert(alpha >= 0.0 && alpha <= 1.0);
    SmartCity::Logger::Info("[PASS] Interpolation alpha within [0, 1] range: ", alpha);

    SmartCity::Logger::Info("All test_time tests PASSED successfully!");
    return 0;
}
