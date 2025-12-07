#include "sensors/sensor_interface.h"
#include <cmath>

namespace sentinel {

// CalibrationData implementation
// Constructor is already inline in header

// No additional implementation needed for abstract interfaces
// All methods are either pure virtual or have inline implementations

// Helper functions for sensor data processing

namespace SensorUtils {

// Convert resistance ratio to PPM using power law
float resistanceRatioToPPM(float rs_r0_ratio, float slope, float intercept) {
    // PPM = 10^((log10(Rs/R0) - intercept) / slope)
    if (rs_r0_ratio <= 0) {
        return -1.0f;
    }
    
    float log_ratio = std::log10(rs_r0_ratio);
    float log_ppm = (log_ratio - intercept) / slope;
    float ppm = std::pow(10.0f, log_ppm);
    
    return ppm;
}

// Convert PPM to resistance ratio
float ppmToResistanceRatio(float ppm, float slope, float intercept) {
    // Rs/R0 = 10^(slope * log10(PPM) + intercept)
    if (ppm <= 0) {
        return -1.0f;
    }
    
    float log_ppm = std::log10(ppm);
    float log_ratio = slope * log_ppm + intercept;
    float ratio = std::pow(10.0f, log_ratio);
    
    return ratio;
}

// Apply exponential moving average filter
float applyEMA(float new_value, float old_value, float alpha) {
    // EMA = alpha * new + (1 - alpha) * old
    if (alpha < 0.0f || alpha > 1.0f) {
        alpha = 0.3f; // Default
    }
    
    return alpha * new_value + (1.0f - alpha) * old_value;
}

// Check if value is within range
bool isInRange(float value, float min_val, float max_val) {
    return value >= min_val && value <= max_val;
}

// Clamp value to range
float clamp(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Map value from one range to another
float mapRange(float value, float in_min, float in_max, float out_min, float out_max) {
    if (in_max == in_min) {
        return out_min;
    }
    
    float normalized = (value - in_min) / (in_max - in_min);
    float mapped = normalized * (out_max - out_min) + out_min;
    
    return mapped;
}

// Calculate moving average
float calculateMovingAverage(const float* values, int count) {
    if (count <= 0 || values == nullptr) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        sum += values[i];
    }
    
    return sum / count;
}

// Calculate standard deviation
float calculateStdDev(const float* values, int count) {
    if (count <= 1 || values == nullptr) {
        return 0.0f;
    }
    
    float mean = calculateMovingAverage(values, count);
    float variance = 0.0f;
    
    for (int i = 0; i < count; i++) {
        float diff = values[i] - mean;
        variance += diff * diff;
    }
    
    variance /= (count - 1);
    return std::sqrt(variance);
}

// Detect outlier using z-score
bool isOutlier(float value, const float* values, int count, float threshold_sigma) {
    if (count < 3 || values == nullptr) {
        return false;
    }
    
    float mean = calculateMovingAverage(values, count);
    float std_dev = calculateStdDev(values, count);
    
    if (std_dev == 0.0f) {
        return false;
    }
    
    float z_score = std::abs(value - mean) / std_dev;
    return z_score > threshold_sigma;
}

// Convert celsius to fahrenheit
float celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

// Convert fahrenheit to celsius
float fahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

// Convert celsius to kelvin
float celsiusToKelvin(float celsius) {
    return celsius + 273.15f;
}

// Convert kelvin to celsius
float kelvinToCelsius(float kelvin) {
    return kelvin - 273.15f;
}

// Calculate dew point from temperature and humidity
float calculateDewPoint(float temperature_c, float humidity_percent) {
    // Magnus formula
    const float a = 17.27f;
    const float b = 237.7f;
    
    float alpha = ((a * temperature_c) / (b + temperature_c)) + 
                  std::log(humidity_percent / 100.0f);
    float dew_point = (b * alpha) / (a - alpha);
    
    return dew_point;
}

// Calculate heat index (feels like temperature)
float calculateHeatIndex(float temperature_f, float humidity_percent) {
    // Simple heat index formula for temperatures above 80°F
    if (temperature_f < 80.0f) {
        return temperature_f;
    }
    
    float t = temperature_f;
    float rh = humidity_percent;
    
    float hi = -42.379f + 
               2.04901523f * t + 
               10.14333127f * rh - 
               0.22475541f * t * rh - 
               0.00683783f * t * t - 
               0.05481717f * rh * rh + 
               0.00122874f * t * t * rh + 
               0.00085282f * t * rh * rh - 
               0.00000199f * t * t * rh * rh;
    
    return hi;
}

// Calculate altitude from pressure (meters)
float calculateAltitude(float pressure_pa, float sea_level_pressure_pa) {
    // Barometric formula
    const float R = 8.31432f;  // Gas constant
    const float g = 9.80665f;  // Gravity
    const float M = 0.0289644f; // Molar mass of air
    const float T = 288.15f;   // Standard temperature (K)
    
    float altitude = (R * T / (g * M)) * 
                    std::log(sea_level_pressure_pa / pressure_pa);
    
    return altitude;
}

// Low-pass filter (simple RC filter)
float lowPassFilter(float new_value, float filtered_value, float alpha) {
    // y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
    return applyEMA(new_value, filtered_value, alpha);
}

// High-pass filter
float highPassFilter(float new_value, float old_value, float old_filtered, float alpha) {
    // y[n] = alpha * y[n-1] + alpha * (x[n] - x[n-1])
    float diff = new_value - old_value;
    return alpha * (old_filtered + diff);
}

// Median of three values
float medianOfThree(float a, float b, float c) {
    if (a > b) {
        if (b > c) return b;
        else if (a > c) return c;
        else return a;
    } else {
        if (a > c) return a;
        else if (b > c) return c;
        else return b;
    }
}

} // namespace SensorUtils

} // namespace sentinel