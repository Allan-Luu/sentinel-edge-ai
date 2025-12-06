#include "network/lora_mesh.h"
#include "utils/logger.h"
#include <cstring>
#include <algorithm>

namespace sentinel {

// Message protocol constants
constexpr uint8_t MSG_TYPE_HEARTBEAT = 0x01;
constexpr uint8_t MSG_TYPE_DETECTION = 0x02;
constexpr uint8_t MSG_TYPE_ACK = 0x03;

LoraMesh::LoraMesh(uint8_t node_id, const LoraConfig& config)
    : node_id_(node_id),
      config_(config),
      is_initialized_(false),
      detection_callback_(nullptr) {
}

LoraMesh::~LoraMesh() {
    shutdown();
}

bool LoraMesh::initialize() {
    Logger::info("Initializing LoRa mesh network (Node ID: " + 
                std::to_string(node_id_) + ")");
    
    // Initialize SPI for LoRa module
    if (!initializeSPI()) {
        Logger::error("Failed to initialize SPI");
        return false;
    }
    
    // Configure LoRa module
    if (!configureLoRa()) {
        Logger::error("Failed to configure LoRa module");
        return false;
    }
    
    // Start network threads
    receive_thread_ = std::thread(&LoraMesh::receiveLoop, this);
    heartbeat_thread_ = std::thread(&LoraMesh::heartbeatLoop, this);
    
    is_initialized_ = true;
    Logger::info("LoRa mesh network initialized successfully");
    return true;
}

bool LoraMesh::initializeSPI() {
    // TODO: Implement actual SPI initialization for your hardware
    // This is a placeholder that would use wiringPi or similar library
    
    // Example pseudo-code:
    // spi_fd_ = wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED);
    // return spi_fd_ >= 0;
    
    Logger::info("SPI initialized (placeholder)");
    return true;
}

bool LoraMesh::configureLoRa() {
    // Configure LoRa module parameters
    // Frequency, bandwidth, spreading factor, etc.
    
    Logger::info("Configuring LoRa:");
    Logger::info("  Frequency: " + std::to_string(config_.frequency) + " MHz");
    Logger::info("  Bandwidth: " + std::to_string(config_.bandwidth) + " kHz");
    Logger::info("  Spreading Factor: " + std::to_string(config_.spreading_factor));
    Logger::info("  TX Power: " + std::to_string(config_.tx_power) + " dBm");
    
    // TODO: Implement actual LoRa configuration
    // This would involve SPI communication with the LoRa module
    
    return true;
}

void LoraMesh::broadcastDetection(bool detected) {
    MeshMessage msg;
    msg.type = MSG_TYPE_DETECTION;
    msg.source_id = node_id_;
    msg.destination_id = 0xFF; // Broadcast
    msg.payload[0] = detected ? 1 : 0;
    msg.payload_len = 1;
    msg.timestamp = std::chrono::system_clock::now();
    
    sendMessage(msg);
    
    Logger::info("Broadcast detection: " + std::string(detected ? "TRUE" : "FALSE"));
}

void LoraMesh::sendMessage(const MeshMessage& msg) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    // Serialize message
    uint8_t buffer[256];
    size_t len = serializeMessage(msg, buffer);
    
    // Send via LoRa
    // TODO: Implement actual LoRa transmission
    // This would use the SPI interface to send data
    
    if (config_.debug_mode) {
        Logger::debug("Sent message type " + std::to_string(msg.type) +
                     " from node " + std::to_string(msg.source_id) +
                     " to node " + std::to_string(msg.destination_id));
    }
}

void LoraMesh::receiveLoop() {
    Logger::info("Starting receive loop");
    
    while (is_initialized_) {
        // TODO: Implement actual LoRa receive
        // This would poll the LoRa module via SPI
        
        // Check if data is available
        uint8_t buffer[256];
        int len = receiveData(buffer, sizeof(buffer));
        
        if (len > 0) {
            // Deserialize and process message
            MeshMessage msg = deserializeMessage(buffer, len);
            processMessage(msg);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    Logger::info("Receive loop terminated");
}

void LoraMesh::heartbeatLoop() {
    Logger::info("Starting heartbeat loop");
    
    while (is_initialized_) {
        // Send periodic heartbeat
        MeshMessage msg;
        msg.type = MSG_TYPE_HEARTBEAT;
        msg.source_id = node_id_;
        msg.destination_id = 0xFF; // Broadcast
        msg.payload_len = 0;
        msg.timestamp = std::chrono::system_clock::now();
        
        sendMessage(msg);
        
        // Clean up stale nodes
        cleanupStaleNodes();
        
        // Wait for next heartbeat interval
        std::this_thread::sleep_for(std::chrono::seconds(config_.heartbeat_interval_sec));
    }
    
    Logger::info("Heartbeat loop terminated");
}

void LoraMesh::processMessage(const MeshMessage& msg) {
    // Ignore messages from self
    if (msg.source_id == node_id_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    // Update node info
    auto& node = active_nodes_[msg.source_id];
    node.node_id = msg.source_id;
    node.last_seen = std::chrono::steady_clock::now();
    
    // Process based on message type
    switch (msg.type) {
        case MSG_TYPE_HEARTBEAT:
            if (config_.debug_mode) {
                Logger::debug("Received heartbeat from node " + 
                            std::to_string(msg.source_id));
            }
            break;
            
        case MSG_TYPE_DETECTION:
            node.detecting = (msg.payload[0] == 1);
            Logger::info("Node " + std::to_string(msg.source_id) + 
                        " detection: " + (node.detecting ? "TRUE" : "FALSE"));
            
            // Notify callback
            if (detection_callback_) {
                detection_callback_(msg.source_id, node.detecting);
            }
            break;
            
        case MSG_TYPE_ACK:
            if (config_.debug_mode) {
                Logger::debug("Received ACK from node " + 
                            std::to_string(msg.source_id));
            }
            break;
            
        default:
            Logger::warn("Unknown message type: " + std::to_string(msg.type));
            break;
    }
}

void LoraMesh::cleanupStaleNodes() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(config_.node_timeout_sec);
    
    for (auto it = active_nodes_.begin(); it != active_nodes_.end();) {
        if (now - it->second.last_seen > timeout) {
            Logger::info("Node " + std::to_string(it->first) + " timed out");
            it = active_nodes_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t LoraMesh::serializeMessage(const MeshMessage& msg, uint8_t* buffer) {
    size_t offset = 0;
    
    buffer[offset++] = msg.type;
    buffer[offset++] = msg.source_id;
    buffer[offset++] = msg.destination_id;
    buffer[offset++] = msg.payload_len;
    
    std::memcpy(buffer + offset, msg.payload, msg.payload_len);
    offset += msg.payload_len;
    
    // Add checksum
    uint8_t checksum = 0;
    for (size_t i = 0; i < offset; i++) {
        checksum ^= buffer[i];
    }
    buffer[offset++] = checksum;
    
    return offset;
}

MeshMessage LoraMesh::deserializeMessage(const uint8_t* buffer, size_t len) {
    MeshMessage msg;
    
    if (len < 5) {
        Logger::error("Invalid message length");
        return msg;
    }
    
    size_t offset = 0;
    msg.type = buffer[offset++];
    msg.source_id = buffer[offset++];
    msg.destination_id = buffer[offset++];
    msg.payload_len = buffer[offset++];
    
    if (msg.payload_len > MAX_PAYLOAD_SIZE) {
        Logger::error("Payload too large");
        msg.payload_len = 0;
        return msg;
    }
    
    std::memcpy(msg.payload, buffer + offset, msg.payload_len);
    offset += msg.payload_len;
    
    // Verify checksum
    uint8_t received_checksum = buffer[offset];
    uint8_t calculated_checksum = 0;
    for (size_t i = 0; i < offset; i++) {
        calculated_checksum ^= buffer[i];
    }
    
    if (received_checksum != calculated_checksum) {
        Logger::warn("Checksum mismatch");
    }
    
    msg.timestamp = std::chrono::system_clock::now();
    return msg;
}

int LoraMesh::receiveData(uint8_t* buffer, size_t max_len) {
    // TODO: Implement actual LoRa receive via SPI
    // This is a placeholder
    return 0;
}

int LoraMesh::getActiveNodeCount() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    return active_nodes_.size();
}

int LoraMesh::getDetectingNodeCount() const {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    int count = 0;
    for (const auto& pair : active_nodes_) {
        if (pair.second.detecting) {
            count++;
        }
    }
    return count;
}

void LoraMesh::processMessages() {
    // This is called from the main loop to ensure thread-safe processing
    // Additional message processing can be done here if needed
}

void LoraMesh::shutdown() {
    Logger::info("Shutting down LoRa mesh network");
    
    is_initialized_ = false;
    
    // Wait for threads to finish
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    // TODO: Close SPI interface
    
    Logger::info("LoRa mesh shutdown complete");
}

} // namespace sentinel