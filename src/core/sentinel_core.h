#ifndef SENTINEL_CORE_H
#define SENTINEL_CORE_H

#include <memory>
#include <chrono>
#include <string>
#include <cstdint>

namespace sentinel {

// Forward declarations
class MQ2Sensor;
class SmokeDetector;
class LoraMesh;

// Configuration structures
struct LoraConfig {
    float frequency = 433.0f;        // MHz
    int bandwidth = 125;             // kHz
    int spreading_factor = 12;
    int tx_power = 20;               // dBm
    int heartbeat_interval_sec = 30;
    int node_timeout_sec = 90;
    bool debug_mode = false;
};

struct Config {
    bool debug_mode = false;
    uint8_t i2c_address = 0x48;
    std::string model_path;
    uint8_t node_id = 1;
    float consensus_threshold = 0.6f;
    int consensus_timeout_sec = 5;
    int alert_duration_sec = 60;
    LoraConfig lora_config;
};

// Detection data structure
struct DetectionData {
    bool sensor_detected = false;
    float smoke_ppm = 0.0f;
    std::chrono::system_clock::time_point sensor_timestamp;
    
    bool vision_detected = false;
    float vision_confidence = 0.0f;
    std::chrono::system_clock::time_point vision_timestamp;
};

// Alert states
enum class AlertState {
    IDLE,
    PENDING,
    ALERT
};

class SentinelCore {
public:
    explicit SentinelCore(const Config& config);
    ~SentinelCore();
    
    // Initialize all subsystems
    bool initialize();
    
    // Main detection loop
    void run();
    
    // Shutdown and cleanup
    void shutdown();
    
private:
    // Subsystem checks
    void checkSensor();
    void checkVision();
    void updateAlertState();
    void evaluateConsensus();
    
    // Mesh network callbacks
    void handleMeshDetection(uint8_t node_id, bool detected);
    
    // Alert handling
    void triggerAlert();
    
    Config config_;
    
    // Subsystem instances
    std::unique_ptr<MQ2Sensor> sensor_;
    std::unique_ptr<SmokeDetector> detector_;
    std::unique_ptr<LoraMesh> mesh_;
    
    // State tracking
    DetectionData detection_data_;
    AlertState alert_state_;
    std::chrono::steady_clock::time_point consensus_start_time_;
    std::chrono::steady_clock::time_point alert_start_time_;
};

} // namespace sentinel

#endif // SENTINEL_CORE_H