#include "analytics/AnalyticsEngine.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace SmartCity::analytics {

AnalyticsEngine::AnalyticsEngine(sim::VehicleManager& vehicleManager,
                                 traffic::TrafficSignalController& signalController,
                                 ped::PedestrianManager& pedestrianManager)
    : m_vehicleManager(vehicleManager)
    , m_signalController(signalController)
    , m_pedestrianManager(pedestrianManager)
{
    m_vehicleManager.SetVehicleCompletedCallback([this](const sim::Vehicle& v) {
        RegisterTripCompletion(v.GetWaitingTime());
    });

    Reset();
}

void AnalyticsEngine::Reset() {
    m_snapshot = AnalyticsSnapshot{};
    m_speedTimeWindow.clear();
    m_exitTimestamps.clear();

    m_totalCompletedTripWaitTime = 0.0f;
    m_totalCompletedTripsCount = 0;
    m_lastKnownCompletedCount = 0;
    m_cumulativeFuelMilliliters = 0.0f;
    m_plotSampleTimer = 0.0f;

    m_speedHistory.Clear();
    m_throughputHistory.Clear();
    m_co2History.Clear();
    for (auto& qHist : m_queueHistories) {
        qHist.Clear();
    }
}

void AnalyticsEngine::RegisterTripCompletion(float tripWaitingTime) {
    m_exitTimestamps.push_back(m_snapshot.simTime);
    m_totalCompletedTripWaitTime += tripWaitingTime;
    m_totalCompletedTripsCount++;
}

void AnalyticsEngine::Step(float dt) {
    if (m_vehicleManager.IsPaused()) {
        return;
    }

    m_snapshot.simTime = m_vehicleManager.GetSimTime();
    m_snapshot.collisions = m_vehicleManager.GetCollisionCount();

    SampleVehiclesAndQueues();
    SampleSpeeds(dt);
    SampleThroughput();
    SampleEmissions(dt);
    SampleSignalsAndPedestrians();
    CalculateCongestion();

    // 1 Hz plot history aggregation
    m_plotSampleTimer += dt;
    if (m_plotSampleTimer >= 1.0f) {
        m_plotSampleTimer = 0.0f;

        m_speedHistory.Push(m_snapshot.speedValid ? m_snapshot.avgSpeedKmh : 0.0f);
        m_throughputHistory.Push(m_snapshot.rollingThroughputValid ? m_snapshot.rollingThroughputPerHour : 0.0f);
        m_co2History.Push(m_snapshot.instantaneousCO2RateGramsPerSec);

        for (size_t a = 0; a < 4; ++a) {
            m_queueHistories[a].Push(static_cast<float>(m_snapshot.queues[a].vehicleCount));
        }
    }
}

void AnalyticsEngine::SampleVehiclesAndQueues() {
    const auto& vehicles = m_vehicleManager.GetActiveVehicles();
    m_snapshot.activeVehicles = static_cast<int>(vehicles.size());
    m_snapshot.totalSpawned = m_vehicleManager.GetTotalSpawned();
    m_snapshot.totalCompleted = m_vehicleManager.GetTotalCompleted();

    // Reset per-approach queue metrics
    for (auto& q : m_snapshot.queues) {
        q.vehicleCount = 0;
        q.lengthMetres = 0.0f;
    }

    int waitingCount = 0;
    float liveWaitingSum = 0.0f;

    // Approach candidate queues: vehicles on inbound lanes
    std::array<std::vector<const sim::Vehicle*>, 4> approachVehicles;

    for (const auto& vPtr : vehicles) {
        if (!vPtr || !vPtr->IsActive()) continue;
        const auto& v = *vPtr;
        float speed = v.GetSpeed();
        float s = v.GetDistanceAlongRoute();
        float stopDist = v.GetRoute().GetStopLineDistance();

        // 1. Waiting Vehicle Rule: speed < 0.10 m/s in approach zone [s_stop - 80m, s_stop + 2m]
        if (speed < 0.10f && s >= (stopDist - 80.0f) && s <= (stopDist + 2.0f)) {
            waitingCount++;
            liveWaitingSum += v.GetWaitingTime();
        }

        // 2. Queue candidate: on inbound approach prior to stop line and crawling/stopped (speed <= 0.50 m/s)
        if (s <= (stopDist + 1.5f) && speed <= 0.50f) {
            size_t appIdx = static_cast<size_t>(v.GetRoute().approachId);
            if (appIdx < 4) {
                approachVehicles[appIdx].push_back(&v);
            }
        }
    }

    m_snapshot.waitingVehicles = waitingCount;
    if (waitingCount > 0) {
        m_snapshot.liveWaitValid = true;
        m_snapshot.avgLiveWaitingTime = liveWaitingSum / static_cast<float>(waitingCount);
    } else {
        m_snapshot.liveWaitValid = false;
        m_snapshot.avgLiveWaitingTime = 0.0f;
    }

    // Evaluate contiguous queues per approach
    for (size_t a = 0; a < 4; ++a) {
        auto& list = approachVehicles[a];
        if (list.empty()) continue;

        // Sort descending by distance along route (closest to stop line first)
        std::sort(list.begin(), list.end(), [](const sim::Vehicle* lhs, const sim::Vehicle* rhs) {
            return lhs->GetDistanceAlongRoute() > rhs->GetDistanceAlongRoute();
        });

        int queueCount = 0;
        float minS = 1e6f;
        float prevS = -1.0f;

        for (const auto* v : list) {
            float s = v->GetDistanceAlongRoute();
            float stopDist = v->GetRoute().GetStopLineDistance();

            if (queueCount == 0) {
                // Lead vehicle must be within 15m of stop line
                if ((stopDist - s) <= 15.0f) {
                    queueCount++;
                    minS = s;
                    prevS = s;
                }
            } else {
                // Subsequent vehicle must be within 8.5m of the vehicle ahead
                float spacing = prevS - s;
                if (spacing >= 0.0f && spacing <= 8.5f) {
                    queueCount++;
                    minS = std::min(minS, s);
                    prevS = s;
                } else {
                    break; // Contiguity broken
                }
            }
        }

        m_snapshot.queues[a].vehicleCount = queueCount;
        if (queueCount > 0 && !list.empty()) {
            float stopDist = list.front()->GetRoute().GetStopLineDistance();
            m_snapshot.queues[a].lengthMetres = std::max(0.0f, stopDist - minS + 2.5f); // + rear bumper half-length
        } else {
            m_snapshot.queues[a].lengthMetres = 0.0f;
        }
    }

    // Trip Waiting Time
    if (m_totalCompletedTripsCount > 0) {
        m_snapshot.tripWaitValid = true;
        m_snapshot.avgTripWaitingTime = m_totalCompletedTripWaitTime / static_cast<float>(m_totalCompletedTripsCount);
    } else {
        m_snapshot.tripWaitValid = false;
        m_snapshot.avgTripWaitingTime = 0.0f;
    }
}

void AnalyticsEngine::SampleSpeeds(float /*dt*/) {
    const auto& vehicles = m_vehicleManager.GetActiveVehicles();
    if (vehicles.empty()) {
        m_snapshot.speedValid = false;
        m_snapshot.avgSpeedMs = 0.0f;
        m_snapshot.avgSpeedKmh = 0.0f;
        m_snapshot.rollingAvgSpeedKmh = 0.0f;
        return;
    }

    float speedSum = 0.0f;
    for (const auto& v : vehicles) {
        speedSum += v->GetSpeed();
    }

    m_snapshot.speedValid = true;
    m_snapshot.avgSpeedMs = speedSum / static_cast<float>(vehicles.size());
    m_snapshot.avgSpeedKmh = m_snapshot.avgSpeedMs * 3.6f;

    // 60-second rolling window
    m_speedTimeWindow.push_back({ m_snapshot.simTime, m_snapshot.avgSpeedKmh });
    while (!m_speedTimeWindow.empty() && (m_snapshot.simTime - m_speedTimeWindow.front().first) > 60.0f) {
        m_speedTimeWindow.pop_front();
    }

    float windowSum = 0.0f;
    for (const auto& item : m_speedTimeWindow) {
        windowSum += item.second;
    }
    m_snapshot.rollingAvgSpeedKmh = windowSum / static_cast<float>(m_speedTimeWindow.size());
}

void AnalyticsEngine::SampleThroughput() {
    float simTime = m_snapshot.simTime;

    // Prune exit timestamps older than 60 seconds
    while (!m_exitTimestamps.empty() && (simTime - m_exitTimestamps.front()) > 60.0f) {
        m_exitTimestamps.pop_front();
    }

    if (simTime >= 5.0f) {
        // Rolling 60s throughput
        m_snapshot.rollingThroughputValid = true;
        m_snapshot.rollingThroughputPerHour = (static_cast<float>(m_exitTimestamps.size()) / 60.0f) * 3600.0f;

        // Cumulative session throughput
        m_snapshot.cumulativeThroughputValid = true;
        m_snapshot.cumulativeThroughputPerHour = (static_cast<float>(m_snapshot.totalCompleted) / simTime) * 3600.0f;
    } else {
        m_snapshot.rollingThroughputValid = false;
        m_snapshot.rollingThroughputPerHour = 0.0f;
        m_snapshot.cumulativeThroughputValid = false;
        m_snapshot.cumulativeThroughputPerHour = 0.0f;
    }
}

void AnalyticsEngine::SampleEmissions(float dt) {
    const auto& vehicles = m_vehicleManager.GetActiveVehicles();
    float totalFuelRateMlPerSec = 0.0f;

    for (const auto& vPtr : vehicles) {
        if (!vPtr || !vPtr->IsActive()) continue;
        const auto& v = *vPtr;
        float speed = v.GetSpeed();
        float accel = v.GetAcceleration();

        float mass = 1450.0f;
        float area = 2.2f;
        float cd = 0.31f;
        float cr = 0.012f;
        float idleRate = 0.28f; // mL/s

        if (v.GetType() == sim::VehicleType::Bus) {
            mass = 11000.0f;
            area = 7.5f;
            cd = 0.65f;
            cr = 0.015f;
            idleRate = 1.10f;
        } else if (v.GetType() == sim::VehicleType::Emergency) {
            mass = 3200.0f;
            area = 3.8f;
            cd = 0.45f;
            cr = 0.014f;
            idleRate = 0.45f;
        }

        // Tractive power demand
        float g = 9.81f;
        float rho = 1.225f;
        float pInertia = mass * speed * accel;
        float pRoll = mass * g * cr * speed;
        float pAero = 0.5f * rho * cd * area * speed * speed * speed;
        float pTractiveWatts = std::max(0.0f, pInertia + pRoll + pAero);

        // Fuel rate in mL/s
        float pTractiveKW = pTractiveWatts * 0.001f;
        float fuelRate = idleRate + 0.065f * pTractiveKW;
        totalFuelRateMlPerSec += fuelRate;
    }

    // CO2 conversion: 2.31 g per mL of fuel
    m_snapshot.instantaneousCO2RateGramsPerSec = totalFuelRateMlPerSec * 2.31f;

    m_cumulativeFuelMilliliters += totalFuelRateMlPerSec * dt;
    m_snapshot.cumulativeFuelLiters = m_cumulativeFuelMilliliters * 0.001f;
    m_snapshot.cumulativeCO2Kg = (m_cumulativeFuelMilliliters * 2.31f) * 0.001f;
}

void AnalyticsEngine::SampleSignalsAndPedestrians() {
    // Pedestrian statistics
    m_snapshot.activePedestrians = static_cast<int>(m_pedestrianManager.GetActiveCount());
    m_snapshot.completedPedestrians = m_pedestrianManager.GetTotalCompleted();
    m_snapshot.pedestrianViolations = m_pedestrianManager.GetIllegalCrossingStarts();

    // Signal state
    m_snapshot.signalPhaseName = traffic::ToString(m_signalController.GetCurrentPhase());
    m_snapshot.signalPhaseElapsed = m_signalController.GetPhaseElapsed();
    m_snapshot.signalPhaseDuration = m_signalController.GetPhaseDuration();
    m_snapshot.signalPhaseRemaining = m_signalController.GetPhaseRemaining();
    m_snapshot.signalSafetyViolation = m_signalController.HasConflictViolation();
}

void AnalyticsEngine::CalculateCongestion() {
    float totalQueueM = 0.0f;
    for (const auto& q : m_snapshot.queues) {
        totalQueueM += q.lengthMetres;
    }

    // Total intersection capacity = 4 approaches * 120m = 480m
    m_snapshot.congestionIndex = std::clamp(totalQueueM / 480.0f, 0.0f, 1.0f);

    if (m_snapshot.congestionIndex < 0.15f) {
        m_snapshot.congestionLevel = "Low (Free Flow)";
    } else if (m_snapshot.congestionIndex < 0.40f) {
        m_snapshot.congestionLevel = "Medium (Stable Flow)";
    } else if (m_snapshot.congestionIndex < 0.70f) {
        m_snapshot.congestionLevel = "High (Approaching Capacity)";
    } else {
        m_snapshot.congestionLevel = "Severe (Gridlock Hazard)";
    }
}

} // namespace SmartCity::analytics
