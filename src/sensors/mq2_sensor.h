#ifndef MQ2_SENSOR_H
#define MQ2_SENSOR_H

#include "sensors/sensor_interface.h"
#include <cstdint>
#include <vector>
#include <chrono>

namespace sentinel {

struct SensorReading {
    std::chrono::system_clock::time_point timestamp;
    int analog_value;
    float resistance;
    float ppm;
    bool smoke_detected;
};

class MQ2Sensor : public IGasSensor {
public:
    explicit MQ2Sensor(uint8_t i2c_address);
    ~MQ2Sensor();
    
    // ISensor interface
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;
    bool calibrate() override;
    bool isHealthy() const override;
    
    // IGasSensor interface
    int readAnalog() override;
    float getResistance() override;
    float getPPM() override;
    bool detectSmoke() override;
    
    // Additional methods
    SensorReading getReading();
    
private:
    uint8_t i2c_addr_;
    int i2c_fd_;
    float ro_; // Sensor resistance in clean air
    bool is_initialized_;
    std::vector<bool> detection_history_;
};

} // namespace sentinel

#endif // MQ2_SENSOR_H