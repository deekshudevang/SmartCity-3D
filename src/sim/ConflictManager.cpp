#include "sim/ConflictManager.h"
#include <algorithm>

namespace SmartCity::sim {

const std::vector<std::string> ConflictManager::s_emptyZoneList{};

ConflictManager::ConflictManager(const city::RoadNetwork& roadNetwork)
    : m_roadNetwork(roadNetwork)
{
    Reset();
}

void ConflictManager::Reset() {
    m_reservations.clear();
    m_pathToZones.clear();

    // 1. Register configured conflict zones from RoadNetwork
    const auto& zones = m_roadNetwork.GetAllConflictZones();
    for (const auto& zone : zones) {
        for (const auto& pathId : zone.conflictingPathIds) {
            m_pathToZones[pathId].push_back(zone.id);
        }
    }

    // 2. Automatically register geometric crossing and proximity conflicts
    // Prevents side-swipes and simultaneous occupancy of overlapping paths
    const auto& paths = m_roadNetwork.GetAllPaths();
    for (size_t i = 0; i < paths.size(); ++i) {
        for (size_t j = i + 1; j < paths.size(); ++j) {
            if (paths[i].fromLaneId == paths[j].fromLaneId) continue;

            float minDistSq = 1e9f;
            for (int s1 = 0; s1 <= 20; ++s1) {
                float t1 = static_cast<float>(s1) / 20.0f;
                glm::vec3 pos1 = paths[i].Evaluate(t1);

                for (int s2 = 0; s2 <= 20; ++s2) {
                    float t2 = static_cast<float>(s2) / 20.0f;
                    glm::vec3 pos2 = paths[j].Evaluate(t2);

                    float dx = pos1.x - pos2.x;
                    float dz = pos1.z - pos2.z;
                    float d2 = dx * dx + dz * dz;
                    if (d2 < minDistSq) {
                        minDistSq = d2;
                    }
                }
            }

            if (minDistSq < (4.5f * 4.5f)) {
                std::string autoZoneId = "AUTO_CZ_" + paths[i].id + "_X_" + paths[j].id;
                auto& listI = m_pathToZones[paths[i].id];
                if (std::find(listI.begin(), listI.end(), autoZoneId) == listI.end()) {
                    listI.push_back(autoZoneId);
                }
                auto& listJ = m_pathToZones[paths[j].id];
                if (std::find(listJ.begin(), listJ.end(), autoZoneId) == listJ.end()) {
                    listJ.push_back(autoZoneId);
                }
            }
        }
    }
}

const std::vector<std::string>& ConflictManager::GetZonesForPath(const std::string& pathId) const {
    auto it = m_pathToZones.find(pathId);
    if (it != m_pathToZones.end()) {
        return it->second;
    }
    return s_emptyZoneList;
}

bool ConflictManager::IsZoneReserved(const std::string& zoneId) const {
    return m_reservations.find(zoneId) != m_reservations.end();
}

int ConflictManager::GetReservingVehicle(const std::string& zoneId) const {
    auto it = m_reservations.find(zoneId);
    if (it != m_reservations.end()) {
        return it->second.vehicleId;
    }
    return -1;
}

bool ConflictManager::HasConflictingReservation(int vehicleId, const std::string& pathId) const {
    const auto& zones = GetZonesForPath(pathId);
    for (const auto& zoneId : zones) {
        auto it = m_reservations.find(zoneId);
        if (it != m_reservations.end()) {
            if (it->second.vehicleId != vehicleId) {
                return true;
            }
        }
    }
    return false;
}

bool ConflictManager::CanReserve(int vehicleId, const std::string& pathId, bool isEmergency) const {
    const auto& zones = GetZonesForPath(pathId);
    for (const auto& zoneId : zones) {
        auto it = m_reservations.find(zoneId);
        if (it != m_reservations.end()) {
            if (it->second.vehicleId != vehicleId) {
                // Strict mutual exclusion: zone cannot be occupied by more than 1 vehicle at a time
                return false;
            }
        }
    }
    return true;
}

bool ConflictManager::RequestReservation(int vehicleId, const std::string& pathId, float currentTime, bool isEmergency) {
    if (!CanReserve(vehicleId, pathId, isEmergency)) {
        return false;
    }

    const auto& zones = GetZonesForPath(pathId);
    for (const auto& zoneId : zones) {
        ZoneReservation res;
        res.vehicleId = vehicleId;
        res.pathId = pathId;
        res.timestamp = currentTime;
        res.isEmergency = isEmergency;
        m_reservations[zoneId] = res;
    }
    return true;
}

void ConflictManager::ReleaseReservation(int vehicleId) {
    for (auto it = m_reservations.begin(); it != m_reservations.end(); ) {
        if (it->second.vehicleId == vehicleId) {
            it = m_reservations.erase(it);
        } else {
            ++it;
        }
    }
}

void ConflictManager::Update(float currentTime, float timeoutSeconds) {
    for (auto it = m_reservations.begin(); it != m_reservations.end(); ) {
        if (currentTime - it->second.timestamp > timeoutSeconds) {
            it = m_reservations.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace SmartCity::sim
