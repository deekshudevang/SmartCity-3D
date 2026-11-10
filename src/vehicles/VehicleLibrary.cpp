#include "smartcity/vehicles/VehicleLibrary.h"
#include "sim/VehicleManager.h"
#include "core/Logger.h"
#include <filesystem>

namespace SmartCity::vehicles {

namespace fs = std::filesystem;

VehicleLibrary& VehicleLibrary::Get() {
    static VehicleLibrary s_instance;
    return s_instance;
}

VehicleLibrary::VehicleLibrary()
    : m_sourceDir("assets/vehicle_presets/source_json")
    , m_normalizedDir("assets/vehicle_presets/normalized")
    , m_manifestPath("assets/vehicle_presets/vehicle_assets.json")
    , m_reportPath("assets/vehicle_presets/import_report.json")
{
}

bool VehicleLibrary::Initialize(const std::string& sourceDir,
                                const std::string& normalizedDir,
                                const std::string& manifestPath,
                                const std::string& reportPath) {
    m_sourceDir = sourceDir;
    m_normalizedDir = normalizedDir;
    m_manifestPath = manifestPath;
    m_reportPath = reportPath;

    core::Logger::Info("VehicleLibrary: Initializing Vehicle Library System...");

    // 1. Ensure required directories exist
    std::error_code ec;
    fs::create_directories(m_sourceDir, ec);
    fs::create_directories(m_normalizedDir, ec);
    fs::create_directories("assets/models/vehicles/supercars", ec);
    fs::create_directories("assets/models/vehicles/sports", ec);
    fs::create_directories("assets/models/vehicles/sedans", ec);
    fs::create_directories("assets/models/vehicles/suvs", ec);
    fs::create_directories("assets/models/vehicles/buses", ec);
    fs::create_directories("assets/models/vehicles/emergency", ec);
    fs::create_directories("assets/models/vehicles/motorcycles", ec);

    // 2. Load or generate vehicle asset manifest
    auto& assetRegistry = VehicleAssetRegistry::Get();
    if (!assetRegistry.LoadManifest(m_manifestPath)) {
        core::Logger::Info("VehicleLibrary: Generating new asset manifest at ", m_manifestPath);
        assetRegistry.SaveManifest(m_manifestPath);
    }

    // 3. Import and normalize all vehicle presets from source_json
    auto& presetRegistry = VehiclePresetRegistry::Get();
    presetRegistry.LoadPresets(m_sourceDir, m_normalizedDir, m_reportPath);

    // 4. Default selection: prioritize PROGEN LUIVA or first valid preset
    const auto& all = presetRegistry.GetAllPresets();
    if (!all.empty()) {
        m_selectedPresetId = all[0].id;
        for (const auto& p : all) {
            if (p.displayName.find("PROGEN LUIVA") != std::string::npos ||
                p.displayName.find("Progen Luiva") != std::string::npos) {
                m_selectedPresetId = p.id;
                break;
            }
        }
        core::Logger::Info("VehicleLibrary: Default vehicle selected: '", m_selectedPresetId, "'.");
    }

    core::Logger::Info("VehicleLibrary: Initialization complete. Total presets: ", all.size());
    return true;
}

ImportReport VehicleLibrary::RescanSourcePresets() {
    core::Logger::Info("VehicleLibrary: Rescanning source JSON presets...");
    auto& presetRegistry = VehiclePresetRegistry::Get();
    ImportReport report = presetRegistry.LoadPresets(m_sourceDir, m_normalizedDir, m_reportPath);

    // Re-verify selection
    if (!m_selectedPresetId.empty() && !presetRegistry.GetPresetById(m_selectedPresetId)) {
        const auto& all = presetRegistry.GetAllPresets();
        if (!all.empty()) {
            m_selectedPresetId = all[0].id;
        } else {
            m_selectedPresetId.clear();
        }
    }
    return report;
}

void VehicleLibrary::SelectPreset(const std::string& presetId) {
    if (VehiclePresetRegistry::Get().GetPresetById(presetId)) {
        m_selectedPresetId = presetId;
    }
}

const VehiclePreset* VehicleLibrary::GetSelectedPreset() const {
    if (m_selectedPresetId.empty()) return nullptr;
    return VehiclePresetRegistry::Get().GetPresetById(m_selectedPresetId);
}

VehiclePreset* VehicleLibrary::GetSelectedPresetMutable() {
    if (m_selectedPresetId.empty()) return nullptr;
    // Helper to get non-const ptr
    auto& reg = VehiclePresetRegistry::Get();
    const auto* p = reg.GetPresetById(m_selectedPresetId);
    if (!p) return nullptr;
    return const_cast<VehiclePreset*>(p);
}

bool VehicleLibrary::SpawnSelectedVehicleIntoSim(sim::VehicleManager& simManager, int approachId, int movementType) {
    const auto* preset = GetSelectedPreset();
    if (!preset) return false;

    city::ApproachId app = static_cast<city::ApproachId>(std::clamp(approachId, 0, 3));
    city::MovementType mov = static_cast<city::MovementType>(std::clamp(movementType, 0, 2));

    sim::VehicleType simType = sim::VehicleType::Sedan;
    if (preset->category == VehicleCategory::Supercar || preset->category == VehicleCategory::Sports) {
        simType = sim::VehicleType::Sedan; // Passenger physics
    } else if (preset->category == VehicleCategory::Bus) {
        simType = sim::VehicleType::Bus;
    } else if (preset->category == VehicleCategory::Emergency) {
        simType = sim::VehicleType::Emergency;
    }

    auto* spawned = simManager.SpawnVehicle(app, mov, simType);
    if (spawned) {
        // Set custom paint colors derived from preset
        glm::vec3 priColor = preset->GetEffectivePrimaryColorRgb();
        spawned->SetColor(glm::vec4(priColor, 1.0f));

        core::Logger::Info("VehicleLibrary: Spawned '", preset->displayName,
                           "' (Model: ", preset->modelHash, ") on approach ", static_cast<int>(app));
        return true;
    }

    return false;
}

} // namespace SmartCity::vehicles

