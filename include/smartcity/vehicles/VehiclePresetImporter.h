#pragma once

#include "smartcity/vehicles/VehiclePreset.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace SmartCity::vehicles {

struct FileImportResult {
    std::string relativePath;
    std::string filename;
    std::string format;
    uint32_t modelHash{ 0 };
    std::string displayName;
    bool isSuccess{ false };
    bool isDuplicate{ false };
    bool isUnsupported{ false };
    bool isMalformed{ false };
    std::string statusMessage;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::string contentHash;
    VehiclePreset preset;
};

struct ImportReport {
    int filesDiscovered{ 0 };
    int validConfigurations{ 0 };
    int warningConfigurations{ 0 };
    int unsupportedFiles{ 0 };
    int malformedFiles{ 0 };
    int duplicateConfigurations{ 0 };
    int successfullyNormalized{ 0 };
    int modelsWithAssets{ 0 };
    int modelsWithMissingAssets{ 0 };
    std::vector<std::string> unsupportedPropertiesEncountered;
    std::vector<FileImportResult> fileResults;

    nlohmann::json ToJson() const;
    void SaveToFile(const std::string& path) const;
};

class VehiclePresetImporter {
public:
    VehiclePresetImporter() = default;

    // Scan a directory recursively and import all discovered vehicle JSON files
    ImportReport ImportDirectory(const std::string& directoryPath,
                                 const std::string& normalizedOutputDir = "");

    // Import a single file directly
    FileImportResult ImportFile(const std::string& filePath);

    // Parse and validate from in-memory string (for unit tests)
    FileImportResult ImportFromString(const std::string& jsonString,
                                     const std::string& sourceFilename = "memory.json");

    // Clear internal deduplication caches
    void Reset();

private:
    // Schema detection and parsers
    bool ParseCheraxEntity(const nlohmann::json& j, const std::string& filename,
                           const std::string& path, const std::string& contentHash,
                           FileImportResult& outResult);

    bool ParseGenericVehicleSchema(const nlohmann::json& j, const std::string& filename,
                                   const std::string& path, const std::string& contentHash,
                                   FileImportResult& outResult);

    // Compute standard FNV-1a 64-bit content hash string
    static std::string ComputeContentHash(const std::string& content);

    // Deduplication tracking: contentHash -> first seen relative path
    std::unordered_map<std::string, std::string> m_seenContentHashes;
    std::unordered_set<std::string> m_unsupportedPropertiesGlobal;
};

} // namespace SmartCity::vehicles

