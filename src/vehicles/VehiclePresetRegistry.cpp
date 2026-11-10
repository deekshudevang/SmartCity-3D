#include "smartcity/vehicles/VehiclePresetRegistry.h"
#include "core/Logger.h"
#include <algorithm>

namespace SmartCity::vehicles {

VehiclePresetRegistry& VehiclePresetRegistry::Get() {
    static VehiclePresetRegistry s_instance;
    return s_instance;
}

void VehiclePresetRegistry::Clear() {
    m_presets.clear();
    m_idIndexMap.clear();
    m_modelHashIndexMap.clear();
}

void VehiclePresetRegistry::RegisterPreset(const VehiclePreset& preset) {
    auto it = m_idIndexMap.find(preset.id);
    if (it != m_idIndexMap.end()) {
        m_presets[it->second] = preset;
    } else {
        size_t idx = m_presets.size();
        m_presets.push_back(preset);
        m_idIndexMap[preset.id] = idx;
        m_modelHashIndexMap[preset.modelHash].push_back(idx);
    }
}

ImportReport VehiclePresetRegistry::LoadPresets(const std::string& sourceDir,
                                                const std::string& normalizedDir,
                                                const std::string& reportOutPath) {
    Clear();
    VehiclePresetImporter importer;
    m_lastReport = importer.ImportDirectory(sourceDir, normalizedDir);

    auto& assetRegistry = VehicleAssetRegistry::Get();

    int availableCount = 0;
    int missingCount = 0;

    for (auto& fileRes : m_lastReport.fileResults) {
        if (fileRes.isSuccess) {
            assetRegistry.ResolvePresetAssets(fileRes.preset);
            if (fileRes.preset.IsModelAvailable()) {
                availableCount++;
            } else {
                missingCount++;
            }
            RegisterPreset(fileRes.preset);
        }
    }

    m_lastReport.modelsWithAssets = availableCount;
    m_lastReport.modelsWithMissingAssets = missingCount;

    if (!reportOutPath.empty()) {
        m_lastReport.SaveToFile(reportOutPath);
    }

    core::Logger::Info("VehiclePresetRegistry: Loaded ", m_presets.size(), " presets into database (",
                       availableCount, " with 3D models, ", missingCount, " missing 3D models).");

    return m_lastReport;
}

const VehiclePreset* VehiclePresetRegistry::GetPresetById(const std::string& id) const {
    auto it = m_idIndexMap.find(id);
    if (it != m_idIndexMap.end()) {
        return &m_presets[it->second];
    }
    return nullptr;
}

std::vector<const VehiclePreset*> VehiclePresetRegistry::GetPresetsByModelHash(uint32_t modelHash) const {
    std::vector<const VehiclePreset*> out;
    auto it = m_modelHashIndexMap.find(modelHash);
    if (it != m_modelHashIndexMap.end()) {
        for (size_t idx : it->second) {
            out.push_back(&m_presets[idx]);
        }
    }
    return out;
}

size_t VehiclePresetRegistry::GetAvailableModelCount() const {
    size_t count = 0;
    for (const auto& p : m_presets) {
        if (p.IsModelAvailable()) count++;
    }
    return count;
}

size_t VehiclePresetRegistry::GetMissingModelCount() const {
    size_t count = 0;
    for (const auto& p : m_presets) {
        if (!p.IsModelAvailable()) count++;
    }
    return count;
}

static std::string ToLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<const VehiclePreset*> VehiclePresetRegistry::QueryPresets(const PresetFilterCriteria& criteria) const {
    std::vector<const VehiclePreset*> results;
    std::string query = ToLower(criteria.searchQuery);

    for (const auto& p : m_presets) {
        // 1. Category filter
        if (criteria.category != VehicleCategory::Unknown && p.category != criteria.category) {
            continue;
        }

        // 2. Asset status filter
        if (criteria.assetStatusFilter == 1 && !p.IsModelAvailable()) {
            continue; // Available only
        } else if (criteria.assetStatusFilter == 2 && p.IsModelAvailable()) {
            continue; // Missing model only
        }

        // 3. Validation filter
        if (criteria.validationFilter == 1 && !p.IsValid()) {
            continue; // Valid only
        } else if (criteria.validationFilter == 2 && p.IsValid()) {
            continue; // Malformed/invalid only
        }

        // 4. Text query
        if (!query.empty()) {
            std::string nameLower = ToLower(p.displayName);
            std::string fileLower = ToLower(p.sourceFile);
            std::string hashStr = std::to_string(p.modelHash);
            std::string catLower = ToLower(VehiclePreset::CategoryToString(p.category));
            std::string plateLower = ToLower(p.plate.text);

            bool matches = (nameLower.find(query) != std::string::npos) ||
                           (fileLower.find(query) != std::string::npos) ||
                           (hashStr.find(query) != std::string::npos) ||
                           (catLower.find(query) != std::string::npos) ||
                           (plateLower.find(query) != std::string::npos);

            if (!matches) continue;
        }

        results.push_back(&p);
    }

    return results;
}

} // namespace SmartCity::vehicles

