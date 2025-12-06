#ifndef TFLITE_INFERENCE_H
#define TFLITE_INFERENCE_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

namespace sentinel {

struct InferenceResult {
    bool success;
    std::vector<float> output;
    float inference_time_ms;
};

struct ModelInfo {
    bool is_loaded;
    int input_height;
    int input_width;
    int input_channels;
    int output_size;
};

class TFLiteInference {
public:
    TFLiteInference();
    ~TFLiteInference();
    
    // Load model from file
    bool loadModel(const std::string& model_path);
    
    // Run inference with raw float data
    InferenceResult runInference(const float* input_data, size_t data_size);
    InferenceResult runInference(const std::vector<float>& input_data);
    
    // Get model dimensions
    void getInputDimensions(int& height, int& width, int& channels) const;
    void getOutputDimensions(int& size) const;
    
    // Configuration
    bool setNumThreads(int num_threads);
    
    // Model information
    ModelInfo getModelInfo() const;
    
    // Check if model is loaded and ready
    bool isInitialized() const;
    
    // Cleanup
    void shutdown();
    
    // Utility functions
    static std::vector<float> preprocessImage(const uint8_t* image_data,
                                             int height, int width, int channels,
                                             bool normalize = true);
    
    static std::vector<float> applySoftmax(const std::vector<float>& logits);
    static int getMaxProbabilityIndex(const std::vector<float>& probabilities);
    
private:
    std::unique_ptr<tflite::Interpreter> interpreter_;
    std::unique_ptr<tflite::FlatBufferModel> model_;
    
    bool is_initialized_;
    
    int input_tensor_idx_;
    int output_tensor_idx_;
    
    int input_batch_;
    int input_height_;
    int input_width_;
    int input_channels_;
};

} // namespace sentinel

#endif // TFLITE_INFERENCE_H