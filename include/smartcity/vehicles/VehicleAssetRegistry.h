#pragma once

#include "smartcity/vehicles/VehiclePreset.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace SmartCity::vehicles {

struct VehicleAssetEntry {
    uint32_t modelHash{ 0 };
    std::string displayName;
    VehicleCategory category{ VehicleCategory::Supercar };
    std::string modelAssetPath;
    ModelAssetStatus assetStatus{ ModelAssetStatus::MissingModel };
    VehicleSimulationProfile simProfile;
    std::vector<std::string> texturePaths;
    std::string wheelModelPath;
};

class VehicleAssetRegistry {
public:
    static VehicleAssetRegistry& Get();

    // Load asset manifest from json file (e.g. assets/vehicle_presets/vehicle_assets.json)
    bool LoadManifest(const std::string& manifestPath);

    // Save current manifest to json file
    bool SaveManifest(const std::string& manifestPath) const;

    // Register or update an asset entry
    void RegisterAsset(const VehicleAssetEntry& entry);

    // Query asset entry by model hash
    const VehicleAssetEntry* FindAsset(uint32_t modelHash) const;

    // Check if a model asset actually exists and can be loaded
    static bool VerifyAssetExists(const std::string& assetPath);

    // Apply asset and simulation profiles to a vehicle preset
    void ResolvePresetAssets(VehiclePreset& preset) const;

    // Populate default entries for all known vehicle models if manifest missing
    void PopulateDefaultKnowledgeBase();

    // Get all registered asset entries
    const std::unordered_map<uint32_t, VehicleAssetEntry>& GetAllAssets() const { return m_assets; }

private:
    VehicleAssetRegistry();
    std::unordered_map<uint32_t, VehicleAssetEntry> m_assets;
    std::string m_manifestPath;
};

} // namespace SmartCity::vehicles

