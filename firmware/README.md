# Dynamic DC Load - ESP32 Firmware

**Current Version:** 7.1.2  
**Release Date:** March 17, 2026  
**Platform:** ESP32 DevKit1  

This directory contains the ESP32 firmware for the Dynamic DC Load controller.

---

## 🚀 Quick Start

### For End Users (Pre-compiled Binaries)
If you just want to flash the firmware without building from source:

1. Go to [**releases/v7.1.2/**](releases/v7.1.2/)
2. Download all files in that folder
3. Follow instructions in [releases/v7.1.2/README.md](releases/v7.1.2/README.md)
4. For calibration, see [releases/v7.1.2/QUICK_START.txt](releases/v7.1.2/QUICK_START.txt)

### For Developers (Build from Source)
If you want to modify or build the firmware yourself:

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repository
3. Open the `firmware/` folder in VS Code with PlatformIO extension
4. Build with `pio run`
5. Upload with `pio run --target upload`

**Required Libraries** (auto-installed by PlatformIO):
- Preferences @ 2.0.0
- ezButton @ 1.0.6
- Adafruit SSD1351 @ 1.3.3
- Adafruit GFX @ 1.12.5
- ADS1X15 @ 0.5.4
- DAC8571 @ 0.1.3
- SPI, Wire (built-in)

---

## 📋 What's New in v7.1.2

### Changes
- ✅ **Battery Tester Protocol** - Optimized serial communication for Battery Tester v2.1.0+
  - Streamlined data reporting for improved performance
  - Enhanced compatibility with new Battery Tester features
  - Watchdog reset improvements
  - 5-second baseline monitoring

### Previous Features (v7.1.1 and v7.1.0 Included)
- 🎛️ **Runtime calibration system** - Calibrate without recompilation
- 💾 **ESP32 Preferences (NVS)** - Persistent storage across firmware updates
- 🖥️ **Serial command interface** - 8 calibration commands via terminal
- 📦 **Binary-friendly** - No dev tools needed for calibration

See [CHANGELOG.md](CHANGELOG.md) for complete version history.

---

## 📁 Directory Structure

```
firmware/
├── src/                      # Source files (.cpp)
│   ├── main.cpp              # Main program entry
│   ├── BatteryMode.cpp       # Battery test mode
│   ├── CalibrationManager.cpp# Calibration system
│   ├── Debug.cpp             # Debug utilities
│   ├── FanController.cpp     # Fan control
│   ├── OLEDDisplay.cpp       # Display management
│   └── RotaryEncoder.cpp     # Encoder input
│
├── include/                  # Header files (.h)
│   ├── Config.h              # System configuration & pins
│   └── [corresponding .h files]
│
├── lib/                      # Local libraries
│   └── MovingAverage/        # Moving average filter
│
├── data/                     # Data files
│   ├── README.md             # Calibration user guide
│   └── DL_Cal_Values.ini     # Calibration template (reference)
│
├── releases/                 # Pre-compiled binaries
│   └── v7.1.2/               # Latest release
│       ├── firmware.bin      # Main firmware
│       ├── bootloader.bin    # ESP32 bootloader
│       ├── partitions.bin    # Partition table
│       ├── flash_firmware.bat# Flash script (Windows)
│       ├── README.md         # Flash instructions
│       ├── QUICK_START.txt   # Calibration quick ref
│       └── VERSION_INFO.txt  # Detailed changelog
│
├── platformio.ini            # PlatformIO configuration
├── increment_version.py      # Version auto-increment script
├── CHANGELOG.md              # Complete version history
├── CALIBRATION_GUIDE.md      # Detailed calibration procedures
├── TEST_PLAN.md              # Testing documentation
└── README.md                 # This file
```

---

## 🔧 Calibration System

The firmware includes a comprehensive runtime calibration system that doesn't require recompilation.

### Entering Calibration Mode
1. Power off ESP32
2. Press and **hold** encoder button
3. Power on (keep holding)
4. Wait for "CALIBRATION MODE" screen
5. Release button

### Serial Commands (9600 baud)
- `CAL` - Show command menu
- `CAL SHOW` - Display current values
- `CAL CV <value>` - Set CV mode calibration
- `CAL DUTV <value>` - Set voltage calibration
- `CAL DUTC <value>` - Set DAC-ADC calibration
- `CAL SHUNT <value>` - Set current calibration
- `CAL SAVE` - Save to NVS (required!)
- `CAL RESET` - Restore defaults
- `CAL EXIT` - Exit (power cycle required)

### Documentation
- **Quick Reference:** [releases/v7.1.2/QUICK_START.txt](releases/v7.1.2/QUICK_START.txt)
- **Complete Guide:** [CALIBRATION_GUIDE.md](CALIBRATION_GUIDE.md)
- **User Guide:** [data/README.md](data/README.md)

---

## 🛠️ Building from Source

### Prerequisites
- [PlatformIO Core](https://platformio.org/) or PlatformIO IDE
- Python 3.x (for PlatformIO)
- USB cable for ESP32

### Build Commands
```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor

# Clean build
pio run --target clean
```

### Configuration
Main configuration is in [include/Config.h](include/Config.h):
- Pin assignments
- PWM settings
- Default calibration values
- Feature flags

---

## 📊 Technical Specifications

- **Platform:** ESP32-D0WD-V3 @ 240MHz dual-core
- **Framework:** Arduino for ESP32
- **Flash Usage:** ~363KB (27.7%)
- **RAM Usage:** ~24KB (7.2%)
- **Serial Baud:** 9600 (battery test mode compatible)
- **Display:** SSD1351 128x128 OLED
- **ADC:** ADS1015 12-bit external ADC
- **DAC:** DAC8571 16-bit external DAC

---

## 🔗 Related Components

This firmware works with:

1. **Battery Tester App** - Python desktop application
   - See: `../Batt-Test-App/` folder
   - Real-time monitoring and data logging
   - Automated battery testing

2. **Hardware** - PCB and enclosure
   - See: `../hardware/` folder
   - Schematics, PCB files, BOM
   - 3D-printed enclosure designs

---

## 📖 Documentation

- [CHANGELOG.md](CHANGELOG.md) - Complete version history
- [CALIBRATION_GUIDE.md](CALIBRATION_GUIDE.md) - Detailed calibration procedures
- [TEST_PLAN.md](TEST_PLAN.md) - Testing and validation
- [data/README.md](data/README.md) - User calibration guide
- [releases/v7.1.2/README.md](releases/v7.1.2/README.md) - Flash instructions
- [releases/v7.1.2/VERSION_INFO.txt](releases/v7.1.2/VERSION_INFO.txt) - Release notes

---

## 🐛 Known Issues

None.

---

## 📝 License

See main repository LICENSE file.

---

## 💬 Support

For questions, issues, or contributions:
- **GitHub Issues:** [https://github.com/paulvee/Dynamic-DC-Load/issues](https://github.com/paulvee/Dynamic-DC-Load/issues)
- **Main Repository:** [https://github.com/paulvee/Dynamic-DC-Load](https://github.com/paulvee/Dynamic-DC-Load)

---

**Copyright © 2026 Paul Versteeg**
