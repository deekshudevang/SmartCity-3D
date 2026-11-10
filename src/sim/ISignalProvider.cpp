#include "sim/ISignalProvider.h"

namespace SmartCity::sim {

FixedCycleSignalProvider::FixedCycleSignalProvider(float greenTime, float yellowTime, float redClearance)
    : m_greenTime(greenTime)
    , m_yellowTime(yellowTime)
    , m_redClearance(redClearance)
    , m_phaseTimer(0.0f)
    , m_currentPhase(0)
    , m_permanentRed(false)
    , m_manualOverride(false)
    , m_manualNSThrough(SignalAspect::Green)
    , m_manualEWThrough(SignalAspect::Red)
    , m_manualNSLeft(SignalAspect::Red)
    , m_manualEWLeft(SignalAspect::Red)
{
}

void FixedCycleSignalProvider::Update(float dt) {
    if (m_permanentRed || m_manualOverride) return;

    m_phaseTimer += dt;
    float currentDuration = 0.0f;

    switch (m_currentPhase) {
        case 0: // NS Green
        case 3: // EW Green
            currentDuration = m_greenTime;
            break;
        case 1: // NS Yellow
        case 4: // EW Yellow
            currentDuration = m_yellowTime;
            break;
        case 2: // All Red after NS
        case 5: // All Red after EW
            currentDuration = m_redClearance;
            break;
    }

    if (m_phaseTimer >= currentDuration) {
        m_phaseTimer = 0.0f;
        m_currentPhase = (m_currentPhase + 1) % 6;
    }
}

void FixedCycleSignalProvider::SetManualAspects(SignalAspect nsThrough, SignalAspect ewThrough,
                                                SignalAspect nsLeft, SignalAspect ewLeft)
{
    m_manualOverride = true;
    m_manualNSThrough = nsThrough;
    m_manualEWThrough = ewThrough;
    m_manualNSLeft = nsLeft;
    m_manualEWLeft = ewLeft;
}

SignalAspect FixedCycleSignalProvider::GetState(city::ApproachId approach, city::MovementType movement) const {
    if (m_permanentRed) {
        return SignalAspect::Red;
    }

    if (m_manualOverride) {
        if (approach == city::ApproachId::North || approach == city::ApproachId::South) {
            return (movement == city::MovementType::Left) ? m_manualNSLeft : m_manualNSThrough;
        } else {
            return (movement == city::MovementType::Left) ? m_manualEWLeft : m_manualEWThrough;
        }
    }

    // Right-turn-on-red under RHT: can proceed on green or yellow, yields on red
    bool isNS = (approach == city::ApproachId::North || approach == city::ApproachId::South);

    switch (m_currentPhase) {
        case 0: // NS Green
            if (isNS) {
                return (movement == city::MovementType::Left) ? SignalAspect::Yellow : SignalAspect::Green;
            }
            return SignalAspect::Red;

        case 1: // NS Yellow
            if (isNS) return SignalAspect::Yellow;
            return SignalAspect::Red;

        case 2: // All Red
            return SignalAspect::Red;

        case 3: // EW Green
            if (!isNS) {
                return (movement == city::MovementType::Left) ? SignalAspect::Yellow : SignalAspect::Green;
            }
            return SignalAspect::Red;

        case 4: // EW Yellow
            if (!isNS) return SignalAspect::Yellow;
            return SignalAspect::Red;

        case 5: // All Red
            return SignalAspect::Red;
    }

    return SignalAspect::Red;
}

} // namespace SmartCity::sim
