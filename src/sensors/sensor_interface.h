#ifndef SENTINEL_SENSOR_INTERFACE_H
#define SENTINEL_SENSOR_INTERFACE_H

#include <cstdint>
#include <chrono>
#include <string>
#include <cmath>

namespace sentinel {

// Sensor status enumeration
enum class SensorStatus {
    OK,
    WARMING_UP,
    CALIBRATING,
    ERROR,
    NOT_CONNECTED,
    OUT_OF_RANGE
};

// Sensor calibration data
struct CalibrationData {
    float offset;
    float scale_factor;
    std::chrono::system_clock::time_point calibration_time;
    bool is_valid;
    
    CalibrationData() 
        : offset(0.0f), 
          scale_factor(1.0f), 
          is_valid(false) {}
};

// Abstract base class for all sensors
class ISensor {
public:
    virtual ~ISensor() = default;
    
    // Initialize sensor hardware
    virtual bool initialize() = 0;
    
    // Cleanup and shutdown
    virtual void shutdown() = 0;
    
    // Check if sensor is properly initialized
    virtual bool isInitialized() const = 0;
    
    // Calibrate sensor (if applicable)
    virtual bool calibrate() = 0;
    
    // Get sensor health status
    virtual bool isHealthy() const = 0;
    
    // Get sensor status
    virtual SensorStatus getStatus() const {
        if (!isInitialized()) {
            return SensorStatus::NOT_CONNECTED;
        }
        if (!isHealthy()) {
            return SensorStatus::ERROR;
        }
        return SensorStatus::OK;
    }
    
    // Get sensor name/type
    virtual std::string getName() const {
        return "Unknown Sensor";
    }
};

// Gas sensor interface (for MQ-2, MQ-7, etc.)
class IGasSensor : public ISensor {
public:
    virtual ~IGasSensor() = default;
    
    // Read raw analog value
    virtual int readAnalog() = 0;
    
    // Get sensor resistance in kOhms
    virtual float getResistance() = 0;
    
    // Get gas concentration in PPM
    virtual float getPPM() = 0;
    
    // Detect presence of smoke/gas
    virtual bool detectSmoke() = 0;
    
    // Get sensor name
    std::string getName() const override {
        return "Gas Sensor";
    }
};

// Temperature sensor interface
class ITemperatureSensor : public ISensor {
public:
    virtual ~ITemperatureSensor() = default;
    
    // Get temperature in Celsius
    virtual float getTemperatureCelsius() = 0;
    
    // Get temperature in Fahrenheit
    virtual float getTemperatureFahrenheit() {
        return (getTemperatureCelsius() * 9.0f / 5.0f) + 32.0f;
    }
    
    // Get temperature in Kelvin
    virtual float getTemperatureKelvin() {
        return getTemperatureCelsius() + 273.15f;
    }
    
    // Get sensor name
    std::string getName() const override {
        return "Temperature Sensor";
    }
};

// Humidity sensor interface
class IHumiditySensor : public ISensor {
public:
    virtual ~IHumiditySensor() = default;
    
    // Get relative humidity percentage (0-100)
    virtual float getHumidity() = 0;
    
    // Check if humidity is in valid range
    virtual bool isHumidityValid() {
        float humidity = getHumidity();
        return humidity >= 0.0f && humidity <= 100.0f;
    }
    
    // Get sensor name
    std::string getName() const override {
        return "Humidity Sensor";
    }
};

// Environmental sensor (combined temp + humidity)
class IEnvironmentalSensor : public ITemperatureSensor, public IHumiditySensor {
public:
    virtual ~IEnvironmentalSensor() = default;
    
    // Environmental data structure
    struct EnvironmentalData {
        float temperature_c;
        float humidity_percent;
        std::chrono::system_clock::time_point timestamp;
        bool valid;
        
        EnvironmentalData() 
            : temperature_c(0.0f), 
              humidity_percent(0.0f), 
              valid(false) {
            timestamp = std::chrono::system_clock::now();
        }
    };
    
    // Read all environmental data at once
    virtual EnvironmentalData readAll() = 0;
    
    // Calculate dew point (°C)
    virtual float calculateDewPoint() {
        float temp = getTemperatureCelsius();
        float humidity = getHumidity();
        
        // Magnus formula approximation
        float a = 17.27f;
        float b = 237.7f;
        float alpha = ((a * temp) / (b + temp)) + std::log(humidity / 100.0f);
        float dew_point = (b * alpha) / (a - alpha);
        
        return dew_point;
    }
    
    // Calculate heat index (°F) - feels like temperature
    virtual float calculateHeatIndex() {
        float temp_f = getTemperatureFahrenheit();
        float humidity = getHumidity();
        
        // Heat index formula (valid for temp > 80°F)
        if (temp_f < 80.0f) {
            return temp_f;
        }
        
        float hi = -42.379f + 
                   2.04901523f * temp_f + 
                   10.14333127f * humidity - 
                   0.22475541f * temp_f * humidity - 
                   0.00683783f * temp_f * temp_f - 
                   0.05481717f * humidity * humidity + 
                   0.00122874f * temp_f * temp_f * humidity + 
                   0.00085282f * temp_f * humidity * humidity - 
                   0.00000199f * temp_f * temp_f * humidity * humidity;
        
        return hi;
    }
    
    // Get sensor name
    std::string getName() const override {
        return "Environmental Sensor";
    }
};

// Pressure sensor interface
class IPressureSensor : public ISensor {
public:
    virtual ~IPressureSensor() = default;
    
    // Get pressure in Pascals
    virtual float getPressurePa() = 0;
    
    // Get pressure in hPa (hectopascals/millibars)
    virtual float getPressureHPa() {
        return getPressurePa() / 100.0f;
    }
    
    // Get pressure in PSI
    virtual float getPressurePSI() {
        return getPressurePa() * 0.000145038f;
    }
    
    // Get sensor name
    std::string getName() const override {
        return "Pressure Sensor";
    }
};

// Light sensor interface
class ILightSensor : public ISensor {
public:
    virtual ~ILightSensor() = default;
    
    // Get light intensity in lux
    virtual float getLightLux() = 0;
    
    // Check if it's dark (below threshold)
    virtual bool isDark(float threshold_lux = 10.0f) {
        return getLightLux() < threshold_lux;
    }
    
    // Get sensor name
    std::string getName() const override {
        return "Light Sensor";
    }
};

// Motion/PIR sensor interface
class IMotionSensor : public ISensor {
public:
    virtual ~IMotionSensor() = default;
    
    // Detect motion
    virtual bool detectMotion() = 0;
    
    // Get time since last motion detected
    virtual std::chrono::milliseconds getTimeSinceMotion() = 0;
    
    // Get sensor name
    std::string getName() const override {
        return "Motion Sensor";
    }
};

} // namespace sentinel

#endif // SENTINEL_SENSOR_INTERFACE_H