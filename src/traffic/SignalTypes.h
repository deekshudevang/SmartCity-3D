#pragma once

#include <string>

namespace SmartCity::traffic {

enum class SignalPhase {
    NS_GREEN,
    NS_YELLOW,
    ALL_RED_1,
    EW_GREEN,
    EW_YELLOW,
    ALL_RED_2
};

enum class PedSignalAspect {
    Walk,
    FlashingDontWalk,
    DontWalk
};

enum class SignalMode {
    FixedTime,
    Adaptive
};

enum class TransitionTrigger {
    Timer,
    Adaptive,
    Emergency,
    Pedestrian,
    Manual
};

inline const char* ToString(SignalPhase phase) {
    switch (phase) {
        case SignalPhase::NS_GREEN:  return "NS_GREEN";
        case SignalPhase::NS_YELLOW: return "NS_YELLOW";
        case SignalPhase::ALL_RED_1: return "ALL_RED_1";
        case SignalPhase::EW_GREEN:  return "EW_GREEN";
        case SignalPhase::EW_YELLOW: return "EW_YELLOW";
        case SignalPhase::ALL_RED_2: return "ALL_RED_2";
    }
    return "UNKNOWN";
}

inline const char* ToString(PedSignalAspect aspect) {
    switch (aspect) {
        case PedSignalAspect::Walk:             return "WALK";
        case PedSignalAspect::FlashingDontWalk: return "FLASHING_DONT_WALK";
        case PedSignalAspect::DontWalk:         return "DONT_WALK";
    }
    return "UNKNOWN";
}

inline const char* ToString(TransitionTrigger trigger) {
    switch (trigger) {
        case TransitionTrigger::Timer:      return "Timer";
        case TransitionTrigger::Adaptive:   return "Adaptive";
        case TransitionTrigger::Emergency:  return "Emergency";
        case TransitionTrigger::Pedestrian: return "Pedestrian";
        case TransitionTrigger::Manual:     return "Manual";
    }
    return "UNKNOWN";
}

struct SignalTransitionRecord {
    float timestamp{ 0.0f };
    SignalPhase prevPhase{ SignalPhase::NS_GREEN };
    SignalPhase newPhase{ SignalPhase::NS_YELLOW };
    TransitionTrigger trigger{ TransitionTrigger::Timer };
    int nsQueue{ 0 };
    int ewQueue{ 0 };
    std::string reason;
};

} // namespace SmartCity::traffic

