cmake_minimum_required(VERSION 3.20)
project(sentinel-edge-ai VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3")

# Enable debug symbols in debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

# Find required packages
find_package(OpenCV 4 REQUIRED)
find_package(Threads REQUIRED)

# TensorFlow Lite (assuming it's installed in a standard location)
# Adjust path as needed for your system
set(TFLITE_INCLUDE_DIR "/usr/local/include/tensorflow/lite")
set(TFLITE_LIB_DIR "/usr/local/lib")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${TFLITE_INCLUDE_DIR}
    /usr/include
)

# Link directories
link_directories(
    ${TFLITE_LIB_DIR}
)

# Source files
set(SOURCES
    src/core/sentinel_core.cpp
    src/core/config_manager.cpp
    src/sensors/mq2_sensor.cpp
    src/vision/smoke_detector.cpp
    src/network/lora_mesh.cpp
    src/utils/logger.cpp
    src/utils/data_processor.cpp
)

# Header files
set(HEADERS
    include/sentinel_core.h
    include/sensors/mq2_sensor.h
    include/sensors/sensor_interface.h
    include/vision/smoke_detector.h
    include/network/lora_mesh.h
    include/utils/logger.h
    include/utils/data_processor.h
)

# Main executable
add_executable(sentinel ${SOURCES})

# Link libraries
target_link_libraries(sentinel
    ${OpenCV_LIBS}
    Threads::Threads
    tensorflowlite
    i2c
    pthread
)

# Installation rules
install(TARGETS sentinel
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include/sentinel
)

install(DIRECTORY configs/
    DESTINATION etc/sentinel
)

install(DIRECTORY models/
    DESTINATION share/sentinel/models
)

# Testing (optional)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
        )
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Sentinel Edge-AI Wildfire Detection System")
message(STATUS "==========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")