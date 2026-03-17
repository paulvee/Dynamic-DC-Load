# Dynamic Load Firmware v7.1.1 - Installation Guide

This package contains pre-compiled firmware binaries for the ESP32 Dynamic Load controller.

## What's New in v7.1.1

### Bug Fixes
- ✅ **Fan control in calibration mode** - Fan now properly stops during calibration mode
  - Fixed PWM channel initialization order in boot sequence
  - Fan reliably stops for quiet operation during calibration

### Changes
- ✅ **Version numbering** - Switched to numeric patch versions (X.Y.Z format)
  - No more letter suffixes for cleaner semantic versioning

### v7.1.0 Features (included)
- ✅ **Runtime calibration system** - Calibrate without recompilation
- ✅ **Boot-time calibration mode** - Hold encoder button during power-on
- ✅ **ESP32 Preferences storage** - Calibration persists across firmware updates
- ✅ **Serial command interface** - 8 calibration commands via serial terminal
- ✅ **Binary distribution friendly** - No development tools needed for calibration

See QUICK_START.txt for calibration instructions.

---

## Prerequisites

### Windows - Quick Method (Recommended)
Use the included **flash_firmware.bat** script:
1. Connect ESP32 via USB
2. Double-click `flash_firmware.bat`
3. Script will automatically detect COM port and flash firmware

### Manual Method - All Platforms

#### Install Python & esptool
1. Install Python from https://www.python.org/downloads/
   - During installation, check "Add Python to PATH"

2. Install esptool:
   ```bash
   pip install esptool
   ```

#### Flash Firmware

**Windows:**
```cmd
esptool.py --chip esp32 --port COM4 --baud 921600 ^
  --before default_reset --after hard_reset write_flash -z ^
  --flash_mode dio --flash_freq 80m --flash_size 4MB ^
  0x1000 bootloader.bin ^
  0x8000 partitions.bin ^
  0xe000 boot_app0.bin ^
  0x10000 firmware.bin
```

**Linux/macOS:**
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Replace `/dev/ttyUSB0` with your actual port (`/dev/ttyUSB0`, `/dev/cu.SLAB_USBtoUART`, etc.)

---

## Finding Your COM Port

### Windows
1. Open Device Manager
2. Expand "Ports (COM & LPT)"
3. Look for "Silicon Labs CP210x USB to UART Bridge" or "CH340"
4. Note the COM port number (e.g., COM4)

### Linux
```bash
ls /dev/ttyUSB*
# or
dmesg | grep tty
```

### macOS
```bash
ls /dev/cu.*
# Look for cu.SLAB_USBtoUART or similar
```

---

## Troubleshooting

### "Failed to connect"
1. **Hold BOOT button** on ESP32 when starting flash process
2. Release BOOT button after "Connecting..." message appears
3. Try a different USB cable (some are power-only)
4. Try lower baud rate: change `921600` to `115200`

### "Permission denied" (Linux/macOS)
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
# Log out and log back in

# Or use sudo
sudo esptool.py ...
```

### Driver Issues (Windows)
- Install CP210x drivers: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- Or CH340 drivers: https://sparks.gogo.co.nz/ch340.html

### Wrong COM Port
- Close any programs using the serial port (Arduino IDE, PuTTY, etc.)
- Unplug and replug ESP32 to refresh connection
- Check Device Manager for correct COM port

---

## Post-Flash Instructions

1. **Power cycle** the ESP32 (unplug and replug USB)

2. **Verify firmware version**:
   - Open serial monitor at 9600 baud
   - Power on ESP32
   - Should see: "Dynamic DC Load - Version 7.1.1"

3. **Calibration** (if needed):
   - See QUICK_START.txt for calibration mode instructions
   - See CALIBRATION_GUIDE.md in main repository for detailed procedures

4. **Normal operation**:
   - Connect to battery test Python application
   - Or use manual controls with rotary encoder

---

## Support & Documentation

- **Main Repository**: https://github.com/paulvee/DL-Firmware-for-VSC
- **Calibration Guide**: See CALIBRATION_GUIDE.md in repository
- **Battery Tester App**: https://github.com/paulvee/BatteryTester

---

## Technical Details

- **Firmware Version**: 7.1.1
- **Build Date**: March 16, 2026
- **Platform**: ESP32 DevKit1 (ESP32-D0WD-V3)
- **Flash Size**: 4MB
- **Partition Scheme**: Default (OTA updates supported)
- **Framework**: Arduino for ESP32

**Binary Sizes:**
- Bootloader: 17,536 bytes @ 0x1000
- Partitions: 3,072 bytes @ 0x8000
- Boot App: 8,192 bytes @ 0xe000
- Firmware: 363,073 bytes @ 0x10000

**Memory Usage:**
- Flash: 27.7% (363,073 / 1,310,720 bytes)
- RAM: 7.2% (23,544 / 327,680 bytes)

---

*Note: Flashing new firmware does NOT erase calibration values stored in ESP32 NVS (Preferences).*
