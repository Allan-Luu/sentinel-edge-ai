#ifndef LORA_MESH_H
#define LORA_MESH_H

#include <cstdint>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>

namespace sentinel {

// Forward declaration from sentinel_core.h
struct LoraConfig;

constexpr size_t MAX_PAYLOAD_SIZE = 64;

struct MeshMessage {
    uint8_t type;
    uint8_t source_id;
    uint8_t destination_id;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t payload_len;
    std::chrono::system_clock::time_point timestamp;
};

struct NodeInfo {
    uint8_t node_id;
    bool detecting;
    std::chrono::steady_clock::time_point last_seen;
    int rssi; // Signal strength
};

class LoraMesh {
public:
    explicit LoraMesh(uint8_t node_id, const LoraConfig& config);
    ~LoraMesh();
    
    // Initialize LoRa module and start networking
    bool initialize();
    
    // Broadcast detection status to all nodes
    void broadcastDetection(bool detected);
    
    // Send a message to the mesh
    void sendMessage(const MeshMessage& msg);
    
    // Process incoming messages (call from main loop)
    void processMessages();
    
    // Get number of active nodes
    int getActiveNodeCount() const;
    
    // Get number of nodes currently detecting smoke
    int getDetectingNodeCount() const;
    
    // Set callback for detection events from other nodes
    using DetectionCallback = std::function<void(uint8_t node_id, bool detected)>;
    void setDetectionCallback(DetectionCallback callback) {
        detection_callback_ = callback;
    }
    
    // Cleanup
    void shutdown();
    
private:
    // Initialize SPI interface for LoRa module
    bool initializeSPI();
    
    // Configure LoRa radio parameters
    bool configureLoRa();
    
    // Network threads
    void receiveLoop();
    void heartbeatLoop();
    
    // Message processing
    void processMessage(const MeshMessage& msg);
    void cleanupStaleNodes();
    
    // Message serialization
    size_t serializeMessage(const MeshMessage& msg, uint8_t* buffer);
    MeshMessage deserializeMessage(const uint8_t* buffer, size_t len);
    
    // Low-level LoRa communication
    int receiveData(uint8_t* buffer, size_t max_len);
    
    uint8_t node_id_;
    LoraConfig config_;
    bool is_initialized_;
    
    // Active nodes in the mesh
    std::map<uint8_t, NodeInfo> active_nodes_;
    mutable std::mutex nodes_mutex_;
    
    // Threading
    std::thread receive_thread_;
    std::thread heartbeat_thread_;
    std::mutex send_mutex_;
    
    // Callback
    DetectionCallback detection_callback_;
    
    // SPI file descriptor
    int spi_fd_;
};

} // namespace sentinel

#endif // LORA_MESH_H