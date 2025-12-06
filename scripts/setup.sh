#!/bin/bash

# Sentinel Edge-AI Wildfire Detection System
# Installation and Setup Script
# 
# Usage: sudo ./scripts/setup.sh

set -e  # Exit on error

echo "========================================="
echo "Sentinel Edge-AI Setup Script"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: This script must be run as root (use sudo)"
    exit 1
fi

# Get the actual user (not root)
ACTUAL_USER=${SUDO_USER:-$USER}
USER_HOME=$(eval echo ~$ACTUAL_USER)

echo "Installing as user: $ACTUAL_USER"
echo ""

# Update system
echo "[1/8] Updating system packages..."
apt update && apt upgrade -y

# Install build tools
echo "[2/8] Installing build tools..."
apt install -y \
    cmake \
    build-essential \
    git \
    pkg-config \
    wget \
    curl

# Install OpenCV and dependencies
echo "[3/8] Installing OpenCV and vision libraries..."
apt install -y \
    libopencv-dev \
    python3-opencv \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libv4l-dev \
    libxvidcore-dev \
    libx264-dev

# Install I2C and SPI tools
echo "[4/8] Installing hardware interface libraries..."
apt install -y \
    i2c-tools \
    libi2c-dev \
    python3-smbus \
    spi-tools

# Install TensorFlow Lite
echo "[5/8] Installing TensorFlow Lite..."
if [ ! -d "/usr/local/lib/tensorflow" ]; then
    mkdir -p /tmp/tflite_install
    cd /tmp/tflite_install
    
    # Download TensorFlow Lite for ARM64
    wget https://github.com/tensorflow/tensorflow/releases/download/v2.13.0/tensorflow-lite-2.13.0-linux-aarch64.tar.gz
    
    # Extract and install
    tar -xzf tensorflow-lite-2.13.0-linux-aarch64.tar.gz
    
    # Copy libraries
    mkdir -p /usr/local/lib/tensorflow/lite
    cp -r include/* /usr/local/include/
    cp lib/* /usr/local/lib/
    
    # Update library cache
    ldconfig
    
    cd -
    rm -rf /tmp/tflite_install
    
    echo "TensorFlow Lite installed successfully"
else
    echo "TensorFlow Lite already installed"
fi

# Enable I2C and SPI
echo "[6/8] Enabling I2C and SPI interfaces..."

# Add to /boot/config.txt if not already present
if ! grep -q "dtparam=i2c_arm=on" /boot/firmware/config.txt; then
    echo "dtparam=i2c_arm=on" >> /boot/firmware/config.txt
fi

if ! grep -q "dtparam=spi=on" /boot/firmware/config.txt; then
    echo "dtparam=spi=on" >> /boot/firmware/config.txt
fi

# Load I2C and SPI kernel modules
modprobe i2c-dev
modprobe spi-bcm2835

# Add modules to load at boot
if ! grep -q "i2c-dev" /etc/modules; then
    echo "i2c-dev" >> /etc/modules
fi

if ! grep -q "spi-bcm2835" /etc/modules; then
    echo "spi-bcm2835" >> /etc/modules
fi

# Add user to i2c and spi groups
usermod -a -G i2c,spi,gpio $ACTUAL_USER

# Install RadioHead library for LoRa
echo "[7/8] Installing RadioHead library for LoRa..."
if [ ! -d "/usr/local/include/RadioHead" ]; then
    cd /tmp
    wget http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.120.zip
    unzip RadioHead-1.120.zip
    cd RadioHead-1.120
    
    # Copy headers
    mkdir -p /usr/local/include/RadioHead
    cp *.h /usr/local/include/RadioHead/
    
    cd -
    rm -rf /tmp/RadioHead-1.120*
    
    echo "RadioHead library installed"
else
    echo "RadioHead library already installed"
fi

# Create directories
echo "[8/8] Creating application directories..."
mkdir -p /var/log/sentinel
mkdir -p /var/lib/sentinel
mkdir -p /etc/sentinel

# Set ownership
chown -R $ACTUAL_USER:$ACTUAL_USER /var/log/sentinel
chown -R $ACTUAL_USER:$ACTUAL_USER /var/lib/sentinel
chown -R $ACTUAL_USER:$ACTUAL_USER /etc/sentinel

# Copy configuration files
if [ -f "configs/node_config.json" ]; then
    cp configs/node_config.json /etc/sentinel/
    chown $ACTUAL_USER:$ACTUAL_USER /etc/sentinel/node_config.json
fi

echo ""
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Reboot your Raspberry Pi: sudo reboot"
echo "2. After reboot, build the project:"
echo "   cd sentinel-edge-ai"
echo "   mkdir build && cd build"
echo "   cmake .."
echo "   make -j4"
echo "3. Run the application:"
echo "   sudo ./sentinel"
echo ""
echo "For hardware setup, see docs/HARDWARE_SETUP.md"
echo ""