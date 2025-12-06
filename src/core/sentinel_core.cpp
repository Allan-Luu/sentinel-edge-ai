#include "sentinel_core.h"
#include "sensors/mq2_sensor.h"
#include "vision/smoke_detector.h"
#include "network/lora_mesh.h"
#include "utils/logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

namespace sentinel {

// Global flag for graceful shutdown
static volatile bool g_running = true;

void signalHandler(int signum) {
    Logger::info("Interrupt signal received. Shutting down...");
    g_running = false;
}

SentinelCore::SentinelCore(const Config& config) 
    : config_(config),
      sensor_(nullptr),
      detector_(nullptr),
      mesh_(nullptr),
      alert_state_(AlertState::IDLE) {
}

SentinelCore::~SentinelCore() {
    shutdown();
}

bool SentinelCore::initialize() {
    Logger::info("Initializing Sentinel Core...");
    
    // Install signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize sensor module
    sensor_ = std::make_unique<MQ2Sensor>(config_.i2c_address);
    if (!sensor_->initialize()) {
        Logger::error("Failed to initialize MQ2 sensor");
        return false;
    }
    Logger::info("MQ2 sensor initialized successfully");
    
    // Initialize vision detector
    detector_ = std::make_unique<SmokeDetector>(config_.model_path);
    if (!detector_->initialize()) {
        Logger::error("Failed to initialize smoke detector");
        return false;
    }
    Logger::info("Smoke detector initialized successfully");
    
    // Initialize LoRa mesh
    mesh_ = std::make_unique<LoraMesh>(config_.node_id, config_.lora_config);
    if (!mesh_->initialize()) {
        Logger::error("Failed to initialize LoRa mesh");
        return false;
    }
    Logger::info("LoRa mesh initialized successfully");
    
    // Register mesh callback
    mesh_->setDetectionCallback([this](uint8_t node_id, bool detected) {
        this->handleMeshDetection(node_id, detected);
    });
    
    Logger::info("Sentinel Core initialization complete");
    return true;
}

void SentinelCore::run() {
    Logger::info("Starting Sentinel detection loop...");
    
    auto last_sensor_check = std::chrono::steady_clock::now();
    auto last_vision_check = std::chrono::steady_clock::now();
    
    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        
        // Check smoke sensor (every 1 second)
        if (now - last_sensor_check >= std::chrono::seconds(1)) {
            checkSensor();
            last_sensor_check = now;
        }
        
        // Check vision system (every 200ms)
        if (now - last_vision_check >= std::chrono::milliseconds(200)) {
            checkVision();
            last_vision_check = now;
        }
        
        // Process mesh messages
        mesh_->processMessages();
        
        // Update alert state
        updateAlertState();
        
        // Small sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    Logger::info("Detection loop terminated");
}

void SentinelCore::checkSensor() {
    float ppm = sensor_->getPPM();
    bool smoke_detected = sensor_->detectSmoke();
    
    if (config_.debug_mode) {
        Logger::debug("Sensor PPM: " + std::to_string(ppm) + 
                     " Detected: " + std::to_string(smoke_detected));
    }
    
    detection_data_.sensor_detected = smoke_detected;
    detection_data_.smoke_ppm = ppm;
    detection_data_.sensor_timestamp = std::chrono::system_clock::now();
}

void SentinelCore::checkVision() {
    auto result = detector_->detectSmoke();
    
    if (config_.debug_mode) {
        Logger::debug("Vision confidence: " + std::to_string(result.confidence) +
                     " Detected: " + std::to_string(result.detected));
    }
    
    detection_data_.vision_detected = result.detected;
    detection_data_.vision_confidence = result.confidence;
    detection_data_.vision_timestamp = std::chrono::system_clock::now();
}

void SentinelCore::updateAlertState() {
    bool local_detection = detection_data_.sensor_detected || 
                          detection_data_.vision_detected;
    
    if (local_detection) {
        if (alert_state_ == AlertState::IDLE) {
            Logger::info("Local detection triggered - entering PENDING state");
            alert_state_ = AlertState::PENDING;
            consensus_start_time_ = std::chrono::steady_clock::now();
            
            // Broadcast detection to mesh
            mesh_->broadcastDetection(true);
        }
        
        // Check if consensus window expired
        if (alert_state_ == AlertState::PENDING) {
            auto elapsed = std::chrono::steady_clock::now() - consensus_start_time_;
            if (elapsed >= std::chrono::seconds(config_.consensus_timeout_sec)) {
                evaluateConsensus();
            }
        }
    } else {
        if (alert_state_ == AlertState::ALERT) {
            // Check if alert should be cleared
            auto elapsed = std::chrono::steady_clock::now() - alert_start_time_;
            if (elapsed >= std::chrono::seconds(config_.alert_duration_sec)) {
                Logger::info("Alert cleared - returning to IDLE");
                alert_state_ = AlertState::IDLE;
                mesh_->broadcastDetection(false);
            }
        } else if (alert_state_ == AlertState::PENDING) {
            Logger::info("Local detection cleared - returning to IDLE");
            alert_state_ = AlertState::IDLE;
            mesh_->broadcastDetection(false);
        }
    }
}

void SentinelCore::evaluateConsensus() {
    int total_nodes = mesh_->getActiveNodeCount() + 1; // +1 for self
    int detecting_nodes = mesh_->getDetectingNodeCount() + 
                         (detection_data_.sensor_detected || 
                          detection_data_.vision_detected ? 1 : 0);
    
    float consensus_ratio = static_cast<float>(detecting_nodes) / total_nodes;
    
    Logger::info("Consensus evaluation: " + std::to_string(detecting_nodes) + 
                "/" + std::to_string(total_nodes) + " nodes (" +
                std::to_string(consensus_ratio * 100) + "%)");
    
    if (consensus_ratio >= config_.consensus_threshold) {
        Logger::warn("ALERT: Wildfire detection confirmed by consensus!");
        alert_state_ = AlertState::ALERT;
        alert_start_time_ = std::chrono::steady_clock::now();
        
        // Trigger alert actions
        triggerAlert();
    } else {
        Logger::info("Consensus not reached - false positive filtered");
        alert_state_ = AlertState::IDLE;
    }
}

void SentinelCore::handleMeshDetection(uint8_t node_id, bool detected) {
    if (config_.debug_mode) {
        Logger::debug("Mesh detection from node " + std::to_string(node_id) +
                     ": " + std::to_string(detected));
    }
}

void SentinelCore::triggerAlert() {
    // Log alert with all detection data
    Logger::warn("=== WILDFIRE ALERT ===");
    Logger::warn("Sensor PPM: " + std::to_string(detection_data_.smoke_ppm));
    Logger::warn("Vision Confidence: " + 
                std::to_string(detection_data_.vision_confidence));
    Logger::warn("Detecting Nodes: " + 
                std::to_string(mesh_->getDetectingNodeCount() + 1));
    Logger::warn("=====================");
    
    // TODO: Add additional alert mechanisms
    // - Send notification via MQTT
    // - Activate sirens/lights
    // - Log to central database
}

void SentinelCore::shutdown() {
    Logger::info("Shutting down Sentinel Core...");
    g_running = false;
    
    if (mesh_) {
        mesh_->shutdown();
    }
    
    if (detector_) {
        detector_->shutdown();
    }
    
    if (sensor_) {
        sensor_->shutdown();
    }
    
    Logger::info("Shutdown complete");
}

} // namespace sentinel

int main(int argc, char* argv[]) {
    using namespace sentinel;
    
    Logger::info("Sentinel Edge-AI Wildfire Detection System v1.0");
    Logger::info("================================================");
    
    // Load configuration
    Config config;
    config.debug_mode = false;
    config.i2c_address = 0x48;
    config.model_path = "../models/smoke_detection.tflite";
    config.node_id = 1;
    config.consensus_threshold = 0.6f;
    config.consensus_timeout_sec = 5;
    config.alert_duration_sec = 60;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            config.debug_mode = true;
            Logger::setLevel(LogLevel::DEBUG);
        } else if (arg == "--config" && i + 1 < argc) {
            // TODO: Load config from file
            i++;
        }
    }
    
    // Initialize and run
    SentinelCore core(config);
    
    if (!core.initialize()) {
        Logger::error("Failed to initialize Sentinel Core");
        return 1;
    }
    
    core.run();
    
    return 0;
}