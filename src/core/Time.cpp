#include "core/Time.h"
#include <algorithm>

namespace SmartCity::core {

Time::Time(double fixedDeltaTime)
    : m_fixedDeltaTime(fixedDeltaTime)
    , m_frameDeltaTime(0.0)
    , m_accumulator(0.0)
    , m_maxDeltaTime(0.25)
    , m_totalRealTime(0.0)
    , m_totalSimTime(0.0)
    , m_timeScale(1.0)
    , m_isPaused(false)
    , m_tickCount(0)
    , m_fps(60.0)
    , m_fpsTimer(0.0)
    , m_frameCount(0)
{
    Reset();
}

void Time::Reset() {
    m_startTime = Clock::now();
    m_lastFrameTime = m_startTime;
    m_frameDeltaTime = 0.0;
    m_accumulator = 0.0;
    m_totalRealTime = 0.0;
    m_totalSimTime = 0.0;
    m_tickCount = 0;
    m_fps = 60.0;
    m_fpsTimer = 0.0;
    m_frameCount = 0;
}

void Time::Update() {
    TimePoint currentTime = Clock::now();
    std::chrono::duration<double> elapsed = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    double dt = elapsed.count();
    if (dt < 0.0) dt = 0.0;
    if (dt > m_maxDeltaTime) dt = m_maxDeltaTime;

    m_frameDeltaTime = dt;
    m_totalRealTime += dt;

    if (!m_isPaused) {
        m_accumulator += dt * m_timeScale;
    }

    m_frameCount++;
    m_fpsTimer += dt;
    if (m_fpsTimer >= 0.5) {
        m_fps = static_cast<double>(m_frameCount) / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer = 0.0;
    }
}

bool Time::ShouldSimulateStep() {
    return m_accumulator >= m_fixedDeltaTime;
}

void Time::ConsumeSimulateStep() {
    m_accumulator -= m_fixedDeltaTime;
    m_totalSimTime += m_fixedDeltaTime;
    m_tickCount++;
}

double Time::GetInterpolationAlpha() const {
    if (m_fixedDeltaTime <= 0.0) return 1.0;
    double alpha = m_accumulator / m_fixedDeltaTime;
    return std::clamp(alpha, 0.0, 1.0);
}

} // namespace SmartCity::core
