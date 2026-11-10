#pragma once

#include "smartcity/vehicles/VehiclePreset.h"
#include "smartcity/vehicles/VehiclePresetRegistry.h"
#include "smartcity/vehicles/VehicleAssetRegistry.h"
#include "smartcity/vehicles/VehicleCustomization.h"
#include <string>
#include <memory>

namespace SmartCity::sim {
class VehicleManager;
}

namespace SmartCity::vehicles {

class VehicleLibrary {
public:
    static VehicleLibrary& Get();

    // Initialize: loads asset manifest and imports/populates all vehicle presets
    bool Initialize(const std::string& sourceDir = "assets/vehicle_presets/source_json",
                    const std::string& normalizedDir = "assets/vehicle_presets/normalized",
                    const std::string& manifestPath = "assets/vehicle_presets/vehicle_assets.json",
                    const std::string& reportPath = "assets/vehicle_presets/import_report.json");

    // Re-scan source folder on demand (e.g. when user drops new JSON files)
    ImportReport RescanSourcePresets();

    // Preset Selection
    void SelectPreset(const std::string& presetId);
    const VehiclePreset* GetSelectedPreset() const;
    VehiclePreset* GetSelectedPresetMutable();

    // Spawning into Traffic Simulation
    bool SpawnSelectedVehicleIntoSim(sim::VehicleManager& simManager, int approachId = 0, int movementType = 1);

    // Customization Renderer
    VehicleCustomizationRenderer& GetRenderer() { return m_renderer; }

    // Registries
    VehiclePresetRegistry& GetPresetRegistry() { return VehiclePresetRegistry::Get(); }
    VehicleAssetRegistry& GetAssetRegistry() { return VehicleAssetRegistry::Get(); }

private:
    VehicleLibrary();

    std::string m_sourceDir;
    std::string m_normalizedDir;
    std::string m_manifestPath;
    std::string m_reportPath;

    std::string m_selectedPresetId;
    VehicleCustomizationRenderer m_renderer;
};

} // namespace SmartCity::vehicles

