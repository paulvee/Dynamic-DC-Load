# Dynamic Load Firmware v7.0.0 - Installation Guide

This package contains pre-compiled firmware binaries for the ESP32 Dynamic Load controller.

## What's New in v7.0.0
- Dual-core optimizations for improved performance
- ISR core affinity fixes for efficient encoder response
- Increased process_encoder task priority for immediate response
- Enhanced thread safety with comprehensive mutex protection
- Optimized OLED display task with larger stack
- Fixed BatteryMode termination message logic

## Prerequisites

### Option 1: Python + esptool (Recommended - Easy)
1. **Install Python** (if not already installed)
   - Download from: https://www.python.org/downloads/
   - During installation, check "Add Python to PATH"

2. **Install esptool**
   ```cmd
   pip install esptool
   ```

3. **Install serial tools** (for COM port detection)
   ```cmd
   pip install pyserial
   ```

### Option 2: ESP Flash Download Tool (No Python needed)
1. Download Espressif Flash Download Tool:
   https://www.espressif.com/en/support/download/other-tools
2. Extract the ZIP file

## Flashing Instructions

### Method 1: Using the Batch Script (Easiest)

1. Connect your ESP32 to the computer via USB
2. Double-click `flash_firmware.bat`
3. Follow the on-screen prompts:
   - Note the COM port number of your ESP32
   - Enter the COM port when asked (e.g., COM3)
4. Wait for flashing to complete (~30 seconds)
5. Done! The ESP32 will restart automatically

### Method 2: Manual Command Line

1. Open Command Prompt in this folder
2. Run the following command (replace COMx with your port):

```cmd
python -m esptool --chip esp32 --port COMx --baud 460800 write_flash -z 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin
```

Example:
```cmd
python -m esptool --chip esp32 --port COM3 --baud 460800 write_flash -z 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin
```

### Method 3: ESP Flash Download Tool (GUI)

1. Run the Flash Download Tool
2. Select "ESP32" chip type
3. Configure the files and addresses:
   - `bootloader.bin` @ 0x1000
   - `partitions.bin` @ 0x8000
   - `boot_app0.bin` @ 0xe000
   - `firmware.bin` @ 0x10000
4. Select your COM port and set baud rate to 460800
5. Click "START" to flash

## Finding Your COM Port

### Windows:
1. Open Device Manager (Win+X → Device Manager)
2. Expand "Ports (COM & LPT)"
3. Look for "USB-SERIAL CH340" or "CP210x" or "Silicon Labs"
4. Note the COM port number (e.g., COM3)

### Alternative - Using Python:
```cmd
python -m serial.tools.list_ports
```

## Troubleshooting

### "Failed to connect to ESP32"
**Solutions:**
- Hold the BOOT button on the ESP32 during flashing
- Try a different USB cable (must support data, not just power)
- Try a different USB port on your computer
- Reduce baud rate to 115200 (change in command/script)

### "esptool not found"
**Solution:**
```cmd
pip install esptool
```

### "Access denied" or "Port busy"
**Solutions:**
- Close the Battery Tester Python application
- Close Arduino IDE or PlatformIO if open
- Unplug and replug the ESP32
- Restart your computer

### Wrong COM Port
**Solution:**
- Check Device Manager for the correct port
- Try unplugging/replugging to see which port appears

## Verification

After flashing successfully:
1. Open Serial Monitor (115200 baud)
2. You should see boot messages showing:
   - "Dynamic Load v7.0.0"
   - Task initialization messages
   - Sensor initialization
   - "Ready" status

## File Descriptions

- **firmware.bin** - Main application code (337 KB)
- **bootloader.bin** - ESP32 bootloader
- **partitions.bin** - Partition table
- **boot_app0.bin** - Boot application selector
- **flash_firmware.bat** - Automated Windows flash script
- **README.md** - This file

## Technical Specifications

- **Platform:** ESP32 DevKit (dual-core Xtensa LX6)
- **Flash Size:** 4MB
- **RAM Usage:** 7.2% (23,480 bytes)
- **Flash Usage:** 25.8% (337,877 bytes)
- **Build Date:** March 8, 2026
- **Version:** 7.0.0

## Support

If you encounter issues:
1. Check the troubleshooting section above
2. Verify all prerequisites are installed
3. Try the alternative flashing methods
4. Contact Paul for assistance

## Hardware Compatibility

This firmware is designed for:
- ESP32 DevKit (38-pin boards)
- ADS1115 16-bit ADC (I2C 0x48)
- DAC8571 16-bit DAC (I2C 0x4C)
- SSD1351 128x128 OLED Display (SPI)
- Rotary Encoder with button
- PWM Fan Control

---

**Firmware Version:** 7.0.0  
**Compiled:** March 8, 2026  
**Compiler:** PlatformIO, ESP32 Arduino Framework  
**Author:** Paul Versteeg
