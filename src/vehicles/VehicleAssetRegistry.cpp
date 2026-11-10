#include "smartcity/vehicles/VehicleAssetRegistry.h"
#include "core/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace SmartCity::vehicles {

namespace fs = std::filesystem;

VehicleAssetRegistry& VehicleAssetRegistry::Get() {
    static VehicleAssetRegistry s_instance;
    return s_instance;
}

VehicleAssetRegistry::VehicleAssetRegistry() {
    PopulateDefaultKnowledgeBase();
}

bool VehicleAssetRegistry::VerifyAssetExists(const std::string& assetPath) {
    if (assetPath.empty()) return false;
    std::error_code ec;
    fs::path p(assetPath);
    if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) {
        return true;
    }
    // Check fallback relative to working dir or ../
    fs::path pAlt = fs::path("../") / assetPath;
    return fs::exists(pAlt, ec) && fs::is_regular_file(pAlt, ec);
}

void VehicleAssetRegistry::RegisterAsset(const VehicleAssetEntry& entry) {
    m_assets[entry.modelHash] = entry;
    // Check real file existence
    if (!entry.modelAssetPath.empty() && VerifyAssetExists(entry.modelAssetPath)) {
        m_assets[entry.modelHash].assetStatus = ModelAssetStatus::Available;
    } else {
        m_assets[entry.modelHash].assetStatus = ModelAssetStatus::MissingModel;
    }
}

const VehicleAssetEntry* VehicleAssetRegistry::FindAsset(uint32_t modelHash) const {
    auto it = m_assets.find(modelHash);
    if (it != m_assets.end()) {
        return &it->second;
    }
    return nullptr;
}

void VehicleAssetRegistry::ResolvePresetAssets(VehiclePreset& preset) const {
    const auto* asset = FindAsset(preset.modelHash);
    if (asset) {
        if (preset.category == VehicleCategory::Unknown) {
            preset.category = asset->category;
        }
        if (preset.displayName.empty() || preset.displayName.find("json") != std::string::npos) {
            preset.displayName = asset->displayName;
        }
        preset.modelAssetPath = asset->modelAssetPath;
        preset.assetStatus = asset->assetStatus;
        preset.simProfile = asset->simProfile;
    } else {
        // Fallback default profile based on model hash / filename
        if (preset.category == VehicleCategory::Unknown) {
            preset.category = VehicleCategory::Supercar;
        }
        preset.assetStatus = ModelAssetStatus::MissingModel;
        preset.simProfile = VehicleSimulationProfile{}; // Default sedan/sports
    }
}

bool VehicleAssetRegistry::LoadManifest(const std::string& manifestPath) {
    m_manifestPath = manifestPath;
    std::error_code ec;
    std::string path = manifestPath;

    if (!fs::exists(path, ec)) {
        path = "../" + manifestPath;
        if (!fs::exists(path, ec)) {
            core::Logger::Warn("VehicleAssetRegistry: Manifest file not found: ", manifestPath, ". Using built-in database.");
            return false;
        }
    }

    std::ifstream f(path);
    if (!f.is_open()) return false;

    try {
        nlohmann::json j;
        f >> j;
        f.close();

        if (j.contains("vehicles") && j["vehicles"].is_array()) {
            for (const auto& item : j["vehicles"]) {
                VehicleAssetEntry entry;
                std::string hashStr = item.value("modelHash", "0");
                try {
                    entry.modelHash = static_cast<uint32_t>(std::stoul(hashStr));
                } catch (...) {
                    entry.modelHash = 0;
                }
                entry.displayName = item.value("displayName", "");
                entry.category = VehiclePreset::StringToCategory(item.value("category", "supercar"));
                entry.modelAssetPath = item.value("modelAsset", "");
                
                if (VerifyAssetExists(entry.modelAssetPath)) {
                    entry.assetStatus = ModelAssetStatus::Available;
                } else {
                    entry.assetStatus = ModelAssetStatus::MissingModel;
                }

                if (item.contains("collisionDimensions") && item["collisionDimensions"].is_object()) {
                    entry.simProfile.dimensions.x = item["collisionDimensions"].value("length", 4.6f);
                    entry.simProfile.dimensions.y = item["collisionDimensions"].value("width", 1.8f);
                    entry.simProfile.dimensions.z = item["collisionDimensions"].value("height", 1.4f);
                }
                if (item.contains("simulationProfile") && item["simulationProfile"].is_object()) {
                    entry.simProfile.maxSpeed = item["simulationProfile"].value("maxSpeed", 18.0f);
                    entry.simProfile.maxAcceleration = item["simulationProfile"].value("maxAcceleration", 4.0f);
                    entry.simProfile.comfortableDecel = item["simulationProfile"].value("comfortableDecel", 3.0f);
                    entry.simProfile.maxDeceleration = item["simulationProfile"].value("maxDeceleration", 9.0f);
                    entry.simProfile.minGap = item["simulationProfile"].value("minGap", 2.0f);
                }

                m_assets[entry.modelHash] = entry;
            }
        }
        core::Logger::Info("VehicleAssetRegistry: Loaded ", m_assets.size(), " asset mappings from manifest.");
        return true;
    } catch (const std::exception& e) {
        core::Logger::Error("VehicleAssetRegistry: Error parsing manifest: ", e.what());
        return false;
    }
}

bool VehicleAssetRegistry::SaveManifest(const std::string& manifestPath) const {
    nlohmann::json j;
    j["schemaVersion"] = "1.0.0";
    j["description"] = "SmartCity 3D Vehicle 3D Asset and Simulation Profile Registry";

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& [hash, asset] : m_assets) {
        nlohmann::json item;
        item["modelHash"] = std::to_string(hash);
        item["displayName"] = asset.displayName;
        item["category"] = VehiclePreset::CategoryToString(asset.category);
        item["modelAsset"] = asset.modelAssetPath;
        item["assetStatus"] = (asset.assetStatus == ModelAssetStatus::Available) ? "AVAILABLE" : "MISSING_MODEL";
        item["collisionDimensions"] = {
            { "length", asset.simProfile.dimensions.x },
            { "width", asset.simProfile.dimensions.y },
            { "height", asset.simProfile.dimensions.z }
        };
        item["simulationProfile"] = {
            { "maxSpeed", asset.simProfile.maxSpeed },
            { "maxAcceleration", asset.simProfile.maxAcceleration },
            { "comfortableDecel", asset.simProfile.comfortableDecel },
            { "maxDeceleration", asset.simProfile.maxDeceleration },
            { "minGap", asset.simProfile.minGap }
        };
        arr.push_back(item);
    }
    j["vehicles"] = arr;

    std::ofstream f(manifestPath);
    if (!f.is_open()) return false;
    f << j.dump(4);
    f.close();
    core::Logger::Info("VehicleAssetRegistry: Saved asset manifest to ", manifestPath);
    return true;
}

void VehicleAssetRegistry::PopulateDefaultKnowledgeBase() {
    // Knowledge base for GTA / Cherax model hashes discovered in source JSONs
    struct ModelKB {
        uint32_t hash;
        const char* name;
        VehicleCategory cat;
        const char* subDir;
        const char* assetFile;
        float length;
        float width;
        float height;
        float maxSpeed;
    };

    static const ModelKB s_kb[] = {
        { 3356898886u, "Progen Luiva",                 VehicleCategory::Supercar,   "supercars",   "progen_luiva.glb",        4.65f, 1.95f, 1.20f, 22.0f },
        { 274946574u,  "Obey 10F",                     VehicleCategory::Sports,     "sports",      "obey_10f.glb",            4.50f, 1.90f, 1.25f, 20.0f },
        { 661493923u,  "Pfister Comet S2",             VehicleCategory::Sports,     "sports",      "pfister_comet_s2.glb",    4.55f, 1.88f, 1.28f, 20.5f },
        { 3050505892u, "Obey 8F Drafter",              VehicleCategory::Sedan,      "sedans",      "obey_8f_drafter.glb",     4.75f, 1.85f, 1.40f, 18.0f },
        { 1748565021u, "Overflod Jesko Absolut",       VehicleCategory::Supercar,   "supercars",   "overflod_jesko.glb",      4.70f, 2.02f, 1.18f, 25.0f },
        { 4080061290u, "Karin Calico GTF",             VehicleCategory::Sports,     "sports",      "karin_calico_gtf.glb",    4.40f, 1.80f, 1.30f, 19.0f },
        { 4017955470u, "Pegassi Zonda 760",            VehicleCategory::Supercar,   "supercars",   "pegassi_zonda_760.glb",   4.60f, 2.00f, 1.15f, 24.0f },
        { 3316510914u, "Annis ZR380 Veleno GT",        VehicleCategory::Sports,     "sports",      "annis_zr380.glb",         4.60f, 1.92f, 1.32f, 20.0f },
        { 2712905841u, "Pegassi Torero XO",            VehicleCategory::Supercar,   "supercars",   "pegassi_torero_xo.glb",   4.62f, 2.05f, 1.16f, 23.0f },
        { 1923534526u, "Annis ZR350 RX7",              VehicleCategory::Sports,     "sports",      "annis_zr350.glb",         4.35f, 1.78f, 1.25f, 19.5f },
        { 662793086u,  "Benefactor Schafter Benny",    VehicleCategory::Sedan,      "sedans",      "benefactor_schafter.glb", 4.90f, 1.86f, 1.44f, 17.5f },
        { 3634959571u, "Bravado Banshee GTS",          VehicleCategory::Sports,     "sports",      "bravado_banshee_gts.glb", 4.45f, 1.95f, 1.22f, 21.0f },
        { 1494708164u, "Benefactor LRC GT",            VehicleCategory::Supercar,   "supercars",   "benefactor_lrc_gt.glb",   4.68f, 2.00f, 1.21f, 22.5f },
        { 3656405053u, "Declasse Scramjet",            VehicleCategory::Supercar,   "supercars",   "declasse_scramjet.glb",   4.55f, 1.90f, 1.15f, 23.0f },
        { 3703315515u, "Dinka Blista F1",              VehicleCategory::Sports,     "sports",      "dinka_blista_f1.glb",     4.10f, 1.75f, 1.35f, 18.0f },
        { 3105951696u, "Vapid Blue Collar Rig",        VehicleCategory::SUV,        "suvs",        "vapid_blue_collar.glb",   5.50f, 2.10f, 1.95f, 15.0f },
        { 2709293299u, "Ubermacht Sentinel GTS / M3",  VehicleCategory::Sports,     "sports",      "ubermacht_sentinel.glb",  4.70f, 1.88f, 1.42f, 19.0f },
        { 2069146067u, "Pegassi Broomstick Oppressor", VehicleCategory::Motorcycle, "motorcycles", "pegassi_oppressor.glb",  2.80f, 1.10f, 1.20f, 24.0f },
        { 127317925u,  "Invetero Coquette C6",         VehicleCategory::Sports,     "sports",      "invetero_coquette_c6.glb",4.52f, 1.91f, 1.24f, 20.0f },
        { 2271995207u, "Grotti Cartuccia GT",          VehicleCategory::Supercar,   "supercars",   "grotti_cartuccia_gt.glb", 4.65f, 1.98f, 1.19f, 22.5f },
        { 386089410u,  "Coil Raiden 2",                VehicleCategory::Sedan,      "sedans",      "coil_raiden_2.glb",       4.95f, 1.94f, 1.41f, 21.0f },
        { 2568944644u, "Pfister Comet S2 Turbo",       VehicleCategory::Sports,     "sports",      "pfister_comet_s2_t.glb",  4.55f, 1.88f, 1.28f, 20.5f },
        { 2362851406u, "Enus Defendybob",              VehicleCategory::SUV,        "suvs",        "enus_defendybob.glb",     5.10f, 2.02f, 1.78f, 17.0f },
        { 196747873u,  "Annis Elegy Retro Custom",     VehicleCategory::Sports,     "sports",      "annis_elegy_retro.glb",   4.60f, 1.82f, 1.34f, 20.0f },
        { 2038480341u, "Annis Euros",                  VehicleCategory::Sports,     "sports",      "annis_euros.glb",         4.40f, 1.85f, 1.30f, 19.5f },
        { 1353120668u, "Western Fast Bike",            VehicleCategory::Motorcycle, "motorcycles", "western_fast_bike.glb",   2.20f, 0.85f, 1.15f, 22.0f },
        { 4039289119u, "Shitzu Hakuchou Drag",         VehicleCategory::Motorcycle, "motorcycles", "shitzu_hakuchou_drag.glb",2.40f, 0.85f, 1.12f, 24.0f },
        { 3145241962u, "Grotti Itali RSX",             VehicleCategory::Supercar,   "supercars",   "grotti_itali_rsx.glb",    4.66f, 1.97f, 1.18f, 23.5f },
        { 409049982u,  "Dinka Jap Kanjo",              VehicleCategory::Sports,     "sports",      "dinka_kanjo.glb",         4.05f, 1.72f, 1.35f, 17.5f },
        { 4003946083u, "Karin Sultan RS Classic",      VehicleCategory::Sports,     "sports",      "karin_sultan_rs.glb",     4.50f, 1.82f, 1.38f, 19.0f },
        { 3540279623u, "Lampadati Corsita",            VehicleCategory::Supercar,   "supercars",   "lampadati_corsita.glb",   4.68f, 1.96f, 1.20f, 22.0f },
        { 223258115u,  "Albany Low Rider",             VehicleCategory::Sedan,      "sedans",      "albany_low_rider.glb",    5.20f, 1.90f, 1.35f, 16.0f },
        { 1484920335u, "Dinka LSCM Jester RR",         VehicleCategory::Sports,     "sports",      "dinka_jester_rr.glb",     4.48f, 1.89f, 1.29f, 20.0f },
        { 1755697647u, "Ubermacht M2 Previon",         VehicleCategory::Sports,     "sports",      "ubermacht_m2.glb",        4.45f, 1.85f, 1.36f, 19.5f },
        { 3787471536u, "Benefactor Schlagen GT",       VehicleCategory::Sports,     "sports",      "benefactor_schlagen.glb", 4.60f, 1.98f, 1.28f, 21.0f },
        { 3348919626u, "Enus Paragon S",               VehicleCategory::Sports,     "sports",      "enus_paragon_s.glb",      4.85f, 1.95f, 1.38f, 20.5f },
        { 1304459735u, "Pfister Growler Oak Green",    VehicleCategory::Sports,     "sports",      "pfister_growler.glb",     4.45f, 1.84f, 1.28f, 20.0f },
        { 687627128u,  "Karin Pinki Suzume",           VehicleCategory::Sports,     "sports",      "karin_suzume.glb",        4.30f, 1.76f, 1.32f, 18.5f },
        { 4071505793u, "Overflod Pipistrello",         VehicleCategory::Supercar,   "supercars",   "overflod_pipistrello.glb",4.65f, 2.01f, 1.17f, 23.0f },
        { 2515968713u, "Pfister Porsche X-Treme",      VehicleCategory::Supercar,   "supercars",   "pfister_xtreme.glb",      4.58f, 1.95f, 1.22f, 22.0f },
        { 2272483501u, "Pfister Purple Porsche 911",   VehicleCategory::Sports,     "sports",      "pfister_911_purple.glb",  4.52f, 1.87f, 1.29f, 20.0f },
        { 4255883207u, "Pegassi Pursuit Ignus",        VehicleCategory::Supercar,   "supercars",   "pegassi_ignus.glb",       4.78f, 2.08f, 1.14f, 24.5f },
        { 1074745671u, "Dewbauchee Specter Classic",   VehicleCategory::Sports,     "sports",      "dewbauchee_specter.glb",  4.65f, 1.92f, 1.30f, 20.5f },
        { 1663218586u, "Progen T20",                   VehicleCategory::Supercar,   "supercars",   "progen_t20.glb",          4.55f, 1.98f, 1.18f, 23.0f },
        { 1031562256u, "Pegassi Tezeract",             VehicleCategory::Supercar,   "supercars",   "pegassi_tezeract.glb",    4.72f, 2.06f, 1.12f, 24.0f },
        { 3126015148u, "Pegassi Toros Urus",           VehicleCategory::SUV,        "suvs",        "pegassi_toros.glb",       5.05f, 2.01f, 1.68f, 20.0f },
        { 2767531027u, "Lampadati Y-Cinquemila",       VehicleCategory::Sedan,      "sedans",      "lampadati_cinquemila.glb",4.98f, 1.92f, 1.45f, 19.0f },
        { 4171974011u, "Benefactor Yellow F1",         VehicleCategory::Supercar,   "supercars",   "benefactor_f1.glb",       5.20f, 2.00f, 1.05f, 26.0f },
        { 3612858749u, "Pegassi Zorrusso",             VehicleCategory::Supercar,   "supercars",   "pegassi_zorrusso.glb",    4.65f, 2.02f, 1.16f, 23.0f },
        { 2436313176u, "Annis ZR Remus",               VehicleCategory::Sports,     "sports",      "annis_remus.glb",         4.45f, 1.80f, 1.34f, 19.0f }
    };

    for (const auto& kb : s_kb) {
        VehicleAssetEntry entry;
        entry.modelHash = kb.hash;
        entry.displayName = kb.name;
        entry.category = kb.cat;
        entry.modelAssetPath = std::string("assets/models/vehicles/") + kb.subDir + "/" + kb.assetFile;
        entry.assetStatus = VerifyAssetExists(entry.modelAssetPath) ? ModelAssetStatus::Available : ModelAssetStatus::MissingModel;

        entry.simProfile.dimensions = glm::vec3(kb.length, kb.width, kb.height);
        entry.simProfile.axleZ = kb.length * 0.32f;
        entry.simProfile.halfTrackWidth = kb.width * 0.44f;
        entry.simProfile.wheelRadius = 0.35f;
        entry.simProfile.maxSpeed = kb.maxSpeed;
        entry.simProfile.maxAcceleration = (kb.cat == VehicleCategory::Supercar) ? 4.5f : 3.5f;
        entry.simProfile.comfortableDecel = 2.8f;
        entry.simProfile.maxDeceleration = 9.0f;
        entry.simProfile.minGap = 2.0f;

        m_assets[kb.hash] = entry;
    }
}

} // namespace SmartCity::vehicles

