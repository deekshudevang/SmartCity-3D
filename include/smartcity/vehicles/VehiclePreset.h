#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace SmartCity::vehicles {

enum class VehicleCategory {
    Supercar,
    Sports,
    Sedan,
    SUV,
    Bus,
    Emergency,
    Motorcycle,
    Unknown
};

enum class ValidationStatus {
    Valid,
    Warning,
    Malformed,
    Unsupported
};

enum class ModelAssetStatus {
    Available,
    MissingModel,
    NotAvailable
};

struct ModificationSlot {
    int index{ -1 };
    bool variation{ false };
    bool present{ false };
};

struct NeonConfiguration {
    bool left{ false };
    bool right{ false };
    bool front{ false };
    bool back{ false };
    glm::ivec3 color{ 255, 255, 255 };
    bool hasNeon{ false };
};

struct PaintConfiguration {
    int primaryColor{ 0 };
    int secondaryColor{ 0 };
    int pearlescentColor{ 0 };
    bool isPrimaryColorCustom{ false };
    glm::ivec3 customPrimaryColor{ 0, 0, 0 };
    bool isSecondaryColorCustom{ false };
    glm::ivec3 customSecondaryColor{ 0, 0, 0 };
    int windowTint{ 0 };
    int interiorColor{ 0 };
    int dashBoardColor{ 0 };
};

struct WheelConfiguration {
    int wheelType{ 0 };
    int wheelColor{ 0 };
    glm::ivec3 tyreSmokeColor{ 255, 255, 255 };
    bool bulletproofTyres{ false };
    bool driftTyres{ false };
    bool hasTyreSettings{ false };
};

struct NumberPlateConfiguration {
    std::string text{ "SMART3D" };
    int styleIndex{ 0 };
};

struct VehicleSimulationProfile {
    float maxSpeed{ 15.0f };       // m/s (~54 km/h)
    float maxAcceleration{ 3.0f }; // m/s^2
    float comfortableDecel{ 2.5f };// m/s^2
    float maxDeceleration{ 8.0f }; // m/s^2
    float minGap{ 2.0f };          // m
    glm::vec3 dimensions{ 4.6f, 1.8f, 1.4f }; // length, width, height (m)
    float axleZ{ 1.4f };           // distance from center to front/rear axles
    float halfTrackWidth{ 0.85f }; // half-width of wheel track
    float wheelRadius{ 0.35f };    // wheel radius
};

class VehiclePreset {
public:
    VehiclePreset();

    // Identity
    std::string id;                  // Unique identifier (hash + clean name)
    std::string displayName;         // User-facing display name
    std::string modelIdentifier;     // String representation of model hash
    uint32_t modelHash{ 0 };         // Raw 32-bit unsigned model hash
    std::string sourceFile;          // Relative path to source JSON
    std::string sourceFormat;        // E.g. "Cherax Entity"
    std::string sourceDirectory;     // Directory where source was located
    VehicleCategory category{ VehicleCategory::Unknown };
    uint32_t baseFlags{ 0 };
    std::string contentHash;         // Content hash for deduplication

    // Customization Configurations
    std::map<int, ModificationSlot> modifications; // mod0 .. mod49
    std::map<int, bool> extras;                   // extra0 .. extra15
    bool hasExtras{ false };
    WheelConfiguration wheels;
    PaintConfiguration paint;
    NeonConfiguration neon;
    NumberPlateConfiguration plate;

    // Asset & Simulation
    std::string modelAssetPath;
    ModelAssetStatus assetStatus{ ModelAssetStatus::MissingModel };
    VehicleSimulationProfile simProfile;

    // Validation & Extensibility
    ValidationStatus validationStatus{ ValidationStatus::Valid };
    std::vector<std::string> validationWarnings;
    std::vector<std::string> validationErrors;
    nlohmann::json unknownProperties; // Preserves unmodeled fields (e.g. attachments)
    nlohmann::json rawSourceJson;     // Exact original JSON

    // Helper queries
    bool IsModelAvailable() const { return assetStatus == ModelAssetStatus::Available; }
    bool IsValid() const { return validationStatus == ValidationStatus::Valid || validationStatus == ValidationStatus::Warning; }
    glm::vec3 GetEffectivePrimaryColorRgb() const;
    glm::vec3 GetEffectiveSecondaryColorRgb() const;
    std::string GetCategoryName() const;
    static std::string CategoryToString(VehicleCategory cat);
    static VehicleCategory StringToCategory(const std::string& str);

    // Serialization to normalized JSON
    nlohmann::json ToNormalizedJson() const;
    static VehiclePreset FromNormalizedJson(const nlohmann::json& j);
};

} // namespace SmartCity::vehicles

