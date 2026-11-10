#pragma once

#include "smartcity/vehicles/VehiclePreset.h"
#include "smartcity/vehicles/VehiclePresetImporter.h"
#include "smartcity/vehicles/VehicleAssetRegistry.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace SmartCity::vehicles {

struct PresetFilterCriteria {
    VehicleCategory category{ VehicleCategory::Unknown }; // Unknown means all
    int assetStatusFilter{ 0 };  // 0 = All, 1 = Available, 2 = Missing Model
    int validationFilter{ 0 };   // 0 = All, 1 = Valid Only, 2 = Invalid/Malformed Only
    std::string searchQuery;     // Text query (name, hash, filename, category)
};

class VehiclePresetRegistry {
public:
    static VehiclePresetRegistry& Get();

    // Scan source directory or load normalized presets
    ImportReport LoadPresets(const std::string& sourceDir,
                             const std::string& normalizedDir = "",
                             const std::string& reportOutPath = "");

    // Add or replace a preset
    void RegisterPreset(const VehiclePreset& preset);

    // Queries
    const VehiclePreset* GetPresetById(const std::string& id) const;
    std::vector<const VehiclePreset*> GetPresetsByModelHash(uint32_t modelHash) const;
    const std::vector<VehiclePreset>& GetAllPresets() const { return m_presets; }

    // Filtering and Searching
    std::vector<const VehiclePreset*> QueryPresets(const PresetFilterCriteria& criteria) const;

    // Telemetry & Statistics
    size_t GetTotalCount() const { return m_presets.size(); }
    size_t GetAvailableModelCount() const;
    size_t GetMissingModelCount() const;
    const ImportReport& GetLastImportReport() const { return m_lastReport; }

    // Clear all
    void Clear();

private:
    VehiclePresetRegistry() = default;

    std::vector<VehiclePreset> m_presets;
    std::unordered_map<std::string, size_t> m_idIndexMap;
    std::unordered_map<uint32_t, std::vector<size_t>> m_modelHashIndexMap;
    ImportReport m_lastReport;
};

} // namespace SmartCity::vehicles

