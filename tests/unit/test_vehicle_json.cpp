#include "smartcity/vehicles/VehiclePresetImporter.h"
#include <iostream>
#include <cassert>

using namespace SmartCity::vehicles;

void TestValidCheraxJson() {
    std::cout << "[Test 1] Valid Cherax Entity JSON..." << std::endl;
    std::string jsonStr = R"({
        "format": "Cherax Entity",
        "type": 1,
        "model": 3356898886,
        "baseFlags": 66863,
        "mods": {
            "mod0": { "index": 6, "variation": false },
            "mod1": { "index": 2, "variation": false }
        },
        "wheelType": 10,
        "wheelColor": 111,
        "primaryColor": 143,
        "secondaryColor": 159,
        "pearlescentColor": 31,
        "isPrimaryColorCustom": false,
        "plateText": "TESTPLATE",
        "plateTextIndex": 1
    })";

    VehiclePresetImporter importer;
    auto res = importer.ImportFromString(jsonStr, "test_valid.json");
    assert(res.isSuccess);
    assert(res.modelHash == 3356898886u);
    assert(res.preset.modifications.size() >= 2);
    assert(res.preset.modifications[0].index == 6);
    assert(res.preset.modifications[0].variation == false);
    assert(res.preset.paint.primaryColor == 143);
    assert(res.preset.paint.secondaryColor == 159);
    assert(res.preset.plate.text == "TESTPLATE");
    std::cout << "  PASS: Valid Cherax Entity parsed with exact values." << std::endl;
}

void TestMissingOptionalProperties() {
    std::cout << "[Test 2] Missing Optional Properties (extras, driftTyres, neons)..." << std::endl;
    std::string jsonStr = R"({
        "format": "Cherax Entity",
        "type": 1,
        "model": 274946574,
        "baseFlags": 1000,
        "mods": {},
        "wheelType": 2,
        "wheelColor": 5,
        "primaryColor": 12,
        "secondaryColor": 15
    })";

    VehiclePresetImporter importer;
    auto res = importer.ImportFromString(jsonStr, "test_missing.json");
    assert(res.isSuccess);
    assert(!res.preset.hasExtras);
    assert(!res.preset.neon.hasNeon);
    assert(!res.preset.wheels.hasTyreSettings);
    std::cout << "  PASS: Missing optional fields handled cleanly without failure." << std::endl;
}

void TestZeroValuedAndFalseProperties() {
    std::cout << "[Test 3] Zero-valued Numerics and False Booleans..." << std::endl;
    std::string jsonStr = R"({
        "format": "Cherax Entity",
        "type": 1,
        "model": 0,
        "baseFlags": 0,
        "mods": {
            "mod0": { "index": 0, "variation": false }
        },
        "wheelType": 0,
        "wheelColor": 0,
        "primaryColor": 0,
        "secondaryColor": 0,
        "isPrimaryColorCustom": false,
        "isSecondaryColorCustom": false,
        "bulletproofTyres": false,
        "driftTyres": false
    })";

    VehiclePresetImporter importer;
    auto res = importer.ImportFromString(jsonStr, "test_zeros.json");
    assert(res.isSuccess);
    assert(res.modelHash == 0u);
    assert(res.preset.modifications[0].index == 0);
    assert(res.preset.modifications[0].variation == false);
    assert(res.preset.paint.primaryColor == 0);
    assert(!res.preset.wheels.bulletproofTyres);
    assert(!res.preset.wheels.driftTyres);
    std::cout << "  PASS: Zero and false values preserved faithfully." << std::endl;
}

void TestInvalidAndEmptyJson() {
    std::cout << "[Test 4] Invalid Syntax and Empty JSON..." << std::endl;
    VehiclePresetImporter importer;

    // Empty
    auto resEmpty = importer.ImportFromString("", "empty.json");
    assert(resEmpty.isMalformed);
    assert(!resEmpty.isSuccess);

    // Syntax Error
    auto resSyntax = importer.ImportFromString("{ format: broken, }", "broken.json");
    assert(resSyntax.isMalformed);
    assert(!resSyntax.isSuccess);

    // Non-object root
    auto resArray = importer.ImportFromString("[ 1, 2, 3 ]", "array.json");
    assert(resArray.isMalformed);
    assert(!resArray.isSuccess);

    std::cout << "  PASS: Malformed and empty JSON safely rejected with errors recorded." << std::endl;
}

void TestUnknownPropertiesPreserved() {
    std::cout << "[Test 5] Unknown / Extended Properties Preservation..." << std::endl;
    std::string jsonStr = R"({
        "format": "Cherax Entity",
        "type": 1,
        "model": 123456,
        "mods": {},
        "_creatorSignature": "ModderX",
        "customArmorValue": 250,
        "attachments": { "attachedTo": "bone_chassis" }
    })";

    VehiclePresetImporter importer;
    auto res = importer.ImportFromString(jsonStr, "test_extended.json");
    assert(res.isSuccess);
    assert(!res.preset.unknownProperties.empty());
    assert(res.preset.unknownProperties.contains("_creatorSignature"));
    assert(res.preset.unknownProperties["_creatorSignature"] == "ModderX");
    assert(res.preset.unknownProperties.contains("attachments"));
    std::cout << "  PASS: Unknown properties preserved in preset." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Vehicle JSON Schema Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    TestValidCheraxJson();
    TestMissingOptionalProperties();
    TestZeroValuedAndFalseProperties();
    TestInvalidAndEmptyJson();
    TestUnknownPropertiesPreserved();

    std::cout << "\nALL VEHICLE JSON TESTS PASSED!" << std::endl;
    return 0;
}

