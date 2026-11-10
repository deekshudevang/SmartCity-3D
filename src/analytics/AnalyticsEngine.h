#pragma once

#include <vector>
#include <array>
#include <string>
#include <deque>
#include <memory>
#include "city/RoadNetwork.h"
#include "sim/VehicleManager.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianManager.h"

namespace SmartCity::analytics {

template<typename T, size_t Capacity = 300>
class RingBuffer {
public:
    void Push(const T& value) {
        if (m_data.size() < Capacity) {
            m_data.push_back(value);
        } else {
            m_data[m_head] = value;
            m_head = (m_head + 1) % Capacity;
        }
    }

    void Clear() {
        m_data.clear();
        m_head = 0;
    }

    size_t Size() const { return m_data.size(); }
    bool Empty() const { return m_data.empty(); }

    // Returns linear ordered array from oldest to newest for ImGui::PlotLines
    std::vector<T> ToOrderedVector() const {
        std::vector<T> result;
        result.reserve(m_data.size());
        if (m_data.size() < Capacity) {
            result = m_data;
        } else {
            for (size_t i = 0; i < Capacity; ++i) {
                result.push_back(m_data[(m_head + i) % Capacity]);
            }
        }
        return result;
    }

private:
    std::vector<T> m_data;
    size_t m_head{ 0 };
};

struct ApproachQueueMetrics {
    int vehicleCount{ 0 };
    float lengthMetres{ 0.0f };
};

struct AnalyticsSnapshot {
    float simTime{ 0.0f };

    // 1. Vehicles
    int activeVehicles{ 0 };
    int waitingVehicles{ 0 };
    int totalSpawned{ 0 };
    int totalCompleted{ 0 };
    int collisions{ 0 };

    // 2. Approach Queues (Index: 0=North, 1=East, 2=South, 3=West)
    std::array<ApproachQueueMetrics, 4> queues;

    // 3. Speeds
    bool speedValid{ false };
    float avgSpeedMs{ 0.0f };
    float avgSpeedKmh{ 0.0f };
    float rollingAvgSpeedKmh{ 0.0f };

    // 4. Waiting Times
    bool liveWaitValid{ false };
    float avgLiveWaitingTime{ 0.0f };
    bool tripWaitValid{ false };
    float avgTripWaitingTime{ 0.0f };

    // 5. Throughput
    bool rollingThroughputValid{ false };
    float rollingThroughputPerHour{ 0.0f };
    bool cumulativeThroughputValid{ false };
    float cumulativeThroughputPerHour{ 0.0f };

    // 6. Pedestrians
    int activePedestrians{ 0 };
    int completedPedestrians{ 0 };
    int pedestrianViolations{ 0 };

    // 7. Signals
    std::string signalPhaseName;
    float signalPhaseElapsed{ 0.0f };
    float signalPhaseDuration{ 0.0f };
    float signalPhaseRemaining{ 0.0f };
    bool signalSafetyViolation{ false };

    // 8. Congestion & Level of Service
    float congestionIndex{ 0.0f }; // [0..1]
    std::string congestionLevel;   // "Low", "Medium", "High", "Severe"

    // 9. Environmental Emissions
    float instantaneousCO2RateGramsPerSec{ 0.0f };
    float cumulativeCO2Kg{ 0.0f };
    float cumulativeFuelLiters{ 0.0f };
};

class AnalyticsEngine {
public:
    AnalyticsEngine(sim::VehicleManager& vehicleManager,
                    traffic::TrafficSignalController& signalController,
                    ped::PedestrianManager& pedestrianManager);

    void Reset();

    // Sample simulation state at fixed dt boundaries (called by sim loop)
    void Step(float dt);

    const AnalyticsSnapshot& GetSnapshot() const { return m_snapshot; }

    // Ring buffer histories for Dear ImGui plots
    std::vector<float> GetSpeedHistory() const { return m_speedHistory.ToOrderedVector(); }
    std::vector<float> GetThroughputHistory() const { return m_throughputHistory.ToOrderedVector(); }
    std::vector<float> GetQueueHistory(city::ApproachId approach) const {
        return m_queueHistories[static_cast<size_t>(approach)].ToOrderedVector();
    }
    std::vector<float> GetCO2History() const { return m_co2History.ToOrderedVector(); }

    // Manual test script injection helpers
    void RegisterTripCompletion(float tripWaitingTime);

private:
    void SampleVehiclesAndQueues();
    void SampleSpeeds(float dt);
    void SampleThroughput();
    void SampleEmissions(float dt);
    void SampleSignalsAndPedestrians();
    void CalculateCongestion();

    sim::VehicleManager& m_vehicleManager;
    traffic::TrafficSignalController& m_signalController;
    ped::PedestrianManager& m_pedestrianManager;

    AnalyticsSnapshot m_snapshot;

    // Rolling windows
    std::deque<std::pair<float, float>> m_speedTimeWindow; // (simTime, avgSpeed)
    std::deque<float> m_exitTimestamps;                    // timestamps of vehicles completing trip

    // Trip waiting time accumulators
    float m_totalCompletedTripWaitTime{ 0.0f };
    int m_totalCompletedTripsCount{ 0 };
    int m_lastKnownCompletedCount{ 0 };

    // Emissions accumulators
    float m_cumulativeFuelMilliliters{ 0.0f };

    // 1 Hz plot sampling timer
    float m_plotSampleTimer{ 0.0f };

    RingBuffer<float, 300> m_speedHistory;
    RingBuffer<float, 300> m_throughputHistory;
    std::array<RingBuffer<float, 300>, 4> m_queueHistories;
    RingBuffer<float, 300> m_co2History;
};

} // namespace SmartCity::analytics

