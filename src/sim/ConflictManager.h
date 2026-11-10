#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "city/RoadNetwork.h"

namespace SmartCity::sim {

struct ZoneReservation {
    int vehicleId{ -1 };
    std::string pathId;
    float timestamp{ 0.0f };
    bool isEmergency{ false };
};

class ConflictManager {
public:
    explicit ConflictManager(const city::RoadNetwork& roadNetwork);

    // Checks if all conflict zones needed by pathId are free (or reserved by this vehicle)
    bool CanReserve(int vehicleId, const std::string& pathId, bool isEmergency) const;

    // Attempts to reserve all zones for pathId; returns true if successful
    bool RequestReservation(int vehicleId, const std::string& pathId, float currentTime, bool isEmergency);

    // Releases all conflict zones held by vehicleId
    void ReleaseReservation(int vehicleId);

    // Returns whether any zone associated with pathId is currently occupied by another vehicle
    bool HasConflictingReservation(int vehicleId, const std::string& pathId) const;

    // Deadlock timeout and cleanup
    void Update(float currentTime, float timeoutSeconds = 15.0f);

    void Reset();

    // Queries
    bool IsZoneReserved(const std::string& zoneId) const;
    int GetReservingVehicle(const std::string& zoneId) const;
    const std::vector<std::string>& GetZonesForPath(const std::string& pathId) const;

private:
    const city::RoadNetwork& m_roadNetwork;
    // Map of zoneId -> active reservation
    std::unordered_map<std::string, ZoneReservation> m_reservations;
    // Lookup from pathId -> list of zoneIds
    std::unordered_map<std::string, std::vector<std::string>> m_pathToZones;
    static const std::vector<std::string> s_emptyZoneList;
};

} // namespace SmartCity::sim

