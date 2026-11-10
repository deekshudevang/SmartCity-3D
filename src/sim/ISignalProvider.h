#pragma once

#include "city/RoadNetwork.h"

namespace SmartCity::sim {

enum class SignalAspect {
    Red,
    Yellow,
    Green
};

class ISignalProvider {
public:
    virtual ~ISignalProvider() = default;

    virtual void Update(float dt) {}
    virtual SignalAspect GetState(city::ApproachId approach, city::MovementType movement) const = 0;
};

class FixedCycleSignalProvider : public ISignalProvider {
public:
    FixedCycleSignalProvider(float greenTime = 24.0f, float yellowTime = 3.5f, float redClearance = 1.5f);
    virtual ~FixedCycleSignalProvider() = default;

    void Update(float dt) override;

    void SetPermanentRed(bool permanentRed) { m_permanentRed = permanentRed; }
    bool IsPermanentRed() const { return m_permanentRed; }

    void SetManualAspects(SignalAspect nsThrough, SignalAspect ewThrough,
                          SignalAspect nsLeft, SignalAspect ewLeft);

    SignalAspect GetState(city::ApproachId approach, city::MovementType movement) const override;

    int GetCurrentPhase() const { return m_currentPhase; }
    float GetPhaseElapsed() const { return m_phaseTimer; }

private:
    float m_greenTime;
    float m_yellowTime;
    float m_redClearance;

    float m_phaseTimer;
    int m_currentPhase; // 0: NS Green, 1: NS Yellow, 2: All Red, 3: EW Green, 4: EW Yellow, 5: All Red

    bool m_permanentRed;
    bool m_manualOverride;

    SignalAspect m_manualNSThrough;
    SignalAspect m_manualEWThrough;
    SignalAspect m_manualNSLeft;
    SignalAspect m_manualEWLeft;
};

} // namespace SmartCity::sim
