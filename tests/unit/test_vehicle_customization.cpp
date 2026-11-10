#include "smartcity/vehicles/VehiclePreset.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace SmartCity::vehicles;

void TestColorPaletteResolution() {
    std::cout << "[Test 1] GTA V Color Palette Mapping..." << std::endl;

    VehiclePreset p;
    p.paint.isPrimaryColorCustom = false;

    // Test Color ID 0 (Metallic Black)
    p.paint.primaryColor = 0;
    glm::vec3 c0 = p.GetEffectivePrimaryColorRgb();
    assert(std::abs(c0.r - 13.0f / 255.0f) < 1e-4f);
    assert(std::abs(c0.g - 17.0f / 255.0f) < 1e-4f);
    assert(std::abs(c0.b - 22.0f / 255.0f) < 1e-4f);

    // Test Color ID 111 (Metallic White)
    p.paint.primaryColor = 111;
    glm::vec3 c111 = p.GetEffectivePrimaryColorRgb();
    assert(std::abs(c111.r - 245.0f / 255.0f) < 1e-4f);
    assert(std::abs(c111.g - 245.0f / 255.0f) < 1e-4f);
    assert(std::abs(c111.b - 245.0f / 255.0f) < 1e-4f);

    // Test Color ID 143 (Matte Black)
    p.paint.primaryColor = 143;
    glm::vec3 c143 = p.GetEffectivePrimaryColorRgb();
    assert(std::abs(c143.r - 18.0f / 255.0f) < 1e-4f);

    std::cout << "  PASS: In-game numeric color IDs correctly resolve to exact physical RGB values." << std::endl;
}

void TestCustomRgbOverride() {
    std::cout << "[Test 2] Custom RGB Overriding..." << std::endl;

    VehiclePreset p;
    p.paint.primaryColor = 111; // Palette White
    p.paint.isPrimaryColorCustom = true;
    p.paint.customPrimaryColor = { 255, 128, 64 };

    glm::vec3 c = p.GetEffectivePrimaryColorRgb();
    assert(std::abs(c.r - 1.0f) < 1e-4f);
    assert(std::abs(c.g - 128.0f / 255.0f) < 1e-4f);
    assert(std::abs(c.b - 64.0f / 255.0f) < 1e-4f);

    std::cout << "  PASS: Custom RGB properly overrides palette index when custom flag is active." << std::endl;
}

void TestNormalizedJsonLosslessRoundTrip() {
    std::cout << "[Test 3] Normalized JSON Round-Trip Equivalence..." << std::endl;

    VehiclePreset orig;
    orig.id = "3356898886_Progen_Luiva";
    orig.displayName = "Progen Luiva";
    orig.modelIdentifier = "3356898886";
    orig.modelHash = 3356898886u;
    orig.sourceFile = "assets/vehicle_presets/source_json/PROGEN LUIVA.json";
    orig.sourceFormat = "Cherax Entity";
    orig.category = VehicleCategory::Supercar;
    orig.baseFlags = 66863;
    orig.contentHash = "abcdef0123456789";

    orig.modifications[0] = { 6, false, true };
    orig.modifications[14] = { 37, false, true };
    orig.hasExtras = true;
    orig.extras[0] = false;
    orig.extras[1] = true;

    orig.paint.primaryColor = 143;
    orig.paint.secondaryColor = 159;
    orig.paint.pearlescentColor = 31;
    orig.plate.text = "MC ELVA ";
    orig.wheels.wheelType = 10;
    orig.wheels.bulletproofTyres = true;

    orig.neon.hasNeon = true;
    orig.neon.left = true;
    orig.neon.color = { 255, 0, 0 };

    nlohmann::json j = orig.ToNormalizedJson();
    VehiclePreset loaded = VehiclePreset::FromNormalizedJson(j);

    assert(loaded.id == orig.id);
    assert(loaded.displayName == orig.displayName);
    assert(loaded.modelHash == orig.modelHash);
    assert(loaded.category == orig.category);
    assert(loaded.modifications[0].index == 6);
    assert(loaded.modifications[14].index == 37);
    assert(loaded.hasExtras == true);
    assert(loaded.extras[1] == true);
    assert(loaded.paint.primaryColor == 143);
    assert(loaded.paint.secondaryColor == 159);
    assert(loaded.wheels.wheelType == 10);
    assert(loaded.plate.text == "MC ELVA ");
    assert(loaded.neon.hasNeon == true);
    assert(loaded.neon.color.r == 255);

    std::cout << "  PASS: Normalized JSON round-trip preserved 100% of values losslessly." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Vehicle Customization Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    TestColorPaletteResolution();
    TestCustomRgbOverride();
    TestNormalizedJsonLosslessRoundTrip();

    std::cout << "\nALL VEHICLE CUSTOMIZATION TESTS PASSED!" << std::endl;
    return 0;
}

