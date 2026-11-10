#include "smartcity/vehicles/VehiclePreset.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace SmartCity::vehicles {

// Standard GTA V Vehicle Paint Palette (Color ID 0 to 159)
// Provides accurate RGB representation of in-game color indices
static const glm::ivec3 s_gtaColorPalette[160] = {
    { 13, 17, 22 },    { 28, 33, 41 },    { 58, 64, 72 },    { 69, 75, 86 },    { 42, 54, 58 },    // 0-4
    { 105, 113, 115 }, { 59, 64, 69 },    { 71, 87, 98 },    { 79, 106, 122 },  { 95, 125, 142 },  // 5-9
    { 34, 43, 56 },    { 26, 31, 38 },    { 29, 35, 45 },    { 38, 47, 59 },    { 69, 75, 79 },    // 10-14
    { 201, 201, 201 }, { 139, 141, 142 }, { 172, 178, 186 }, { 108, 107, 102 }, { 77, 90, 89 },    // 15-19
    { 64, 46, 52 },    { 34, 25, 24 },    { 83, 27, 27 },    { 142, 27, 31 },   { 150, 42, 42 },   // 20-24
    { 176, 54, 48 },   { 142, 70, 47 },   { 155, 32, 48 },   { 178, 34, 34 },   { 174, 31, 31 },   // 25-29
    { 138, 23, 28 },   { 131, 23, 30 },   { 152, 114, 39 },  { 145, 99, 44 },   { 78, 50, 39 },    // 30-34
    { 94, 62, 46 },    { 146, 107, 60 },  { 130, 86, 45 },   { 170, 89, 40 },   { 138, 51, 37 },   // 35-39
    { 117, 43, 25 },   { 84, 31, 24 },    { 159, 90, 44 },   { 178, 125, 60 },  { 199, 172, 120 }, // 40-44
    { 165, 115, 66 },  { 84, 53, 33 },    { 51, 36, 26 },    { 26, 34, 28 },    { 34, 49, 34 },    // 45-49
    { 40, 60, 40 },    { 51, 74, 49 },    { 66, 83, 55 },    { 48, 64, 60 },    { 34, 64, 86 },    // 50-54
    { 86, 125, 114 },  { 34, 76, 68 },    { 53, 89, 79 },    { 38, 64, 45 },    { 47, 79, 62 },    // 55-59
    { 43, 66, 50 },    { 36, 49, 40 },    { 28, 38, 33 },    { 38, 59, 43 },    { 63, 107, 158 },  // 60-64
    { 79, 125, 178 },  { 84, 118, 142 },  { 42, 82, 117 },   { 37, 58, 86 },    { 44, 70, 94 },    // 65-69
    { 33, 44, 66 },    { 23, 33, 51 },    { 38, 56, 82 },    { 51, 74, 102 },   { 79, 110, 146 },  // 70-74
    { 38, 48, 71 },    { 28, 34, 53 },    { 44, 53, 79 },    { 61, 73, 107 },   { 94, 112, 148 },  // 75-79
    { 41, 64, 89 },    { 28, 43, 61 },    { 38, 43, 56 },    { 61, 84, 112 },   { 84, 112, 146 },  // 80-84
    { 56, 74, 94 },    { 71, 92, 115 },   { 41, 56, 77 },    { 240, 184, 38 },  { 245, 158, 33 },  // 85-89
    { 240, 142, 33 },  { 224, 122, 28 },  { 158, 82, 23 },   { 148, 74, 23 },   { 209, 115, 33 },  // 90-94
    { 235, 133, 33 },  { 209, 148, 41 },  { 178, 125, 41 },  { 199, 158, 56 },  { 173, 138, 49 },  // 95-99
    { 148, 117, 41 },  { 122, 97, 33 },   { 209, 168, 66 },  { 224, 194, 92 },  { 245, 219, 122 }, // 100-104
    { 224, 178, 79 },  { 199, 148, 56 },  { 163, 122, 49 },  { 235, 219, 184 }, { 219, 204, 173 }, // 105-109
    { 204, 189, 163 }, { 245, 245, 245 }, { 178, 178, 178 }, { 122, 122, 122 }, { 77, 77, 77 },     // 110-114
    { 51, 51, 51 },    { 28, 28, 28 },    { 18, 18, 18 },    { 209, 199, 189 }, { 189, 178, 168 }, // 115-119
    { 148, 138, 128 }, { 107, 97, 87 },   { 66, 56, 46 },    { 46, 36, 26 },    { 245, 235, 224 }, // 120-124
    { 204, 194, 184 }, { 163, 153, 143 }, { 122, 112, 102 }, { 82, 71, 61 },    { 41, 31, 20 },    // 125-129
    { 245, 240, 235 }, { 204, 199, 194 }, { 163, 158, 153 }, { 122, 117, 112 }, { 82, 77, 71 },    // 130-134
    { 245, 245, 245 }, { 158, 158, 158 }, { 245, 148, 184 }, { 235, 117, 163 }, { 219, 87, 143 },  // 135-139
    { 87, 51, 77 },    { 41, 107, 71 },   { 28, 82, 51 },    { 18, 18, 18 },    { 38, 38, 38 },    // 140-144
    { 66, 66, 66 },    { 97, 97, 97 },    { 138, 138, 138 }, { 18, 28, 41 },    { 28, 41, 56 },    // 145-149
    { 148, 28, 38 },   { 178, 41, 51 },   { 209, 56, 66 },   { 189, 138, 41 },  { 209, 163, 56 },  // 150-154
    { 224, 184, 71 },  { 82, 122, 51 },   { 107, 148, 66 },  { 133, 178, 82 },  { 153, 199, 97 }   // 155-159
};

VehiclePreset::VehiclePreset() {
    // Default 50 modification slots
    for (int i = 0; i < 50; ++i) {
        modifications[i] = { -1, false, false };
    }
    // Default 16 extras
    for (int i = 0; i < 16; ++i) {
        extras[i] = false;
    }
}

glm::vec3 VehiclePreset::GetEffectivePrimaryColorRgb() const {
    if (paint.isPrimaryColorCustom) {
        return glm::vec3(
            static_cast<float>(paint.customPrimaryColor.r) / 255.0f,
            static_cast<float>(paint.customPrimaryColor.g) / 255.0f,
            static_cast<float>(paint.customPrimaryColor.b) / 255.0f
        );
    }
    int idx = std::clamp(paint.primaryColor, 0, 159);
    const auto& c = s_gtaColorPalette[idx];
    return glm::vec3(
        static_cast<float>(c.r) / 255.0f,
        static_cast<float>(c.g) / 255.0f,
        static_cast<float>(c.b) / 255.0f
    );
}

glm::vec3 VehiclePreset::GetEffectiveSecondaryColorRgb() const {
    if (paint.isSecondaryColorCustom) {
        return glm::vec3(
            static_cast<float>(paint.customSecondaryColor.r) / 255.0f,
            static_cast<float>(paint.customSecondaryColor.g) / 255.0f,
            static_cast<float>(paint.customSecondaryColor.b) / 255.0f
        );
    }
    int idx = std::clamp(paint.secondaryColor, 0, 159);
    const auto& c = s_gtaColorPalette[idx];
    return glm::vec3(
        static_cast<float>(c.r) / 255.0f,
        static_cast<float>(c.g) / 255.0f,
        static_cast<float>(c.b) / 255.0f
    );
}

std::string VehiclePreset::GetCategoryName() const {
    return CategoryToString(category);
}

std::string VehiclePreset::CategoryToString(VehicleCategory cat) {
    switch (cat) {
        case VehicleCategory::Supercar:    return "Supercar";
        case VehicleCategory::Sports:      return "Sports";
        case VehicleCategory::Sedan:       return "Sedan";
        case VehicleCategory::SUV:         return "SUV";
        case VehicleCategory::Bus:         return "Bus";
        case VehicleCategory::Emergency:   return "Emergency";
        case VehicleCategory::Motorcycle:  return "Motorcycle";
        case VehicleCategory::Unknown:
        default:                           return "Unknown";
    }
}

VehicleCategory VehiclePreset::StringToCategory(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "supercar" || lower == "supercars")     return VehicleCategory::Supercar;
    if (lower == "sports" || lower == "sport")           return VehicleCategory::Sports;
    if (lower == "sedan" || lower == "sedans")           return VehicleCategory::Sedan;
    if (lower == "suv" || lower == "suvs")               return VehicleCategory::SUV;
    if (lower == "bus" || lower == "buses")              return VehicleCategory::Bus;
    if (lower == "emergency")                            return VehicleCategory::Emergency;
    if (lower == "motorcycle" || lower == "motorcycles") return VehicleCategory::Motorcycle;
    return VehicleCategory::Unknown;
}

nlohmann::json VehiclePreset::ToNormalizedJson() const {
    nlohmann::json j;
    j["schemaVersion"] = "1.0.0";
    j["id"] = id;
    j["displayName"] = displayName;
    j["modelIdentifier"] = modelIdentifier;
    j["modelHash"] = modelHash;
    j["sourceFile"] = sourceFile;
    j["sourceFormat"] = sourceFormat;
    j["sourceDirectory"] = sourceDirectory;
    j["category"] = CategoryToString(category);
    j["baseFlags"] = baseFlags;
    j["contentHash"] = contentHash;

    // Modifications
    nlohmann::json modsObj = nlohmann::json::object();
    for (const auto& [slot, mod] : modifications) {
        if (mod.present) {
            modsObj["mod" + std::to_string(slot)] = {
                { "index", mod.index },
                { "variation", mod.variation }
            };
        }
    }
    j["mods"] = modsObj;

    // Extras
    if (hasExtras) {
        nlohmann::json extrasObj = nlohmann::json::object();
        for (const auto& [slot, enabled] : extras) {
            extrasObj["extra" + std::to_string(slot)] = enabled;
        }
        j["extras"] = extrasObj;
    }

    // Wheels
    j["wheelConfiguration"] = {
        { "wheelType", wheels.wheelType },
        { "wheelColor", wheels.wheelColor },
        { "tyreSmokeColor", {
            { "r", wheels.tyreSmokeColor.r },
            { "g", wheels.tyreSmokeColor.g },
            { "b", wheels.tyreSmokeColor.b }
        }},
        { "bulletproofTyres", wheels.bulletproofTyres },
        { "driftTyres", wheels.driftTyres },
        { "hasTyreSettings", wheels.hasTyreSettings }
    };

    // Paint
    j["paintConfiguration"] = {
        { "primaryColor", paint.primaryColor },
        { "secondaryColor", paint.secondaryColor },
        { "pearlescentColor", paint.pearlescentColor },
        { "isPrimaryColorCustom", paint.isPrimaryColorCustom },
        { "customPrimaryColor", {
            { "r", paint.customPrimaryColor.r },
            { "g", paint.customPrimaryColor.g },
            { "b", paint.customPrimaryColor.b }
        }},
        { "isSecondaryColorCustom", paint.isSecondaryColorCustom },
        { "customSecondaryColor", {
            { "r", paint.customSecondaryColor.r },
            { "g", paint.customSecondaryColor.g },
            { "b", paint.customSecondaryColor.b }
        }},
        { "windowTint", paint.windowTint },
        { "interiorColor", paint.interiorColor },
        { "dashBoardColor", paint.dashBoardColor }
    };

    // Neon
    j["neonConfiguration"] = {
        { "left", neon.left },
        { "right", neon.right },
        { "front", neon.front },
        { "back", neon.back },
        { "color", {
            { "r", neon.color.r },
            { "g", neon.color.g },
            { "b", neon.color.b }
        }},
        { "hasNeon", neon.hasNeon }
    };

    // Number Plate
    j["numberPlate"] = {
        { "text", plate.text },
        { "styleIndex", plate.styleIndex }
    };

    // Simulation Profile
    j["simulationProfile"] = {
        { "maxSpeed", simProfile.maxSpeed },
        { "maxAcceleration", simProfile.maxAcceleration },
        { "comfortableDecel", simProfile.comfortableDecel },
        { "maxDeceleration", simProfile.maxDeceleration },
        { "minGap", simProfile.minGap },
        { "dimensions", {
            { "length", simProfile.dimensions.x },
            { "width", simProfile.dimensions.y },
            { "height", simProfile.dimensions.z }
        }}
    };

    // Asset reference
    j["modelAssetReference"] = modelAssetPath;
    switch (assetStatus) {
        case ModelAssetStatus::Available:    j["assetStatus"] = "AVAILABLE"; break;
        case ModelAssetStatus::MissingModel: j["assetStatus"] = "MISSING_MODEL"; break;
        default:                             j["assetStatus"] = "NOT_AVAILABLE"; break;
    }

    // Validation
    switch (validationStatus) {
        case ValidationStatus::Valid:       j["validationStatus"] = "VALID"; break;
        case ValidationStatus::Warning:     j["validationStatus"] = "WARNING"; break;
        case ValidationStatus::Malformed:   j["validationStatus"] = "MALFORMED"; break;
        default:                            j["validationStatus"] = "UNSUPPORTED"; break;
    }
    j["validationWarnings"] = validationWarnings;
    j["validationErrors"] = validationErrors;

    if (!unknownProperties.empty()) {
        j["unknownProperties"] = unknownProperties;
    }

    return j;
}

VehiclePreset VehiclePreset::FromNormalizedJson(const nlohmann::json& j) {
    VehiclePreset p;
    p.id = j.value("id", "");
    p.displayName = j.value("displayName", "");
    p.modelIdentifier = j.value("modelIdentifier", "");
    p.modelHash = j.value("modelHash", 0u);
    p.sourceFile = j.value("sourceFile", "");
    p.sourceFormat = j.value("sourceFormat", "");
    p.sourceDirectory = j.value("sourceDirectory", "");
    p.category = StringToCategory(j.value("category", "Unknown"));
    p.baseFlags = j.value("baseFlags", 0u);
    p.contentHash = j.value("contentHash", "");

    if (j.contains("mods") && j["mods"].is_object()) {
        for (const auto& [k, v] : j["mods"].items()) {
            if (k.rfind("mod", 0) == 0) {
                int slot = std::stoi(k.substr(3));
                ModificationSlot ms;
                ms.index = v.value("index", -1);
                ms.variation = v.value("variation", false);
                ms.present = true;
                p.modifications[slot] = ms;
            }
        }
    }

    if (j.contains("extras") && j["extras"].is_object()) {
        p.hasExtras = true;
        for (const auto& [k, v] : j["extras"].items()) {
            if (k.rfind("extra", 0) == 0) {
                int slot = std::stoi(k.substr(5));
                p.extras[slot] = v.get<bool>();
            }
        }
    }

    if (j.contains("wheelConfiguration") && j["wheelConfiguration"].is_object()) {
        const auto& w = j["wheelConfiguration"];
        p.wheels.wheelType = w.value("wheelType", 0);
        p.wheels.wheelColor = w.value("wheelColor", 0);
        if (w.contains("tyreSmokeColor")) {
            p.wheels.tyreSmokeColor.r = w["tyreSmokeColor"].value("r", 255);
            p.wheels.tyreSmokeColor.g = w["tyreSmokeColor"].value("g", 255);
            p.wheels.tyreSmokeColor.b = w["tyreSmokeColor"].value("b", 255);
        }
        p.wheels.bulletproofTyres = w.value("bulletproofTyres", false);
        p.wheels.driftTyres = w.value("driftTyres", false);
        p.wheels.hasTyreSettings = w.value("hasTyreSettings", false);
    }

    if (j.contains("paintConfiguration") && j["paintConfiguration"].is_object()) {
        const auto& pt = j["paintConfiguration"];
        p.paint.primaryColor = pt.value("primaryColor", 0);
        p.paint.secondaryColor = pt.value("secondaryColor", 0);
        p.paint.pearlescentColor = pt.value("pearlescentColor", 0);
        p.paint.isPrimaryColorCustom = pt.value("isPrimaryColorCustom", false);
        if (pt.contains("customPrimaryColor")) {
            p.paint.customPrimaryColor.r = pt["customPrimaryColor"].value("r", 0);
            p.paint.customPrimaryColor.g = pt["customPrimaryColor"].value("g", 0);
            p.paint.customPrimaryColor.b = pt["customPrimaryColor"].value("b", 0);
        }
        p.paint.isSecondaryColorCustom = pt.value("isSecondaryColorCustom", false);
        if (pt.contains("customSecondaryColor")) {
            p.paint.customSecondaryColor.r = pt["customSecondaryColor"].value("r", 0);
            p.paint.customSecondaryColor.g = pt["customSecondaryColor"].value("g", 0);
            p.paint.customSecondaryColor.b = pt["customSecondaryColor"].value("b", 0);
        }
        p.paint.windowTint = pt.value("windowTint", 0);
        p.paint.interiorColor = pt.value("interiorColor", 0);
        p.paint.dashBoardColor = pt.value("dashBoardColor", 0);
    }

    if (j.contains("neonConfiguration") && j["neonConfiguration"].is_object()) {
        const auto& n = j["neonConfiguration"];
        p.neon.left = n.value("left", false);
        p.neon.right = n.value("right", false);
        p.neon.front = n.value("front", false);
        p.neon.back = n.value("back", false);
        p.neon.hasNeon = n.value("hasNeon", false);
        if (n.contains("color")) {
            p.neon.color.r = n["color"].value("r", 255);
            p.neon.color.g = n["color"].value("g", 255);
            p.neon.color.b = n["color"].value("b", 255);
        }
    }

    if (j.contains("numberPlate") && j["numberPlate"].is_object()) {
        p.plate.text = j["numberPlate"].value("text", "SMART3D");
        p.plate.styleIndex = j["numberPlate"].value("styleIndex", 0);
    }

    if (j.contains("simulationProfile") && j["simulationProfile"].is_object()) {
        const auto& sp = j["simulationProfile"];
        p.simProfile.maxSpeed = sp.value("maxSpeed", 15.0f);
        p.simProfile.maxAcceleration = sp.value("maxAcceleration", 3.0f);
        p.simProfile.comfortableDecel = sp.value("comfortableDecel", 2.5f);
        p.simProfile.maxDeceleration = sp.value("maxDeceleration", 8.0f);
        p.simProfile.minGap = sp.value("minGap", 2.0f);
        if (sp.contains("dimensions")) {
            p.simProfile.dimensions.x = sp["dimensions"].value("length", 4.6f);
            p.simProfile.dimensions.y = sp["dimensions"].value("width", 1.8f);
            p.simProfile.dimensions.z = sp["dimensions"].value("height", 1.4f);
        }
    }

    p.modelAssetPath = j.value("modelAssetReference", "");
    std::string assetStat = j.value("assetStatus", "MISSING_MODEL");
    if (assetStat == "AVAILABLE") p.assetStatus = ModelAssetStatus::Available;
    else if (assetStat == "MISSING_MODEL") p.assetStatus = ModelAssetStatus::MissingModel;
    else p.assetStatus = ModelAssetStatus::NotAvailable;

    std::string valStat = j.value("validationStatus", "VALID");
    if (valStat == "VALID") p.validationStatus = ValidationStatus::Valid;
    else if (valStat == "WARNING") p.validationStatus = ValidationStatus::Warning;
    else if (valStat == "MALFORMED") p.validationStatus = ValidationStatus::Malformed;
    else p.validationStatus = ValidationStatus::Unsupported;

    if (j.contains("unknownProperties")) {
        p.unknownProperties = j["unknownProperties"];
    }

    return p;
}

} // namespace SmartCity::vehicles

