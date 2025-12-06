#include "core/config_manager.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace sentinel {

ConfigManager::ConfigManager() : is_loaded_(false) {}

ConfigManager::~ConfigManager() {}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    Logger::info("Loading configuration from: " + filepath);
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::error("Failed to open config file: " + filepath);
        return false;
    }
    
    // Parse JSON manually (simple parser for this use case)
    // In production, use a JSON library like nlohmann/json
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Parse node configuration
    config_.node_id = parseUInt8(content, "\"id\"");
    
    // Parse sensor configuration
    std::string i2c_addr_str = parseString(content, "\"i2c_address\"");
    config_.i2c_address = static_cast<uint8_t>(std::stoi(i2c_addr_str, nullptr, 16));
    
    // Parse vision configuration
    config_.model_path = parseString(content, "\"model_path\"");
    
    // Parse LoRa configuration
    config_.lora_config.frequency = parseFloat(content, "\"frequency_mhz\"");
    config_.lora_config.bandwidth = parseInt(content, "\"bandwidth_khz\"");
    config_.lora_config.spreading_factor = parseInt(content, "\"spreading_factor\"");
    config_.lora_config.tx_power = parseInt(content, "\"tx_power_dbm\"");
    config_.lora_config.heartbeat_interval_sec = parseInt(content, "\"heartbeat_interval_sec\"");
    config_.lora_config.node_timeout_sec = parseInt(content, "\"node_timeout_sec\"");
    
    // Parse consensus configuration
    config_.consensus_threshold = parseFloat(content, "\"threshold\"");
    config_.consensus_timeout_sec = parseInt(content, "\"timeout_sec\"");
    
    // Parse alert configuration
    config_.alert_duration_sec = parseInt(content, "\"duration_sec\"");
    
    // Parse system configuration
    std::string log_level = parseString(content, "\"log_level\"");
    config_.debug_mode = (log_level == "DEBUG");
    
    is_loaded_ = true;
    Logger::info("Configuration loaded successfully");
    
    return true;
}

bool ConfigManager::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        Logger::error("Failed to open config file for writing: " + filepath);
        return false;
    }
    
    // Write JSON configuration
    file << "{\n";
    file << "  \"node\": {\n";
    file << "    \"id\": " << static_cast<int>(config_.node_id) << "\n";
    file << "  },\n";
    file << "  \"sensor\": {\n";
    file << "    \"i2c_address\": \"0x" << std::hex << static_cast<int>(config_.i2c_address) << "\"\n";
    file << "  },\n";
    file << "  \"vision\": {\n";
    file << "    \"model_path\": \"" << config_.model_path << "\"\n";
    file << "  },\n";
    file << "  \"lora\": {\n";
    file << "    \"frequency_mhz\": " << config_.lora_config.frequency << ",\n";
    file << "    \"bandwidth_khz\": " << config_.lora_config.bandwidth << ",\n";
    file << "    \"spreading_factor\": " << config_.lora_config.spreading_factor << ",\n";
    file << "    \"tx_power_dbm\": " << config_.lora_config.tx_power << "\n";
    file << "  },\n";
    file << "  \"consensus\": {\n";
    file << "    \"threshold\": " << config_.consensus_threshold << ",\n";
    file << "    \"timeout_sec\": " << config_.consensus_timeout_sec << "\n";
    file << "  }\n";
    file << "}\n";
    
    file.close();
    Logger::info("Configuration saved to: " + filepath);
    
    return true;
}

Config ConfigManager::getConfig() const {
    return config_;
}

void ConfigManager::setConfig(const Config& config) {
    config_ = config;
    is_loaded_ = true;
}

bool ConfigManager::isLoaded() const {
    return is_loaded_;
}

// Helper parsing functions
uint8_t ConfigManager::parseUInt8(const std::string& content, const std::string& key) {
    size_t pos = content.find(key);
    if (pos == std::string::npos) return 0;
    
    pos = content.find(":", pos);
    if (pos == std::string::npos) return 0;
    
    // Skip whitespace and find number
    pos++;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
    
    std::string number;
    while (pos < content.size() && (isdigit(content[pos]) || content[pos] == '-')) {
        number += content[pos++];
    }
    
    return static_cast<uint8_t>(std::stoi(number));
}

int ConfigManager::parseInt(const std::string& content, const std::string& key) {
    size_t pos = content.find(key);
    if (pos == std::string::npos) return 0;
    
    pos = content.find(":", pos);
    if (pos == std::string::npos) return 0;
    
    pos++;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
    
    std::string number;
    while (pos < content.size() && (isdigit(content[pos]) || content[pos] == '-')) {
        number += content[pos++];
    }
    
    return std::stoi(number);
}

float ConfigManager::parseFloat(const std::string& content, const std::string& key) {
    size_t pos = content.find(key);
    if (pos == std::string::npos) return 0.0f;
    
    pos = content.find(":", pos);
    if (pos == std::string::npos) return 0.0f;
    
    pos++;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
    
    std::string number;
    while (pos < content.size() && (isdigit(content[pos]) || content[pos] == '.' || content[pos] == '-')) {
        number += content[pos++];
    }
    
    return std::stof(number);
}

std::string ConfigManager::parseString(const std::string& content, const std::string& key) {
    size_t pos = content.find(key);
    if (pos == std::string::npos) return "";
    
    pos = content.find(":", pos);
    if (pos == std::string::npos) return "";
    
    pos = content.find("\"", pos);
    if (pos == std::string::npos) return "";
    
    pos++; // Skip opening quote
    
    std::string value;
    while (pos < content.size() && content[pos] != '\"') {
        value += content[pos++];
    }
    
    return value;
}

} // namespace sentinel