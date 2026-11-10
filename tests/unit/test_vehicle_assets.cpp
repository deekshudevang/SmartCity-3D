#include "smartcity/vehicles/VehicleAssetRegistry.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace SmartCity::vehicles;
namespace fs = std::filesystem;

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Vehicle Asset Registry Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    auto& assetReg = VehicleAssetRegistry::Get();

    std::cout << "[Test 1] Built-in Knowledge Base Model Lookup..." << std::endl;
    const auto* entry = assetReg.FindAsset(3356898886u);
    assert(entry != nullptr);
    assert(entry->displayName == "Progen Luiva");
    assert(entry->category == VehicleCategory::Supercar);
    std::cout << "  Model 3356898886 mapped to: " << entry->displayName
              << " (" << VehiclePreset::CategoryToString(entry->category) << ")" << std::endl;
    std::cout << "  PASS: Knowledge base lookup verified." << std::endl;

    std::cout << "[Test 2] Asset Existence Verification..." << std::endl;
    // Known existing asset
    std::string existingFile = "assets/models/test_cube.obj";
    if (!fs::exists(existingFile) && fs::exists("../assets/models/test_cube.obj")) {
        existingFile = "../assets/models/test_cube.obj";
    }
    assert(VehicleAssetRegistry::VerifyAssetExists(existingFile));

    // Non-existent asset
    assert(!VehicleAssetRegistry::VerifyAssetExists("assets/models/vehicles/supercars/non_existent_car.glb"));
    assert(!VehicleAssetRegistry::VerifyAssetExists(""));
    std::cout << "  PASS: Asset existence check correctly distinguishes present vs missing files." << std::endl;

    std::cout << "[Test 3] Preset Asset Resolution (Honest MISSING_MODEL Status)..." << std::endl;
    VehiclePreset preset;
    preset.modelHash = 3356898886u;
    assetReg.ResolvePresetAssets(preset);

    assert(preset.category == VehicleCategory::Supercar);
    assert(preset.displayName == "Progen Luiva");
    assert(preset.assetStatus == ModelAssetStatus::MissingModel);
    assert(!preset.IsModelAvailable());
    std::cout << "  PASS: Missing 3D assets honestly reported as MISSING_MODEL without false generic claiming." << std::endl;

    std::cout << "[Test 4] Manifest Round-Trip Serialization..." << std::endl;
    std::string manifestTestPath = "assets/vehicle_presets/test_manifest.json";
    if (!fs::exists("assets/vehicle_presets") && fs::exists("../assets/vehicle_presets")) {
        manifestTestPath = "../assets/vehicle_presets/test_manifest.json";
    }

    bool saved = assetReg.SaveManifest(manifestTestPath);
    assert(saved);
    assert(fs::exists(manifestTestPath));

    bool reloaded = assetReg.LoadManifest(manifestTestPath);
    assert(reloaded);
    std::cout << "  PASS: Manifest successfully exported and re-imported." << std::endl;

    // Clean up test file
    fs::remove(manifestTestPath);

    std::cout << "\nALL VEHICLE ASSET REGISTRY TESTS PASSED!" << std::endl;
    return 0;
}

