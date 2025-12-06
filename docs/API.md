# Sentinel API Documentation

Complete API reference for the Sentinel Edge-AI Wildfire Detection System.

## Table of Contents

1. [Core Components](#core-components)
2. [Sensor Module](#sensor-module)
3. [Vision Module](#vision-module)
4. [Network Module](#network-module)
5. [Utility Classes](#utility-classes)
6. [Data Structures](#data-structures)

---

## Core Components

### SentinelCore

Main application controller that orchestrates all subsystems.

#### Constructor

```cpp
SentinelCore::SentinelCore(const Config& config)
```

**Parameters:**
- `config`: Configuration structure containing all system parameters

#### Methods

##### initialize()

```cpp
bool initialize()
```

Initialize all subsystems (sensors, vision, networking).

**Returns:** `true` if initialization successful, `false` otherwise

**Example:**
```cpp
Config config;
config.node_id = 1;
config.model_path = "../models/smoke_detection.tflite";

SentinelCore core(config);
if (core.initialize()) {
    core.run();
}
```

##### run()

```cpp
void run()
```

Main detection loop. Runs continuously until shutdown signal received.

**Behavior:**
- Checks sensors every 1 second
- Processes vision frames every 200ms
- Updates mesh network continuously
- Evaluates consensus when detection occurs

##### shutdown()

```cpp
void shutdown()
```

Gracefully shutdown all subsystems and cleanup resources.

---

## Sensor Module

### MQ2Sensor

Interface to MQ-2 smoke/gas sensor via I2C.

#### Constructor

```cpp
MQ2Sensor::MQ2Sensor(uint8_t i2c_address)
```

**Parameters:**
- `i2c_address`: I2C address of the ADS1015 ADC (default: 0x48)

#### Methods

##### initialize()

```cpp
bool initialize()
```

Open I2C connection and calibrate sensor.

**Returns:** `true` on success

**Calibration:** 30-second warm-up + 50 samples for R0 calculation

##### calibrate()

```cpp
bool calibrate()
```

Calibrate sensor in clean air environment.

**Returns:** `true` if calibration successful

**Time:** ~30 seconds

##### readAnalog()

```cpp
int readAnalog()
```

Read raw ADC value (0-4095 for 12-bit ADC).

**Returns:** Raw analog reading, or -1 on error

##### getResistance()

```cpp
float getResistance()
```

Calculate sensor resistance in kΩ.

**Returns:** Resistance value based on voltage divider formula

**Formula:** `Rs = ((Vcc - V) / V) * RL`

##### getPPM()

```cpp
float getPPM()
```

Get smoke concentration in parts per million.

**Returns:** Smoke concentration (PPM), or -1 on error

**Algorithm:** Uses smoke curve equation with Rs/R0 ratio

##### detectSmoke()

```cpp
bool detectSmoke()
```

Boolean smoke detection with temporal filtering.

**Returns:** `true` if smoke detected

**Threshold:** 200 PPM (configurable)

**Filter:** Requires 3 of last 5 readings above threshold

##### getReading()

```cpp
SensorReading getReading()
```

Get complete sensor data package.

**Returns:** `SensorReading` structure with all data

**Example:**
```cpp
MQ2Sensor sensor(0x48);
sensor.initialize();

SensorReading reading = sensor.getReading();
std::cout << "PPM: " << reading.ppm << std::endl;
std::cout << "Detected: " << reading.smoke_detected << std::endl;
```

---

## Vision Module

### SmokeDetector

Computer vision-based smoke detection using TensorFlow Lite.

#### Constructor

```cpp
SmokeDetector::SmokeDetector(const std::string& model_path)
```

**Parameters:**
- `model_path`: Path to TFLite model file

#### Methods

##### initialize()

```cpp
bool initialize()
```

Load AI model and initialize camera.

**Returns:** `true` on success

**Camera Settings:**
- Resolution: 640x480
- FPS: 30
- Device: /dev/video0

##### detectSmoke()

```cpp
DetectionResult detectSmoke()
```

Capture frame and run inference.

**Returns:** `DetectionResult` with confidence scores

**Processing Pipeline:**
1. Capture camera frame
2. Resize to 224x224 (model input size)
3. Normalize pixels to [0, 1]
4. Run TFLite inference
5. Apply temporal smoothing

**Example:**
```cpp
SmokeDetector detector("../models/smoke_detection.tflite");
detector.initialize();

DetectionResult result = detector.detectSmoke();
if (result.detected) {
    std::cout << "Smoke detected! Confidence: " 
              << result.confidence * 100 << "%" << std::endl;
}
```

##### captureFrame()

```cpp
cv::Mat captureFrame()
```

Capture single frame without inference.

**Returns:** OpenCV Mat containing RGB image

##### saveFrame()

```cpp
void saveFrame(const cv::Mat& frame, const std::string& filename)
```

Save frame to disk.

**Parameters:**
- `frame`: Image to save
- `filename`: Output file path

**Supported Formats:** JPG, PNG, BMP

---

## Network Module

### LoraMesh

LoRa mesh networking for distributed consensus.

#### Constructor

```cpp
LoraMesh::LoraMesh(uint8_t node_id, const LoraConfig& config)
```

**Parameters:**
- `node_id`: Unique identifier for this node (1-255)
- `config`: LoRa configuration (frequency, bandwidth, etc.)

#### Methods

##### initialize()

```cpp
bool initialize()
```

Configure LoRa module and start network threads.

**Returns:** `true` on success

**LoRa Parameters:**
- Frequency: 433 MHz (configurable)
- Bandwidth: 125 kHz
- Spreading Factor: 12
- TX Power: 20 dBm
- Range: ~10km line-of-sight

##### broadcastDetection()

```cpp
void broadcastDetection(bool detected)
```

Send detection status to all mesh nodes.

**Parameters:**
- `detected`: Current detection state

**Message Format:**
```
[Type][Source][Dest][Length][Payload][Checksum]
  1B     1B     1B     1B      0-64B      1B
```

##### setDetectionCallback()

```cpp
void setDetectionCallback(DetectionCallback callback)
```

Register callback for remote detection events.

**Parameters:**
- `callback`: Function called when remote node reports detection

**Example:**
```cpp
mesh.setDetectionCallback([](uint8_t node_id, bool detected) {
    std::cout << "Node " << (int)node_id 
              << (detected ? " detected smoke" : " cleared") 
              << std::endl;
});
```

##### getActiveNodeCount()

```cpp
int getActiveNodeCount() const
```

Get number of active nodes in mesh.

**Returns:** Count of nodes that responded to heartbeat

##### getDetectingNodeCount()

```cpp
int getDetectingNodeCount() const
```

Get number of nodes currently detecting smoke.

**Returns:** Count of nodes reporting positive detection

---

### ConsensusEngine

Distributed consensus algorithm for reducing false positives.

#### Constructor

```cpp
ConsensusEngine::ConsensusEngine(float threshold, int timeout_sec)
```

**Parameters:**
- `threshold`: Consensus ratio required (0.0-1.0, default: 0.6)
- `timeout_sec`: Voting period duration (default: 5)

#### Methods

##### startVoting()

```cpp
void startVoting(uint8_t local_node_id, bool local_detection)
```

Begin consensus voting session.

**Parameters:**
- `local_node_id`: This node's ID
- `local_detection`: This node's detection state

##### addVote()

```cpp
void addVote(uint8_t node_id, bool detection)
```

Record vote from remote node.

**Parameters:**
- `node_id`: Remote node identifier
- `detection`: Remote node's detection state

##### evaluateConsensus()

```cpp
ConsensusResult evaluateConsensus()
```

Calculate consensus after timeout period.

**Returns:** `ConsensusResult` with voting statistics

**Algorithm:**
```
consensus_ratio = detecting_nodes / total_nodes
confirmed = (consensus_ratio >= threshold)
```

**Example:**
```cpp
ConsensusEngine consensus(0.6f, 5);

// Local detection triggered
consensus.startVoting(1, true);

// Receive votes from other nodes
consensus.addVote(2, true);
consensus.addVote(3, false);
consensus.addVote(4, true);

// After timeout, evaluate
ConsensusResult result = consensus.evaluateConsensus();
if (result.detection_confirmed) {
    // 3/4 nodes = 75% > 60% threshold
    triggerAlert();
}
```

---

## Utility Classes

### Logger

Thread-safe logging system with severity levels.

#### Static Methods

##### setLevel()

```cpp
static void setLevel(LogLevel level)
```

Set minimum log level.

**Parameters:**
- `level`: DEBUG, INFO, WARN, or ERROR

##### Log Methods

```cpp
static void debug(const std::string& message)
static void info(const std::string& message)
static void warn(const std::string& message)
static void error(const std::string& message)
```

**Example:**
```cpp
Logger::setLevel(LogLevel::DEBUG);
Logger::info("System started");
Logger::warn("Sensor calibrating...");
Logger::error("Failed to connect");
```

**Output Format:**
```
[2024-12-06 14:23:45.123] INFO  - System started
[2024-12-06 14:23:46.456] WARN  - Sensor calibrating...
[2024-12-06 14:23:47.789] ERROR - Failed to connect
```

---

### DataProcessor

Statistical data processing and filtering.

#### Constructor

```cpp
DataProcessor::DataProcessor(size_t window_size = 100)
```

**Parameters:**
- `window_size`: Maximum samples to retain

#### Methods

##### addSample()

```cpp
void addSample(float value)
```

Add single data point.

##### Statistical Methods

```cpp
float getMean() const           // Average value
float getMedian() const         // Middle value
float getStdDev() const         // Standard deviation
float getMin() const            // Minimum value
float getMax() const            // Maximum value
float getRange() const          // Max - Min
```

##### Moving Averages

```cpp
float getMovingAverage(size_t window = 10) const
float getExponentialMovingAverage(float alpha = 0.3f) const
```

##### Filtering

```cpp
std::vector<float> applyLowPassFilter(float alpha = 0.3f) const
std::vector<float> applyMedianFilter(size_t window = 5) const
```

##### Outlier Detection

```cpp
bool detectOutlier(float value, float threshold_sigmas = 3.0f) const
```

Detect if value is outlier using z-score.

**Example:**
```cpp
DataProcessor processor(50);

for (int i = 0; i < 100; i++) {
    float ppm = sensor.getPPM();
    processor.addSample(ppm);
}

float avg = processor.getMean();
float trend = processor.calculateSlope();

if (trend > 0.5f) {
    std::cout << "Smoke level increasing!" << std::endl;
}
```

---

### ConfigManager

JSON configuration file management.

#### Methods

##### loadFromFile()

```cpp
bool loadFromFile(const std::string& filepath)
```

Load configuration from JSON file.

**Returns:** `true` on success

##### saveToFile()

```cpp
bool saveToFile(const std::string& filepath) const
```

Save configuration to JSON file.

**Returns:** `true` on success

##### getConfig()

```cpp
Config getConfig() const
```

Get current configuration structure.

**Example:**
```cpp
ConfigManager config_mgr;

if (config_mgr.loadFromFile("/etc/sentinel/config.json")) {
    Config config = config_mgr.getConfig();
    
    std::cout << "Node ID: " << (int)config.node_id << std::endl;
    std::cout << "Threshold: " << config.consensus_threshold << std::endl;
}
```

---

## Data Structures

### Config

Main configuration structure.

```cpp
struct Config {
    bool debug_mode;                    // Enable debug logging
    uint8_t i2c_address;               // Sensor I2C address
    std::string model_path;            // Path to TFLite model
    uint8_t node_id;                   // Node identifier
    float consensus_threshold;         // Consensus ratio (0.0-1.0)
    int consensus_timeout_sec;         // Voting period duration
    int alert_duration_sec;            // How long alert persists
    LoraConfig lora_config;            // LoRa parameters
};
```

### LoraConfig

LoRa radio configuration.

```cpp
struct LoraConfig {
    float frequency;                   // MHz (433 or 915)
    int bandwidth;                     // kHz (125, 250, 500)
    int spreading_factor;              // 7-12
    int tx_power;                      // dBm (2-20)
    int heartbeat_interval_sec;        // Heartbeat period
    int node_timeout_sec;              // Node timeout threshold
    bool debug_mode;                   // Debug logging
};
```

### DetectionResult

Vision detection output.

```cpp
struct DetectionResult {
    bool detected;                     // Smoke detected flag
    float confidence;                  // Raw confidence (0.0-1.0)
    float smoothed_confidence;         // Temporally filtered
    float inference_time_ms;           // Processing time
    std::chrono::system_clock::time_point timestamp;
};
```

### SensorReading

Complete sensor data package.

```cpp
struct SensorReading {
    std::chrono::system_clock::time_point timestamp;
    int analog_value;                  // Raw ADC reading
    float resistance;                  // Sensor resistance (kΩ)
    float ppm;                        // Smoke concentration
    bool smoke_detected;              // Boolean detection
};
```

### ConsensusResult

Consensus voting result.

```cpp
struct ConsensusResult {
    ConsensusState state;             // IDLE, VOTING, CONFIRMED, REJECTED
    bool reached;                     // Voting completed
    bool detection_confirmed;         // Consensus achieved
    int total_votes;                  // Number of voting nodes
    int positive_votes;               // Nodes detecting smoke
    float consensus_ratio;            // positive/total
};
```

---

## Usage Examples

### Basic Detection Loop

```cpp
#include "sentinel_core.h"

int main() {
    Config config;
    config.node_id = 1;
    config.model_path = "../models/smoke_detection.tflite";
    config.i2c_address = 0x48;
    config.consensus_threshold = 0.6f;
    
    SentinelCore sentinel(config);
    
    if (!sentinel.initialize()) {
        return 1;
    }
    
    sentinel.run();  // Blocks until shutdown
    
    return 0;
}
```

### Custom Detection Handler

```cpp
MQ2Sensor sensor(0x48);
SmokeDetector vision("model.tflite");

sensor.initialize();
vision.initialize();

while (running) {
    // Get sensor data
    float ppm = sensor.getPPM();
    
    // Get vision result
    auto result = vision.detectSmoke();
    
    // Fusion: require both
    if (ppm > 200 && result.confidence > 0.75) {
        triggerAlert();
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

### Mesh Network Setup

```cpp
LoraConfig lora_config;
lora_config.frequency = 433.0f;
lora_config.bandwidth = 125;
lora_config.spreading_factor = 12;

LoraMesh mesh(1, lora_config);
mesh.initialize();

mesh.setDetectionCallback([](uint8_t node_id, bool detected) {
    std::cout << "Node " << (int)node_id << ": " 
              << (detected ? "SMOKE" : "CLEAR") << std::endl;
});

// Broadcast local detection
mesh.broadcastDetection(true);

// Get mesh status
int active = mesh.getActiveNodeCount();
int detecting = mesh.getDetectingNodeCount();
```

---

## Error Handling

All methods that can fail return `bool` or check with `isInitialized()`.

```cpp
if (!sensor.initialize()) {
    Logger::error("Sensor init failed");
    // Check hardware connections
    // Verify I2C enabled
    return false;
}

if (!vision.initialize()) {
    Logger::error("Vision init failed");
    // Check model file exists
    // Verify camera connected
    return false;
}
```

## Thread Safety

- **Logger**: Thread-safe
- **DataProcessor**: Thread-safe (internal mutex)
- **ConsensusEngine**: Thread-safe (internal mutex)
- **LoraMesh**: Thread-safe for public methods
- **Sensors/Vision**: Not thread-safe, use from single thread

---

## Performance Considerations

### CPU Usage

- Sensor reading: <1% CPU
- Vision inference: ~40-50% CPU (one core)
- LoRa networking: <5% CPU

### Memory Usage

- Base system: ~50 MB
- TFLite model: ~10-20 MB
- Frame buffers: ~5 MB
- Total: ~100 MB typical

### Latency

- Sensor reading: <10ms
- Vision inference: 100-150ms
- LoRa transmission: 200-500ms
- End-to-end: <2 seconds

---

## Building Documentation

Generate Doxygen documentation:

```bash
cd sentinel-edge-ai
doxygen Doxyfile
```

View at: `docs/html/index.html`