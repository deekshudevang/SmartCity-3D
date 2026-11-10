#include <iostream>
#include <cassert>
#include <cmath>
#include <thread>
#include "core/Time.h"
#include "core/Logger.h"

int main() {
    SmartCity::core::Logger::Init("logs/test_time.log");
    SmartCity::core::Logger::Info("Running unit test_time...");

    double fixedDt = 1.0 / 60.0;
    SmartCity::core::Time timer(fixedDt);

    assert(timer.GetTickCount() == 0);
    assert(timer.GetTotalSimTime() == 0.0);
    assert(!timer.IsPaused());
    assert(timer.GetTimeScale() == 1.0);
    SmartCity::core::Logger::Info("[PASS] Timer initial state");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    timer.Update();

    assert(timer.GetFrameDeltaTime() > 0.030);
    assert(timer.ShouldSimulateStep());

    int simulatedSteps = 0;
    while (timer.ShouldSimulateStep()) {
        timer.ConsumeSimulateStep();
        simulatedSteps++;
    }

    assert(simulatedSteps >= 2);
    assert(timer.GetTickCount() == static_cast<uint64_t>(simulatedSteps));
    assert(std::abs(timer.GetTotalSimTime() - (simulatedSteps * fixedDt)) < 1e-6);
    SmartCity::core::Logger::Info("[PASS] Fixed timestep accumulation (", simulatedSteps, " steps)");

    timer.SetPaused(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    timer.Update();
    assert(!timer.ShouldSimulateStep());
    SmartCity::core::Logger::Info("[PASS] Simulation pause behavior");

    double alpha = timer.GetInterpolationAlpha();
    assert(alpha >= 0.0 && alpha <= 1.0);
    SmartCity::core::Logger::Info("[PASS] Interpolation alpha within [0, 1] range: ", alpha);

    SmartCity::core::Logger::Info("All test_time unit tests PASSED successfully!");
    SmartCity::core::Logger::Shutdown();
    return 0;
}
