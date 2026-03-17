# Dynamic DC Load

**DIY Programmable Electronic Load with Battery Testing Capabilities**

A joint project with Bud Bennett featuring a professional-grade DC Dynamic Load with advanced features including runtime calibration, battery testing automation, and multiple operating modes.

![Version](https://img.shields.io/badge/Firmware-v7.1.2-blue)
![Hardware](https://img.shields.io/badge/Hardware-v5.1a-green)
![Battery Tester](https://img.shields.io/badge/Batt%20Tester-v2.1.0-orange)
![License](https://img.shields.io/badge/License-Open%20Source-brightgreen)

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [Specifications](#-specifications)
- [Repository Structure](#-repository-structure)
- [Quick Start](#-quick-start)
- [Documentation](#-documentation)
- [Blog Posts](#-blog-posts)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

The Dynamic DC Load is a precision electronic load designed for testing power supplies, batteries, and other DC sources. It features ESP32-based control with an OLED display, offering both manual operation and automated battery testing through a Python desktop application.

### What Makes This Special?

- **Runtime Calibration:** No recompilation needed - calibrate via serial commands
- **Battery Testing:** Automated testing with the included Python application
- **Multiple Modes:** CC, CV, CP, CR, and Battery Test modes
- **Real-time Control:** Hardware PWM with software regulation
- **Professional Features:** Temperature monitoring, fan control, safety limits
- **Open Source:** Complete schematics, PCB files, firmware, and software

---

## ✨ Key Features

### Hardware Features
- 🔋 **Input Range:** 1-100VDC with reverse polarity protection
- ⚡ **Current Capacity:** Up to 10A @ 40V (180W @ 25°C ambient)
- 🎯 **Accuracy:** ±0.3% voltage, ±0.4% current
- 🔒 **Safety:** Over-voltage, over-current, over-power, over-temperature protection
- 🌡️ **Thermal Management:** Dual temperature-controlled fans
- 🖥️ **Display:** 1.5" 128x128 OLED color display
- 🔘 **Control:** Rotary encoder with dual-button functions
- 📊 **Monitoring:** DSO trigger output (1V = 10A)

### Firmware Features (v7.1.2)
- 🎛️ **Runtime Calibration System:** Adjust calibration without recompiling
- 💾 **Persistent Storage:** ESP32 NVS storage survives firmware updates
- 🖥️ **Serial Interface:** 8 calibration commands via any terminal
- 🔄 **Operating Modes:**
  - **CC (Constant Current):** Hardware real-time regulation
  - **CV (Constant Voltage):** Hardware real-time regulation
  - **CP (Constant Power):** Active software regulation (±156µA resolution)
  - **CR (Constant Resistance):** Active software regulation (±156µA resolution)
  - **Battery Test:** Automated testing with data logging
- 🌀 **Smart Fan Control:** Temperature-based with calibration mode quiet operation
- 📦 **Binary Distribution:** Pre-compiled firmware for easy updates

### Software Features (Battery Tester App v2.1.0)
- 📈 **Real-time Visualization:** Live voltage, current, power, and capacity graphs
- 🛑 **Graceful Stop:** Stop button for safe test termination with data retention
- 📊 **Test Automation:** Configurable discharge and recovery cycles
- 🎯 **Smart Defaults:** Auto-calculated cutoff voltages and time estimates
- 📉 **Auto-ranging:** Automatic Y-axis scaling for optimal visualization
- 💾 **Data Export:** CSV export with sortable, analyzable test results
- ⚡ **Streamlined Protocol:** Optimized for firmware v7.1.2+ compatibility
- 🖥️ **Desktop Application:** Python/PyQt6 GUI (Windows, macOS, Linux)
- 🔋 **Multiple Cell Chemistries:** Supports various battery types (Li-ion, NiMH, etc.)
- 🔢 **Battery Packs:** Multiple cell configuration for battery pack testing

**Download:** [Battery Tester v2.1.0 Release](https://github.com/paulvee/Dynamic-DC-Load/releases/tag/batt-tester-v2.1.0)

---

## 🔧 Specifications

| Parameter | Specification |
|-----------|--------------|
| **Input Voltage** | 1-100 VDC |
| **Maximum Current** | 10A @ 40V |
| **Maximum Power** | 180W @ 25°C ambient (heatsink @ 85°C) |
| **Voltage Accuracy** | ±0.3% |
| **Current Accuracy** | ±0.4% |
| **Off-State Current** | 1.9µA @ 2V, 57.7µA @ 60V |
| **Reverse Polarity** | Protected to -100V |
| **Power Supply** | 12 VDC @ 0.5A (wall adapter) |
| **Dimensions** | 21cm × 18cm × 11cm |
| **Weight** | ~1110 grams |
| **Cooling** | Dual temperature-controlled fans |
| **Display** | 128×128 OLED, 1.5", full color |

---

## 📁 Repository Structure

```
Dynamic-DC-Load/
│
├── firmware/                  # ESP32 Firmware
│   ├── src/                   # Source files
│   ├── include/               # Header files
│   ├── releases/              # Pre-compiled binaries
│   │   └── v7.1.0/           # Latest release
│   ├── CALIBRATION_GUIDE.md   # Detailed calibration procedures
│   ├── CHANGELOG.md           # Version history
│   └── README.md             # Firmware documentation
│
├── Batt-Test-App/            # Python Battery Tester Application v2.1.0
│   ├── gui/                  # PyQt6 user interface
│   ├── hardware/             # Serial communication protocol
│   ├── core/                 # Data models and business logic
│   ├── utils/                # Configuration and export utilities
│   └── README.md             # Application documentation
│
├── hardware/                  # Hardware Design Files
│   ├── schematics/           # Circuit diagrams (PDF, PNG)
│   ├── pcb/                  # KiCAD PCB files
│   ├── bom/                  # Bill of Materials
│   ├── enclosure/            # 3D-printable enclosure
│   └── README.md            # Hardware documentation
│
├── docs/                      # Documentation & Media
│   ├── images/               # Photos and screenshots
│   └── README.md            # Documentation index
│
├── archive/                   # Historical versions
│   └── [Old firmware and code]
│
└── README.md                 # This file
```

---

## 🚀 Quick Start

### Option 1: Pre-compiled Firmware (Recommended for Users)

1. **Download firmware binaries:**
   - Navigate to [`firmware/releases/v7.1.0/`](firmware/releases/v7.1.0/)
   - Download all files

2. **Flash to ESP32:**
   - **Windows:** Run `flash_firmware.bat`
   - **Linux/Mac:** Use esptool.py (see [instructions](firmware/releases/v7.1.0/README.md))

3. **Calibrate (if needed):**
   - See [`firmware/releases/v7.1.0/QUICK_START.txt`](firmware/releases/v7.1.0/QUICK_START.txt)

### Option 2: Build from Source (For Developers)

1. **Clone repository:**
   ```bash
   git clone https://github.com/paulvee/Dynamic-DC-Load.git
   cd Dynamic-DC-Load/firmware
   ```

2. **Install PlatformIO:**
   - [Download PlatformIO](https://platformio.org/)

3. **Build and upload:**
   ```bash
   pio run --target upload
   ```

### Option 3: Build Hardware

1. **Review hardware documentation:**
   - See [`hardware/README.md`](hardware/README.md)

2. **Order components:**
   - Use BOM files in [`hardware/bom/`](hardware/bom/)

3. **Fabricate PCBs:**
   - Send files from [`hardware/pcb/`](hardware/pcb/) to PCB manufacturer

4. **3D print enclosure:**
   - Files in [`hardware/enclosure/`](hardware/enclosure/)

5. **Assemble and test:**
   - Follow build blog post (see below)

---

## 📚 Documentation

### Firmware
- **[Firmware README](firmware/README.md)** - Build and flash instructions
- **[Calibration Guide](firmware/CALIBRATION_GUIDE.md)** - Complete calibration procedures
- **[Changelog](firmware/CHANGELOG.md)** - Version history and updates
- **[Quick Start](firmware/releases/v7.1.0/QUICK_START.txt)** - Calibration quick reference
- **[Release Notes](firmware/releases/v7.1.0/VERSION_INFO.txt)** - Detailed release information

### Hardware
- **[Hardware README](hardware/README.md)** - Assembly and specifications
- **[Schematics](hardware/schematics/)** - Circuit diagrams
- **[PCB Files](hardware/pcb/)** - KiCAD design files
- **[BOM](hardware/bom/)** - Component lists

### Battery Tester App
Desktop application for automated battery discharge testing and analysis.

**Quick Start:**
1. Download [BatteryTester.exe](https://github.com/paulvee/Dynamic-DC-Load/releases/tag/batt-tester-v2.1.0) (v2.1.0)
2. Connect your Dynamic Load via USB
3. Configure test parameters (cutoff voltage, discharge current)
4. Monitor real-time graphs and data
5. Export results to CSV for analysis

**Requirements:**
- Dynamic Load firmware v7.1.2 or higher
- Windows 10/11 (executable) or Python 3.9+ on Windows/macOS/Linux (source)

**Documentation:** See [`Batt-Test-App/README.md`](Batt-Test-App/README.md)

**Features:**
- Automated discharge and recovery testing
- Real-time graphing (voltage, current, power, capacity)
- Stop button for graceful test termination
- Auto-ranging displays and automatic calculations
- CSV data export with analysis

---

## 📝 Blog Posts

Detailed project documentation on Paul's DIY Electronics Blog:

- **[Building the DIY Dynamic DC Load](https://www.paulvdiyblogs.net/2024/09/building-diy-dynamic-dc-load.html)**  
  Complete assembly guide with photos and testing procedures

- **[Design Process: DIY DC Dynamic Load Instrument](https://www.paulvdiyblogs.net/2024/04/build-diy-dc-dynamic-load-instrument.html)**  
  Design decisions, circuit analysis, and development process

---

## 🛠️ Calibration System

The firmware includes a runtime calibration system - no recompilation needed!

### Enter Calibration Mode
1. Power off the DL and remove the serial cable
2. Hold encoder button
3. Power on (keep holding)
4. Wait for "CALIBRATION MODE" screen
5. Release button and add the serial cable

### Serial Commands (9600 baud)
```
CAL           - Show command menu
CAL SHOW      - Display current values
CAL CV <val>  - Set CV calibration
CAL SAVE      - Save to NVS
CAL EXIT      - Exit (requires power cycle)
```

**Full Guide:** [`firmware/CALIBRATION_GUIDE.md`](firmware/CALIBRATION_GUIDE.md)

---

## 🤝 Contributing

This is a joint project with **Bud Bennett**. Contributions are welcome!

### Ways to Contribute
- 🐛 Report bugs via [GitHub Issues](https://github.com/paulvee/Dynamic-DC-Load/issues)
- 💡 Suggest features or improvements
- 📝 Improve documentation
- 🔧 Submit hardware modifications
- 💻 Contribute code improvements

### Before Contributing
- Review existing issues and pull requests
- Test your changes thoroughly
- Document your modifications
- Follow existing code style

---

## ⚠️ Safety Warning

**This device handles high currents and power. Improper use can result in fire, component damage, or injury.**

**Required Safety Measures:**
- ✅ Proper heatsinks with thermal compound
- ✅ Adequate ventilation (fans operating)
- ✅ Appropriate wire gauge for current
- ✅ Never exceed ratings
- ✅ Test at low power first
- ✅ Keep away from flammable materials

**Always:**
- Monitor temperature during operation
- Use in well-ventilated area
- Have fire extinguisher nearby when testing

---

## 📜 License

Open source project - see LICENSE file for details.

**Note:** If you build this project or use it in your work, please credit Paul Versteeg and Bud Bennett.

---

## 🔗 Links

- **GitHub Repository:** [https://github.com/paulvee/Dynamic-DC-Load](https://github.com/paulvee/Dynamic-DC-Load)
- **Paul's Blog:** [https://www.paulvdiyblogs.net/](https://www.paulvdiyblogs.net/)
- **Issues & Support:** [GitHub Issues](https://github.com/paulvee/Dynamic-DC-Load/issues)

---

## 📸 Gallery

See [`docs/images/`](docs/images/) for build photos and test results.

---

## 🎯 Project Status

| Component | Status | Version |
|-----------|--------|---------|
| **Firmware** | ✅ Stable | v7.1.1 |
| **Hardware** | ✅ Stable | v5.1a |
| **Battery Tester App** | 🚧 Coming Soon | TBD |
| **Documentation** | ✅ Complete | Current |

---

## 🙏 Acknowledgments

- **Bud Bennett** - Co-designer and hardware expert
- **Community Contributors** - Testing and feedback
- **Open Source Libraries** - See firmware documentation

---

**Built with ❤️ by DIY Electronics Enthusiasts**

**Copyright © 2026 Paul Versteeg & Bud Bennett**

---

*For questions, support, or to share your build, visit the [GitHub Issues](https://github.com/paulvee/Dynamic-DC-Load/issues) page or check out Paul's blog!*

