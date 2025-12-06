# Hardware Setup Guide

This guide provides detailed instructions for assembling and configuring the Sentinel Edge-AI Wildfire Detection System hardware.

## Table of Contents

1. [Bill of Materials](#bill-of-materials)
2. [Wiring Diagrams](#wiring-diagrams)
3. [Hardware Assembly](#hardware-assembly)
4. [Testing Components](#testing-components)
5. [Troubleshooting](#troubleshooting)

## Bill of Materials

### Core Components

| Item | Specification | Quantity | Est. Cost |
|------|--------------|----------|-----------|
| Raspberry Pi 5 | 4GB RAM | 1 | $60 |
| MicroSD Card | 32GB Class 10 | 1 | $10 |
| Raspberry Pi Camera v3 | 12MP | 1 | $25 |
| Flying Fish MQ-2 | Smoke/Gas Sensor | 1 | $5 |
| Ra-02 LoRa Module | SX1278, 433MHz | 1 | $8 |
| ADS1015 | 12-bit ADC | 1 | $10 |
| USB-C Power Supply | 5V 3A | 1 | $12 |
| Antenna | 433MHz | 1 | $5 |
| **Total** | | | **~$135** |

### Optional Components

- Weatherproof enclosure ($20)
- Solar panel + battery pack ($40)
- GPS module ($15)
- Temperature/humidity sensor ($8)

### Tools Required

- Soldering iron and solder
- Wire strippers
- Multimeter
- Breadboard (for prototyping)
- Jumper wires (male-to-female, male-to-male)

## Wiring Diagrams

### MQ-2 Sensor Connection (via ADS1015 ADC)

```
MQ-2 Sensor          ADS1015 ADC          Raspberry Pi 5
-----------          -----------          --------------
VCC (5V)    -------> VDD                  
GND         -------> GND        -------> GND (Pin 6)
A0          -------> A0
                     SCL        -------> GPIO 3 (SCL, Pin 5)
                     SDA        -------> GPIO 2 (SDA, Pin 3)
                     VDD        -------> 3.3V (Pin 1)
```

**Notes:**
- MQ-2 requires 5V power supply
- ADS1015 communicates via I2C (0x48 default address)
- Pull-up resistors (4.7kΩ) on SDA/SCL lines if needed

### LoRa Module (Ra-02) Connection

```
Ra-02 Module         Raspberry Pi 5
------------         --------------
VCC (3.3V)  -------> 3.3V (Pin 17)
GND         -------> GND (Pin 20)
MISO        -------> GPIO 9 (MISO, Pin 21)
MOSI        -------> GPIO 10 (MOSI, Pin 19)
SCK         -------> GPIO 11 (SCK, Pin 23)
NSS/CS      -------> GPIO 8 (CE0, Pin 24)
RST         -------> GPIO 25 (Pin 22)
DIO0        -------> GPIO 24 (Pin 18)
ANT         -------> Antenna connector
```

**Notes:**
- LoRa communicates via SPI
- Connect antenna before powering on
- DIO0 pin used for receive interrupts

### Camera Connection

- **Camera Module (CSI)**: Connect to the CSI camera port on Raspberry Pi
- **USB Camera**: Connect to any USB 3.0 port

### Complete Pinout Diagram

```
Raspberry Pi 5 GPIO Pinout (Top View)
=====================================
     3.3V [ 1] [ 2] 5V
  GPIO 2  [ 3] [ 4] 5V
  GPIO 3  [ 5] [ 6] GND
  GPIO 4  [ 7] [ 8] GPIO 14
      GND [ 9] [10] GPIO 15
 GPIO 17  [11] [12] GPIO 18
 GPIO 27  [13] [14] GND
 GPIO 22  [15] [16] GPIO 23
     3.3V [17] [18] GPIO 24  <--- LoRa DIO0
 GPIO 10  [19] [20] GND
  GPIO 9  [21] [22] GPIO 25  <--- LoRa RST
 GPIO 11  [23] [24] GPIO 8   <--- LoRa CS
      GND [25] [26] GPIO 7
  GPIO 0  [27] [28] GPIO 1
  GPIO 5  [29] [30] GND
  GPIO 6  [31] [32] GPIO 12
 GPIO 13  [33] [34] GND
 GPIO 19  [35] [36] GPIO 16
 GPIO 26  [37] [38] GPIO 20
      GND [39] [40] GPIO 21
```

## Hardware Assembly

### Step 1: Prepare the Raspberry Pi

1. Flash Raspberry Pi OS (64-bit) to MicroSD card:
   ```bash
   # Using Raspberry Pi Imager
   # Select: Raspberry Pi OS (64-bit)
   # Configure WiFi and SSH before flashing
   ```

2. Insert MicroSD card and boot the Pi

3. Enable I2C and SPI interfaces:
   ```bash
   sudo raspi-config
   # Interface Options -> I2C -> Enable
   # Interface Options -> SPI -> Enable
   # Interface Options -> Camera -> Enable
   ```

### Step 2: Assemble MQ-2 Sensor Circuit

1. **Solder headers** to ADS1015 ADC module if needed

2. **Connect MQ-2 to ADS1015**:
   - MQ-2 VCC → ADS1015 VDD
   - MQ-2 GND → ADS1015 GND  
   - MQ-2 A0 → ADS1015 A0

3. **Connect ADS1015 to Raspberry Pi**:
   - Use female-to-female jumper wires
   - Follow wiring diagram above
   - Ensure secure connections

### Step 3: Install LoRa Module

1. **Solder antenna connector** to Ra-02 module

2. **Connect Ra-02 to Raspberry Pi**:
   - Use female-to-female jumper wires
   - Follow SPI wiring diagram
   - **IMPORTANT**: Connect antenna before power on

3. **Mount antenna**:
   - Attach 433MHz antenna to ANT connector
   - Position vertically for optimal range

### Step 4: Install Camera

**For CSI Camera:**
1. Lift the tab on the CSI port
2. Insert ribbon cable with contacts facing away from tab
3. Close the tab firmly

**For USB Camera:**
1. Connect to USB 3.0 port (blue)

### Step 5: Power Supply

1. Connect USB-C power supply (5V 3A minimum)
2. Verify all components power on
3. Check for proper boot sequence

## Testing Components

### Test I2C Communication (MQ-2 Sensor)

```bash
# Install i2c-tools
sudo apt install i2c-tools

# Scan I2C bus
sudo i2cdetect -y 1

# Should show device at 0x48:
#      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
# 40: -- -- -- -- -- -- -- -- 48 -- -- -- -- -- -- --
```

### Test SPI Communication (LoRa Module)

```bash
# Check SPI devices
ls -l /dev/spidev*

# Should show:
# /dev/spidev0.0
# /dev/spidev0.1

# Test LoRa communication (requires RadioHead library)
# Run test program to verify module responds
```

### Test Camera

```bash
# List video devices
v4l2-ctl --list-devices

# Capture test image
libcamera-still -o test.jpg

# Or for USB camera:
fswebcam test.jpg
```

### Test MQ-2 Sensor Reading

```bash
# Read from ADC
cd sentinel-edge-ai/tests
./sensor_test

# Expected output:
# MQ-2 Sensor Test
# ================
# Analog Value: 1234
# Resistance: 8.45 kOhms
# PPM: 150.3
# Status: Normal (no smoke detected)
```

## Troubleshooting

### I2C Issues

**Problem**: No device detected at 0x48

**Solutions**:
1. Check wiring connections
2. Verify I2C is enabled: `sudo raspi-config`
3. Try different I2C address: `sudo i2cdetect -y 1`
4. Check pull-up resistors (4.7kΩ on SDA/SCL)

### SPI Issues

**Problem**: LoRa module not responding

**Solutions**:
1. Verify SPI is enabled
2. Check all SPI connections (MISO, MOSI, SCK, CS)
3. Verify 3.3V power supply
4. Ensure antenna is connected
5. Check RST pin connection

### Camera Issues

**Problem**: Camera not detected

**Solutions**:
1. Check ribbon cable connection
2. Verify camera is enabled: `sudo raspi-config`
3. Try: `vcgencmd get_camera` (should show detected=1)
4. For USB camera, try different USB port

### MQ-2 Sensor Issues

**Problem**: Erratic readings or always detecting smoke

**Solutions**:
1. Allow 24-48 hour burn-in period for new sensors
2. Calibrate in clean air environment
3. Check power supply (requires stable 5V)
4. Verify analog reading is in valid range (0-4095)
5. Replace sensor if values are extreme

### Power Issues

**Problem**: System reboots randomly

**Solutions**:
1. Use power supply with sufficient current (3A minimum)
2. Check for short circuits
3. Reduce number of connected peripherals
4. Add capacitor (1000μF) near power input

## Safety Considerations

- ⚠️ **Never power on LoRa module without antenna** - can damage transmitter
- ⚠️ **MQ-2 sensor gets hot during operation** - normal behavior
- ⚠️ **Operate in well-ventilated area during testing**
- ⚠️ **Do not expose to flammable gases during testing**
- ⚠️ **Use proper ESD precautions** when handling components

## Next Steps

After successful hardware assembly and testing:

1. Follow software installation in main [README.md](../README.md)
2. Run system tests: `cd build && ctest`
3. Deploy to field location
4. Configure multiple nodes for mesh network

## Additional Resources

- [Raspberry Pi GPIO Pinout](https://pinout.xyz/)
- [MQ-2 Datasheet](https://www.pololu.com/file/0J309/MQ2.pdf)
- [ADS1015 Datasheet](https://www.ti.com/lit/ds/symlink/ads1015.pdf)
- [SX1278 LoRa Module Datasheet](https://www.semtech.com/products/wireless-rf/lora-core/sx1278)