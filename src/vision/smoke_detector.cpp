#include "vision/smoke_detector.h"
#include "vision/tflite_inference.h"
#include "utils/logger.h"
// #include <opencv2/opencv.hpp>
// #include <opencv2/imgproc.hpp>

namespace sentinel {

SmokeDetector::SmokeDetector(const std::string& model_path)
    : model_path_(model_path),
      inference_engine_(nullptr),
      is_initialized_(false),
      input_height_(224),
      input_width_(224),
      input_channels_(3) {
}

SmokeDetector::~SmokeDetector() {
    shutdown();
}

bool SmokeDetector::initialize() {
    Logger::info("Initializing Smoke Detector with model: " + model_path_);
    
    // Initialize TFLite inference engine
    inference_engine_ = std::make_unique<TFLiteInference>();
    
    if (!inference_engine_->loadModel(model_path_)) {
        Logger::error("Failed to load TFLite model");
        return false;
    }
    
    // Get model input dimensions
    inference_engine_->getInputDimensions(input_height_, input_width_, input_channels_);
    
    Logger::info("Model input shape: " + std::to_string(input_height_) + "x" +
                std::to_string(input_width_) + "x" + 
                std::to_string(input_channels_));
    
    // Set number of threads for inference (optimize for Raspberry Pi)
    inference_engine_->setNumThreads(2);
    
    // Initialize camera
    camera_.open(0); // Open default camera
    if (!camera_.isOpened()) {
        Logger::error("Failed to open camera");
        return false;
    }
    
    // Set camera properties
    camera_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    camera_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    camera_.set(cv::CAP_PROP_FPS, 30);
    
    is_initialized_ = true;
    Logger::info("Smoke Detector initialized successfully");
    return true;
}

DetectionResult SmokeDetector::detectSmoke() {
    DetectionResult result;
    result.detected = false;
    result.confidence = 0.0f;
    result.smoothed_confidence = 0.0f;
    result.timestamp = std::chrono::system_clock::now();
    
    if (!is_initialized_) {
        Logger::error("Detector not initialized");
        return result;
    }
    
    // Capture frame
    cv::Mat frame;
    if (!camera_.read(frame) || frame.empty()) {
        Logger::error("Failed to capture frame");
        return result;
    }
    
    // Preprocess frame
    cv::Mat processed = preprocessFrame(frame);
    
    // Convert to float array for inference
    std::vector<float> input_data(input_height_ * input_width_ * input_channels_);
    
    // Copy pixel data (already normalized in preprocessFrame)
    int idx = 0;
    for (int y = 0; y < input_height_; y++) {
        for (int x = 0; x < input_width_; x++) {
            cv::Vec3f pixel = processed.at<cv::Vec3f>(y, x);
            input_data[idx++] = pixel[0]; // R
            input_data[idx++] = pixel[1]; // G
            input_data[idx++] = pixel[2]; // B
        }
    }
    
    // Run inference
    auto inference_result = inference_engine_->runInference(input_data);
    
    if (!inference_result.success) {
        Logger::error("Inference failed");
        return result;
    }
    
    // Extract smoke detection probability
    // Assuming binary classification: [no_smoke_prob, smoke_prob]
    if (inference_result.output.size() >= 2) {
        result.confidence = inference_result.output[1]; // Smoke probability
    } else if (inference_result.output.size() == 1) {
        result.confidence = inference_result.output[0]; // Single output
    }
    
    result.detected = (result.confidence > CONFIDENCE_THRESHOLD);
    result.inference_time_ms = inference_result.inference_time_ms;
    
    // Apply temporal smoothing
    confidence_history_.push_back(result.confidence);
    if (confidence_history_.size() > 10) {
        confidence_history_.erase(confidence_history_.begin());
    }
    
    // Calculate smoothed confidence
    float smoothed_confidence = 0.0f;
    for (float c : confidence_history_) {
        smoothed_confidence += c;
    }
    smoothed_confidence /= confidence_history_.size();
    
    result.smoothed_confidence = smoothed_confidence;
    result.detected = (smoothed_confidence > CONFIDENCE_THRESHOLD);
    
    return result;
}

cv::Mat SmokeDetector::preprocessFrame(const cv::Mat& frame) {
    cv::Mat processed;
    
    // Resize to model input size
    cv::resize(frame, processed, cv::Size(input_width_, input_height_));
    
    // Convert BGR to RGB
    cv::cvtColor(processed, processed, cv::COLOR_BGR2RGB);
    
    // Convert to float and normalize pixel values to [0, 1]
    processed.convertTo(processed, CV_32F, 1.0 / 255.0);
    
    return processed;
}

cv::Mat SmokeDetector::captureFrame() {
    cv::Mat frame;
    if (camera_.isOpened()) {
        camera_.read(frame);
    }
    return frame;
}

void SmokeDetector::saveFrame(const cv::Mat& frame, const std::string& filename) {
    if (!frame.empty()) {
        cv::imwrite(filename, frame);
        Logger::info("Frame saved: " + filename);
    } else {
        Logger::error("Cannot save empty frame");
    }
}

std::vector<float> SmokeDetector::getConfidenceHistory() const {
    return confidence_history_;
}

void SmokeDetector::clearHistory() {
    confidence_history_.clear();
}

void SmokeDetector::shutdown() {
    if (camera_.isOpened()) {
        camera_.release();
    }
    
    if (inference_engine_) {
        inference_engine_->shutdown();
    }
    
    is_initialized_ = false;
    Logger::info("Smoke Detector shutdown complete");
}

} // namespace sentinel