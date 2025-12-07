#include "sensors/mq2_sensor.h"
#include "utils/logger.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath>
#include <algorithm>

namespace sentinel {

// MQ-2 calibration constants
constexpr float RL_VALUE = 5.0f;           // Load resistance in kOhms
constexpr float RO_CLEAN_AIR = 9.83f;      // Sensor resistance in clean air
constexpr float SMOKE_CURVE[3] = {2.3f, 0.53f, -0.44f}; // Smoke curve parameters

MQ2Sensor::MQ2Sensor(uint8_t i2c_address)
    : i2c_addr_(i2c_address),
      i2c_fd_(-1),
      ro_(RO_CLEAN_AIR),
      is_initialized_(false) {
}

MQ2Sensor::~MQ2Sensor() {
    shutdown();
}

bool MQ2Sensor::initialize() {
    Logger::info("Initializing MQ2 sensor on I2C address 0x" + 
                std::to_string(i2c_addr_));
    
    // Open I2C bus
    const char* i2c_device = "/dev/i2c-1";
    i2c_fd_ = open(i2c_device, O_RDWR);
    if (i2c_fd_ < 0) {
        Logger::error("Failed to open I2C device: " + std::string(i2c_device));
        return false;
    }
    
    // Set I2C slave address
    if (ioctl(i2c_fd_, I2C_SLAVE, i2c_addr_) < 0) {
        Logger::error("Failed to set I2C slave address");
        close(i2c_fd_);
        i2c_fd_ = -1;
        return false;
    }
    
    // Calibrate sensor
    Logger::info("Calibrating MQ2 sensor (30 seconds warm-up)...");
    if (!calibrate()) {
        Logger::error("Sensor calibration failed");
        close(i2c_fd_);
        i2c_fd_ = -1;
        return false;
    }
    
    is_initialized_ = true;
    Logger::info("MQ2 sensor initialized successfully (R0=" + 
                std::to_string(ro_) + " kOhms)");
    return true;
}

bool MQ2Sensor::calibrate() {
    // Warm-up period (30 seconds)
    for (int i = 0; i < 30; i++) {
        readAnalog();
        sleep(1);
        if (i % 5 == 0) {
            Logger::info("Calibrating... " + std::to_string(i) + "/30s");
        }
    }
    
    // Calculate R0 by averaging readings in clean air
    float rs_sum = 0.0f;
    const int samples = 50;
    
    for (int i = 0; i < samples; i++) {
        float rs = getResistance();
        rs_sum += rs;
        usleep(100000); // 100ms delay
    }
    
    ro_ = rs_sum / samples / RO_CLEAN_AIR;
    
    // Validate calibration
    if (ro_ <= 0 || ro_ > 50) {
        Logger::error("Invalid calibration value: " + std::to_string(ro_));
        ro_ = RO_CLEAN_AIR; // Use default
        return false;
    }
    
    return true;
}

int MQ2Sensor::readAnalog() {
    if (!is_initialized_ || i2c_fd_ < 0) {
        return -1;
    }
    
    // Read 2 bytes from ADC
    uint8_t buffer[2] = {0};
    if (read(i2c_fd_, buffer, 2) != 2) {
        Logger::error("Failed to read from I2C device");
        return -1;
    }
    
    // Convert to 12-bit value (assuming ADS1015)
    int value = ((buffer[0] & 0x0F) << 8) | buffer[1];
    return value;
}

float MQ2Sensor::getResistance() {
    int analog_value = readAnalog();
    if (analog_value < 0) {
        return -1.0f;
    }
    
    // Convert ADC reading to voltage (assuming 3.3V reference, 12-bit ADC)
    float voltage = (analog_value / 4095.0f) * 3.3f;
    
    // Calculate sensor resistance
    // Rs = (Vc - V) * RL / V
    if (voltage <= 0.01f) {
        return -1.0f; // Prevent division by zero
    }
    
    float rs = ((3.3f - voltage) / voltage) * RL_VALUE;
    return rs;
}

float MQ2Sensor::getPPM() {
    float rs = getResistance();
    if (rs < 0) {
        return -1.0f;
    }
    
    // Calculate Rs/R0 ratio
    float ratio = rs / ro_;
    
    // Apply smoke curve equation: log(ppm) = m * log(ratio) + b
    // Where m = SMOKE_CURVE[1] and b = SMOKE_CURVE[2]
    float log_ppm = (std::log10(ratio) - SMOKE_CURVE[1]) / SMOKE_CURVE[2];
    float ppm = std::pow(10, log_ppm);
    
    return ppm;
}

bool MQ2Sensor::detectSmoke() {
    float ppm = getPPM();
    
    // Threshold for smoke detection (adjustable based on environment)
    const float SMOKE_THRESHOLD = 200.0f; // PPM
    
    // Apply temporal filtering to reduce noise
    detection_history_.push_back(ppm > SMOKE_THRESHOLD);
    if (detection_history_.size() > 5) {
        detection_history_.erase(detection_history_.begin());
    }
    
    // Require 3 out of 5 recent readings to be positive
    int positive_count = 0;
    for (bool detected : detection_history_) {
        if (detected) positive_count++;
    }
    
    return positive_count >= 3;
}

SensorReading MQ2Sensor::getReading() {
    SensorReading reading;
    reading.timestamp = std::chrono::system_clock::now();
    reading.analog_value = readAnalog();
    reading.resistance = getResistance();
    reading.ppm = getPPM();
    reading.smoke_detected = detectSmoke();
    return reading;
}

bool MQ2Sensor::isInitialized() const {
    return is_initialized_;
}

bool MQ2Sensor::isHealthy() const {
    if (!is_initialized_) {
        return false;
    }
    
    // Check if we can read from sensor
    int value = const_cast<MQ2Sensor*>(this)->readAnalog();
    return value >= 0;
}

void MQ2Sensor::shutdown() {
    if (i2c_fd_ >= 0) {
        close(i2c_fd_);
        i2c_fd_ = -1;
    }
    is_initialized_ = false;
    Logger::info("MQ2 sensor shutdown complete");
}

} // namespace sentinel