#include "smartcity/vehicles/VehiclePresetRegistry.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace SmartCity::vehicles;
namespace fs = std::filesystem;

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Vehicle Registry Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    std::string sourceDir = "assets/vehicle_presets/source_json";
    if (!fs::exists(sourceDir)) {
        sourceDir = "../assets/vehicle_presets/source_json";
    }
    assert(fs::exists(sourceDir));

    auto& registry = VehiclePresetRegistry::Get();
    registry.LoadPresets(sourceDir);

    std::cout << "[Test 1] Registry Population & Totals..." << std::endl;
    assert(registry.GetTotalCount() >= 66);
    std::cout << "  Total presets registered: " << registry.GetTotalCount() << std::endl;
    std::cout << "  PASS: Presets loaded successfully." << std::endl;

    std::cout << "[Test 2] Query by Model Hash (3356898886)..." << std::endl;
    auto luivaPresets = registry.GetPresetsByModelHash(3356898886u);
    assert(!luivaPresets.empty());
    std::cout << "  Found " << luivaPresets.size() << " preset(s) for model hash 3356898886." << std::endl;
    bool foundProgenLuiva = false;
    for (const auto* p : luivaPresets) {
        if (p->displayName.find("LUIVA") != std::string::npos || p->displayName.find("Luiva") != std::string::npos) {
            foundProgenLuiva = true;
        }
    }
    assert(foundProgenLuiva);
    std::cout << "  PASS: Model hash querying verified." << std::endl;

    std::cout << "[Test 3] Text Search Query ('Jesko', 'Audi', 'Zonda')..." << std::endl;
    PresetFilterCriteria critJesko;
    critJesko.searchQuery = "Jesko";
    auto jeskoResults = registry.QueryPresets(critJesko);
    assert(!jeskoResults.empty());
    std::cout << "  Found " << jeskoResults.size() << " match(es) for 'Jesko'." << std::endl;

    PresetFilterCriteria critAudi;
    critAudi.searchQuery = "Audi";
    auto audiResults = registry.QueryPresets(critAudi);
    assert(!audiResults.empty());
    std::cout << "  Found " << audiResults.size() << " match(es) for 'Audi'." << std::endl;

    PresetFilterCriteria critZonda;
    critZonda.searchQuery = "Zonda";
    auto zondaResults = registry.QueryPresets(critZonda);
    assert(!zondaResults.empty());
    std::cout << "  Found " << zondaResults.size() << " match(es) for 'Zonda'." << std::endl;
    std::cout << "  PASS: Text search query verified." << std::endl;

    std::cout << "[Test 4] Category Filtering..." << std::endl;
    PresetFilterCriteria critSuper;
    critSuper.category = VehicleCategory::Supercar;
    auto superResults = registry.QueryPresets(critSuper);
    assert(!superResults.empty());
    std::cout << "  Supercars count: " << superResults.size() << std::endl;

    PresetFilterCriteria critSports;
    critSports.category = VehicleCategory::Sports;
    auto sportsResults = registry.QueryPresets(critSports);
    assert(!sportsResults.empty());
    std::cout << "  Sports cars count: " << sportsResults.size() << std::endl;
    std::cout << "  PASS: Category filtering verified." << std::endl;

    std::cout << "\nALL VEHICLE REGISTRY TESTS PASSED!" << std::endl;
    return 0;
}

