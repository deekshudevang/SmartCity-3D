#include "smartcity/vehicles/VehiclePresetImporter.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace SmartCity::vehicles;
namespace fs = std::filesystem;

void TestDirectoryScanAndNormalization() {
    std::cout << "[Test 1] Directory Scanning and Normalization..." << std::endl;

    std::string sourceDir = "assets/vehicle_presets/source_json";
    if (!fs::exists(sourceDir)) {
        sourceDir = "../assets/vehicle_presets/source_json";
    }
    assert(fs::exists(sourceDir));

    std::string normOutDir = "assets/vehicle_presets/normalized";
    if (!fs::exists(normOutDir) && fs::exists("../assets")) {
        normOutDir = "../assets/vehicle_presets/normalized";
    }

    VehiclePresetImporter importer;
    ImportReport report = importer.ImportDirectory(sourceDir, normOutDir);

    std::cout << "  Discovered: " << report.filesDiscovered << " files" << std::endl;
    std::cout << "  Successfully Normalized: " << report.successfullyNormalized << std::endl;
    std::cout << "  Duplicates: " << report.duplicateConfigurations << std::endl;

    assert(report.filesDiscovered >= 66);
    assert(report.successfullyNormalized >= 60);

    std::cout << "  PASS: Directory imported and normalized successfully." << std::endl;
}

void TestProgenLuivaRegression() {
    std::cout << "[Test 2] PROGEN LUIVA.json Regression Test..." << std::endl;

    std::string filePath = "assets/vehicle_presets/source_json/PROGEN LUIVA.json";
    if (!fs::exists(filePath)) {
        filePath = "../assets/vehicle_presets/source_json/PROGEN LUIVA.json";
    }
    assert(fs::exists(filePath));

    VehiclePresetImporter importer;
    FileImportResult res = importer.ImportFile(filePath);

    assert(res.isSuccess);
    assert(res.modelHash == 3356898886u);
    assert(res.preset.paint.primaryColor == 143);
    assert(res.preset.paint.secondaryColor == 159);
    assert(res.preset.paint.pearlescentColor == 31);
    assert(res.preset.plate.text == "MC ELVA ");
    assert(res.preset.plate.styleIndex == 6);
    assert(res.preset.wheels.wheelType == 10);
    assert(res.preset.wheels.bulletproofTyres == true);
    assert(res.preset.wheels.driftTyres == false);

    // Modifications
    assert(res.preset.modifications[0].index == 6);
    assert(res.preset.modifications[14].index == 37);
    assert(res.preset.modifications[22].index == 10);
    assert(res.preset.modifications[23].index == 47);

    // Neons
    assert(res.preset.neon.hasNeon == true);
    assert(res.preset.neon.left == true);
    assert(res.preset.neon.right == true);
    assert(res.preset.neon.front == true);
    assert(res.preset.neon.back == true);
    assert(res.preset.neon.color.r == 255);
    assert(res.preset.neon.color.g == 0);
    assert(res.preset.neon.color.b == 0);

    std::cout << "  PASS: PROGEN LUIVA.json exact values verified." << std::endl;
}

void TestContentHashDeduplication() {
    std::cout << "[Test 3] Content Hash Deduplication..." << std::endl;

    std::string sample = R"({
        "format": "Cherax Entity",
        "type": 1,
        "model": 999999,
        "mods": {},
        "primaryColor": 10
    })";

    VehiclePresetImporter importer;
    auto res1 = importer.ImportFromString(sample, "vehicle_a.json");
    assert(res1.isSuccess);
    assert(!res1.isDuplicate);

    auto res2 = importer.ImportFromString(sample, "vehicle_copy.json");
    assert(res2.isSuccess);
    assert(res2.isDuplicate);
    assert(res2.contentHash == res1.contentHash);

    std::cout << "  PASS: Content hash deduplication accurately detected identical files." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Vehicle Importer Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    TestDirectoryScanAndNormalization();
    TestProgenLuivaRegression();
    TestContentHashDeduplication();

    std::cout << "\nALL VEHICLE IMPORTER TESTS PASSED!" << std::endl;
    return 0;
}

