#ifndef SMOKE_DETECTOR_H
#define SMOKE_DETECTOR_H

#include <string>
#include <memory>
#include <vector>
#include <chrono>
// #include <opencv2/core.hpp>
// #include <opencv2/videoio.hpp>

namespace sentinel {

// Forward declaration
class TFLiteInference;

struct DetectionResult {
    bool detected;
    float confidence;
    float smoothed_confidence;
    float inference_time_ms;
    std::chrono::system_clock::time_point timestamp;
};

class SmokeDetector {
public:
    explicit SmokeDetector(const std::string& model_path);
    ~SmokeDetector();
    
    // Initialize vision system and load model
    bool initialize();
    
    // Perform smoke detection on current frame
    DetectionResult detectSmoke();
    
    // Capture a single frame from camera
    cv::Mat captureFrame();
    
    // Save frame to disk
    void saveFrame(const cv::Mat& frame, const std::string& filename);
    
    // Get confidence history
    std::vector<float> getConfidenceHistory() const;
    
    // Clear confidence history
    void clearHistory();
    
    // Cleanup
    void shutdown();
    
private:
    // Preprocess frame for model input
    cv::Mat preprocessFrame(const cv::Mat& frame);
    
    std::string model_path_;
    std::unique_ptr<TFLiteInference> inference_engine_;
    
    cv::VideoCapture camera_;
    
    bool is_initialized_;
    int input_height_;
    int input_width_;
    int input_channels_;
    
    std::vector<float> confidence_history_;
    
    static constexpr float CONFIDENCE_THRESHOLD = 0.75f;
};

} // namespace sentinel

#endif // SMOKE_DETECTOR_H