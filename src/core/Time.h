#pragma once

#include <chrono>

namespace SmartCity {

class Time {
public:
    Time(double fixedDeltaTime = 1.0 / 60.0);

    void Reset();
    void Update();

    // Fixed timestep query & consumption
    bool ShouldSimulateStep();
    void ConsumeSimulateStep();

    // Accessors
    double GetFrameDeltaTime() const { return m_frameDeltaTime; }
    double GetFixedDeltaTime() const { return m_fixedDeltaTime; }
    double GetTotalRealTime() const { return m_totalRealTime; }
    double GetTotalSimTime() const { return m_totalSimTime; }
    double GetInterpolationAlpha() const;
    uint64_t GetTickCount() const { return m_tickCount; }
    double GetFPS() const { return m_fps; }

    // Simulation speed scale (0.0 = paused, 1.0 = normal, 2.0 = 2x, etc.)
    void SetTimeScale(double scale) { m_timeScale = scale < 0.0 ? 0.0 : scale; }
    double GetTimeScale() const { return m_timeScale; }
    void SetPaused(bool paused) { m_isPaused = paused; }
    bool IsPaused() const { return m_isPaused; }

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint m_lastFrameTime;
    TimePoint m_startTime;

    double m_fixedDeltaTime;
    double m_frameDeltaTime;
    double m_accumulator;
    double m_maxDeltaTime;
    double m_totalRealTime;
    double m_totalSimTime;
    double m_timeScale;
    bool m_isPaused;
    uint64_t m_tickCount;

    // FPS calculation
    double m_fps;
    double m_fpsTimer;
    int m_frameCount;
};

} // namespace SmartCity
