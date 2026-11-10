#include "core/Config.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <regex>

namespace SmartCity::core {

static bool ExtractFloat(const std::string& content, const std::string& key, float& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = std::stof(match[1].str());
        return true;
    }
    return false;
}

static bool ExtractDouble(const std::string& content, const std::string& key, double& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = std::stod(match[1].str());
        return true;
    }
    return false;
}

static bool ExtractUInt(const std::string& content, const std::string& key, uint32_t& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = static_cast<uint32_t>(std::stoul(match[1].str()));
        return true;
    }
    return false;
}

static bool ExtractInt(const std::string& content, const std::string& key, int& outVal) {
    std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]+)");
    std::smatch match;
    if (std::regex_search(content, match, re) && match.size() > 1) {
        outVal = std::stoi(match[1].str());
        return true;
    }
    return false;
}

bool ConfigManager::LoadSimulationConfig(const std::string& path, SimulationSettings& outSettings) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::Warn("Could not open simulation config at ", path, ". Using default configuration.");
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    ExtractUInt(content, "random_seed", outSettings.randomSeed);
    ExtractDouble(content, "fixed_timestep_s", outSettings.fixedTimestep);
    ExtractDouble(content, "max_delta_time_s", outSettings.maxDeltaTime);

    ExtractFloat(content, "speed_limit_mps", outSettings.speedLimit);
    ExtractFloat(content, "turning_speed_limit_mps", outSettings.turningSpeedLimit);
    ExtractFloat(content, "idm_desired_velocity_mps", outSettings.idmDesiredVelocity);
    ExtractFloat(content, "idm_safe_time_headway_s", outSettings.idmSafeTimeHeadway);
    ExtractFloat(content, "idm_min_gap_distance_m", outSettings.idmMinGapDistance);
    ExtractFloat(content, "idm_max_acceleration_mps2", outSettings.idmMaxAcceleration);
    ExtractFloat(content, "idm_comfortable_deceleration_mps2", outSettings.idmComfortableDecel);
    ExtractFloat(content, "idm_delta_exponent", outSettings.idmDeltaExponent);

    ExtractFloat(content, "phase_green_time_s", outSettings.phaseGreenTime);
    ExtractFloat(content, "phase_yellow_clearance_s", outSettings.phaseYellowClearance);
    ExtractFloat(content, "phase_all_red_clearance_s", outSettings.phaseAllRedClearance);
    ExtractFloat(content, "left_turn_green_time_s", outSettings.leftTurnGreenTime);
    ExtractFloat(content, "pedestrian_walk_time_s", outSettings.pedestrianWalkTime);
    ExtractFloat(content, "min_green_s", outSettings.minGreen);
    ExtractFloat(content, "max_green_s", outSettings.maxGreen);

    ExtractFloat(content, "north_spawn_rate_vpm", outSettings.northSpawnRate);
    ExtractFloat(content, "south_spawn_rate_vpm", outSettings.southSpawnRate);
    ExtractFloat(content, "east_spawn_rate_vpm", outSettings.eastSpawnRate);
    ExtractFloat(content, "west_spawn_rate_vpm", outSettings.westSpawnRate);

    Logger::Info("Loaded simulation config from ", path);
    return true;
}

bool ConfigManager::LoadIntersectionConfig(const std::string& path, IntersectionSettings& outSettings) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::Warn("Could not open intersection config at ", path, ". Using default configuration.");
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    ExtractFloat(content, "road_width_m", outSettings.roadWidth);
    ExtractFloat(content, "approach_length_m", outSettings.approachLength);
    ExtractFloat(content, "sidewalk_width_m", outSettings.sidewalkWidth);
    ExtractFloat(content, "curb_height_m", outSettings.curbHeight);
    ExtractFloat(content, "corner_radius_m", outSettings.cornerRadius);
    ExtractFloat(content, "stop_line_distance_m", outSettings.stopLineDistance);
    ExtractFloat(content, "crosswalk_distance_m", outSettings.crosswalkDistance);
    ExtractFloat(content, "crosswalk_width_m", outSettings.crosswalkWidth);
    ExtractInt(content, "num_lanes_per_approach", outSettings.numLanesPerApproach);

    Logger::Info("Loaded intersection config from ", path);
    return true;
}

} // namespace SmartCity::core
