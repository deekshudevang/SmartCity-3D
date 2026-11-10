#include "smartcity/vehicles/VehiclePresetImporter.h"
#include "core/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace SmartCity::vehicles {

namespace fs = std::filesystem;

// FNV-1a 64-bit content hash
std::string VehiclePresetImporter::ComputeContentHash(const std::string& content) {
    uint64_t hash = 14695981039346656037ull;
    for (char c : content) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ull;
    }
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

void VehiclePresetImporter::Reset() {
    m_seenContentHashes.clear();
    m_unsupportedPropertiesGlobal.clear();
}

FileImportResult VehiclePresetImporter::ImportFile(const std::string& filePath) {
    FileImportResult res;
    res.relativePath = filePath;

    std::error_code ec;
    fs::path p(filePath);
    res.filename = p.filename().string();

    if (!fs::exists(p, ec) || !fs::is_regular_file(p, ec)) {
        res.isMalformed = true;
        res.statusMessage = "File does not exist or is not a regular file";
        res.errors.push_back(res.statusMessage);
        return res;
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        res.isMalformed = true;
        res.statusMessage = "Could not open file for reading";
        res.errors.push_back(res.statusMessage);
        return res;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return ImportFromString(content, res.filename);
}

FileImportResult VehiclePresetImporter::ImportFromString(const std::string& jsonString,
                                                        const std::string& sourceFilename) {
    FileImportResult res;
    res.filename = sourceFilename;
    res.relativePath = sourceFilename;

    if (jsonString.empty()) {
        res.isMalformed = true;
        res.statusMessage = "Empty JSON file";
        res.errors.push_back("File is 0 bytes or empty");
        return res;
    }

    // Compute content hash for duplicate detection
    res.contentHash = ComputeContentHash(jsonString);
    auto it = m_seenContentHashes.find(res.contentHash);
    if (it != m_seenContentHashes.end()) {
        res.isDuplicate = true;
        res.warnings.push_back("Duplicate configuration content identical to: " + it->second);
    } else {
        m_seenContentHashes[res.contentHash] = sourceFilename;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(jsonString);
    } catch (const nlohmann::json::exception& e) {
        res.isMalformed = true;
        res.statusMessage = std::string("JSON parse error: ") + e.what();
        res.errors.push_back(res.statusMessage);
        return res;
    }

    if (!j.is_object()) {
        res.isMalformed = true;
        res.statusMessage = "Root JSON element is not an object";
        res.errors.push_back(res.statusMessage);
        return res;
    }

    // Inspect format
    std::string format = j.value("format", "");
    res.format = format;

    if (format == "Cherax Entity") {
        if (ParseCheraxEntity(j, sourceFilename, sourceFilename, res.contentHash, res)) {
            res.isSuccess = true;
            res.statusMessage = "Successfully imported Cherax Entity";
        }
    } else {
        // Attempt generic vehicle schema detection
        if (ParseGenericVehicleSchema(j, sourceFilename, sourceFilename, res.contentHash, res)) {
            res.isSuccess = true;
            res.statusMessage = "Successfully imported generic vehicle schema";
        } else {
            res.isUnsupported = true;
            res.statusMessage = "Unsupported or unrecognized vehicle configuration format";
            res.errors.push_back("Format '" + format + "' is not recognized as a valid vehicle configuration");
        }
    }

    return res;
}

static std::string CleanDisplayName(const std::string& filename) {
    std::string s = filename;
    if (s.size() > 5 && s.substr(s.size() - 5) == ".json") {
        s = s.substr(0, s.size() - 5);
    }
    // Replace underscores with spaces
    std::replace(s.begin(), s.end(), '_', ' ');
    // Remove duplicate spaces
    std::string clean;
    bool prevSpace = false;
    for (char c : s) {
        if (c == ' ') {
            if (!prevSpace) clean += c;
            prevSpace = true;
        } else {
            clean += c;
            prevSpace = false;
        }
    }
    return clean;
}

bool VehiclePresetImporter::ParseCheraxEntity(const nlohmann::json& j, const std::string& filename,
                                             const std::string& path, const std::string& contentHash,
                                             FileImportResult& outResult) {
    VehiclePreset preset;
    preset.sourceFile = path;
    preset.sourceDirectory = fs::path(path).parent_path().string();
    preset.sourceFormat = "Cherax Entity";
    preset.contentHash = contentHash;
    preset.rawSourceJson = j;

    // Check entity type (1 = Vehicle)
    int type = j.value("type", -1);
    if (type != 1) {
        outResult.isUnsupported = true;
        outResult.statusMessage = "Entity type is not a vehicle (type != 1)";
        outResult.errors.push_back("Expected entity type 1 (Vehicle), but found type " + std::to_string(type));
        return false;
    }

    // Extract Model
    if (!j.contains("model")) {
        outResult.isMalformed = true;
        outResult.errors.push_back("Missing required 'model' field");
        return false;
    }

    if (j["model"].is_number()) {
        preset.modelHash = j["model"].get<uint32_t>();
        preset.modelIdentifier = std::to_string(preset.modelHash);
    } else if (j["model"].is_string()) {
        preset.modelIdentifier = j["model"].get<std::string>();
        try {
            preset.modelHash = static_cast<uint32_t>(std::stoul(preset.modelIdentifier));
        } catch (...) {
            preset.modelHash = 0;
            outResult.warnings.push_back("Model string could not be parsed as unsigned integer hash: " + preset.modelIdentifier);
        }
    } else {
        outResult.isMalformed = true;
        outResult.errors.push_back("'model' field is neither number nor string");
        return false;
    }

    outResult.modelHash = preset.modelHash;

    // Extract Base Flags
    preset.baseFlags = j.value("baseFlags", 0u);

    // Clean display name
    preset.displayName = CleanDisplayName(filename);
    outResult.displayName = preset.displayName;
    preset.id = std::to_string(preset.modelHash) + "_" + preset.displayName;
    // Replace non-alphanumeric chars in ID
    for (char& c : preset.id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            c = '_';
        }
    }

    // 1. Modifications (mod0 .. mod49)
    if (j.contains("mods") && j["mods"].is_object()) {
        for (const auto& [k, v] : j["mods"].items()) {
            if (k.rfind("mod", 0) == 0 && v.is_object()) {
                try {
                    int slot = std::stoi(k.substr(3));
                    ModificationSlot ms;
                    ms.index = v.value("index", -1);
                    ms.variation = v.value("variation", false);
                    ms.present = true;
                    preset.modifications[slot] = ms;
                } catch (...) {
                    outResult.warnings.push_back("Invalid modification key: " + k);
                }
            }
        }
    } else {
        outResult.warnings.push_back("Missing or invalid 'mods' object");
    }

    // 2. Extras (extra0 .. extra15)
    if (j.contains("extras") && j["extras"].is_object()) {
        preset.hasExtras = true;
        for (const auto& [k, v] : j["extras"].items()) {
            if (k.rfind("extra", 0) == 0 && v.is_boolean()) {
                try {
                    int slot = std::stoi(k.substr(5));
                    preset.extras[slot] = v.get<bool>();
                } catch (...) {
                    outResult.warnings.push_back("Invalid extra key: " + k);
                }
            }
        }
    } else {
        preset.hasExtras = false;
        // Non-fatal warning
    }

    // 3. Wheels
    preset.wheels.wheelType = j.value("wheelType", 0);
    preset.wheels.wheelColor = j.value("wheelColor", 0);
    if (j.contains("tyreSmokeColor") && j["tyreSmokeColor"].is_object()) {
        preset.wheels.tyreSmokeColor.r = j["tyreSmokeColor"].value("r", 255);
        preset.wheels.tyreSmokeColor.g = j["tyreSmokeColor"].value("g", 255);
        preset.wheels.tyreSmokeColor.b = j["tyreSmokeColor"].value("b", 255);
    }
    if (j.contains("bulletproofTyres")) {
        preset.wheels.bulletproofTyres = j.value("bulletproofTyres", false);
        preset.wheels.hasTyreSettings = true;
    }
    if (j.contains("driftTyres")) {
        preset.wheels.driftTyres = j.value("driftTyres", false);
        preset.wheels.hasTyreSettings = true;
    }

    // 4. Paint
    preset.paint.primaryColor = j.value("primaryColor", 0);
    preset.paint.secondaryColor = j.value("secondaryColor", 0);
    preset.paint.pearlescentColor = j.value("pearlescentColor", 0);
    preset.paint.isPrimaryColorCustom = j.value("isPrimaryColorCustom", false);
    if (j.contains("customPrimaryColor") && j["customPrimaryColor"].is_object()) {
        preset.paint.customPrimaryColor.r = j["customPrimaryColor"].value("r", 0);
        preset.paint.customPrimaryColor.g = j["customPrimaryColor"].value("g", 0);
        preset.paint.customPrimaryColor.b = j["customPrimaryColor"].value("b", 0);
    }
    preset.paint.isSecondaryColorCustom = j.value("isSecondaryColorCustom", false);
    if (j.contains("customSecondaryColor") && j["customSecondaryColor"].is_object()) {
        preset.paint.customSecondaryColor.r = j["customSecondaryColor"].value("r", 0);
        preset.paint.customSecondaryColor.g = j["customSecondaryColor"].value("g", 0);
        preset.paint.customSecondaryColor.b = j["customSecondaryColor"].value("b", 0);
    }
    preset.paint.windowTint = j.value("windowTint", 0);
    preset.paint.interiorColor = j.value("interiorColor", 0);
    preset.paint.dashBoardColor = j.value("dashBoardColor", 0);

    // 5. Neon
    if (j.contains("neonKits") && j["neonKits"].is_object()) {
        preset.neon.hasNeon = true;
        preset.neon.left = j["neonKits"].value("left", false);
        preset.neon.right = j["neonKits"].value("right", false);
        preset.neon.front = j["neonKits"].value("front", false);
        preset.neon.back = j["neonKits"].value("back", false);
        if (j["neonKits"].contains("color") && j["neonKits"]["color"].is_object()) {
            preset.neon.color.r = j["neonKits"]["color"].value("r", 255);
            preset.neon.color.g = j["neonKits"]["color"].value("g", 255);
            preset.neon.color.b = j["neonKits"]["color"].value("b", 255);
        }
    } else {
        preset.neon.hasNeon = false;
    }

    // 6. Number Plate
    preset.plate.text = j.value("plateText", "SMART3D");
    preset.plate.styleIndex = j.value("plateTextIndex", 0);

    // 7. Unknown / Extended properties preservation
    const std::vector<std::string> knownKeys = {
        "format", "type", "model", "baseFlags", "mods", "extras",
        "wheelType", "wheelColor", "tyreSmokeColor", "bulletproofTyres", "driftTyres",
        "primaryColor", "secondaryColor", "pearlescentColor",
        "isPrimaryColorCustom", "customPrimaryColor",
        "isSecondaryColorCustom", "customSecondaryColor",
        "neonKits", "windowTint", "interiorColor", "dashBoardColor",
        "plateText", "plateTextIndex"
    };

    for (const auto& [k, v] : j.items()) {
        if (std::find(knownKeys.begin(), knownKeys.end(), k) == knownKeys.end()) {
            preset.unknownProperties[k] = v;
            m_unsupportedPropertiesGlobal.insert(k);
            outResult.warnings.push_back("Preserving unknown property: " + k);
        }
    }

    preset.validationStatus = outResult.warnings.empty() ? ValidationStatus::Valid : ValidationStatus::Warning;
    preset.validationWarnings = outResult.warnings;
    preset.validationErrors = outResult.errors;

    outResult.preset = preset;
    return true;
}

bool VehiclePresetImporter::ParseGenericVehicleSchema(const nlohmann::json& j, const std::string& filename,
                                                    const std::string& path, const std::string& contentHash,
                                                    FileImportResult& outResult) {
    // Check if it possesses minimal vehicle attributes
    if (!j.contains("model") && !j.contains("modelHash") && !j.contains("modelIdentifier")) {
        return false;
    }

    VehiclePreset preset;
    preset.sourceFile = path;
    preset.sourceDirectory = fs::path(path).parent_path().string();
    preset.sourceFormat = "Generic Vehicle";
    preset.contentHash = contentHash;
    preset.rawSourceJson = j;

    if (j.contains("modelHash")) {
        preset.modelHash = j["modelHash"].get<uint32_t>();
    } else if (j.contains("model")) {
        if (j["model"].is_number()) preset.modelHash = j["model"].get<uint32_t>();
        else if (j["model"].is_string()) preset.modelHash = static_cast<uint32_t>(std::stoul(j["model"].get<std::string>()));
    }
    preset.modelIdentifier = std::to_string(preset.modelHash);
    preset.displayName = j.value("displayName", CleanDisplayName(filename));
    preset.id = std::to_string(preset.modelHash) + "_" + preset.displayName;

    outResult.modelHash = preset.modelHash;
    outResult.displayName = preset.displayName;
    outResult.preset = preset;
    return true;
}

ImportReport VehiclePresetImporter::ImportDirectory(const std::string& directoryPath,
                                                    const std::string& normalizedOutputDir) {
    ImportReport report;
    std::error_code ec;

    if (!fs::exists(directoryPath, ec) || !fs::is_directory(directoryPath, ec)) {
        core::Logger::Error("VehiclePresetImporter: Directory does not exist: ", directoryPath);
        return report;
    }

    core::Logger::Info("VehiclePresetImporter: Recursively scanning: ", directoryPath);

    // Recursively collect all .json files
    std::vector<fs::path> jsonPaths;
    for (const auto& entry : fs::recursive_directory_iterator(directoryPath, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            jsonPaths.push_back(entry.path());
        }
    }

    report.filesDiscovered = static_cast<int>(jsonPaths.size());
    core::Logger::Info("VehiclePresetImporter: Discovered ", report.filesDiscovered, " JSON candidate files.");

    // Sort paths for deterministic reproducible order
    std::sort(jsonPaths.begin(), jsonPaths.end());

    for (const auto& p : jsonPaths) {
        FileImportResult res = ImportFile(p.string());
        report.fileResults.push_back(res);

        if (res.isDuplicate) {
            report.duplicateConfigurations++;
        }

        if (res.isMalformed) {
            report.malformedFiles++;
        } else if (res.isUnsupported) {
            report.unsupportedFiles++;
        } else if (res.isSuccess) {
            if (res.warnings.empty()) {
                report.validConfigurations++;
            } else {
                report.warningConfigurations++;
            }
            report.successfullyNormalized++;

            // Optionally write normalized JSON
            if (!normalizedOutputDir.empty()) {
                fs::create_directories(normalizedOutputDir, ec);
                std::string normFilename = res.preset.id + ".json";
                fs::path outPath = fs::path(normalizedOutputDir) / normFilename;
                std::ofstream outFile(outPath);
                if (outFile.is_open()) {
                    outFile << res.preset.ToNormalizedJson().dump(4);
                    outFile.close();
                }
            }
        }
    }

    for (const auto& prop : m_unsupportedPropertiesGlobal) {
        report.unsupportedPropertiesEncountered.push_back(prop);
    }
    std::sort(report.unsupportedPropertiesEncountered.begin(), report.unsupportedPropertiesEncountered.end());

    core::Logger::Info("VehiclePresetImporter: Finished. Valid: ", report.validConfigurations,
                       ", Warnings: ", report.warningConfigurations,
                       ", Duplicates: ", report.duplicateConfigurations,
                       ", Malformed: ", report.malformedFiles,
                       ", Unsupported: ", report.unsupportedFiles);

    return report;
}

nlohmann::json ImportReport::ToJson() const {
    nlohmann::json j;
    j["summary"] = {
        { "filesDiscovered", filesDiscovered },
        { "validConfigurations", validConfigurations },
        { "warningConfigurations", warningConfigurations },
        { "unsupportedFiles", unsupportedFiles },
        { "malformedFiles", malformedFiles },
        { "duplicateConfigurations", duplicateConfigurations },
        { "successfullyNormalized", successfullyNormalized },
        { "modelsWithAssets", modelsWithAssets },
        { "modelsWithMissingAssets", modelsWithMissingAssets }
    };

    j["unsupportedPropertiesEncountered"] = unsupportedPropertiesEncountered;

    nlohmann::json filesArr = nlohmann::json::array();
    for (const auto& r : fileResults) {
        nlohmann::json fj;
        fj["filename"] = r.filename;
        fj["relativePath"] = r.relativePath;
        fj["format"] = r.format;
        fj["modelHash"] = r.modelHash;
        fj["displayName"] = r.displayName;
        fj["isSuccess"] = r.isSuccess;
        fj["isDuplicate"] = r.isDuplicate;
        fj["isMalformed"] = r.isMalformed;
        fj["isUnsupported"] = r.isUnsupported;
        fj["statusMessage"] = r.statusMessage;
        fj["warnings"] = r.warnings;
        fj["errors"] = r.errors;
        fj["contentHash"] = r.contentHash;
        filesArr.push_back(fj);
    }
    j["files"] = filesArr;

    return j;
}

void ImportReport::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (f.is_open()) {
        f << ToJson().dump(4);
        f.close();
        core::Logger::Info("VehiclePresetImporter: Saved import report to ", path);
    } else {
        core::Logger::Error("VehiclePresetImporter: Could not save report to ", path);
    }
}

} // namespace SmartCity::vehicles

