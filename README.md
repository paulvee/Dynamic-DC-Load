# Dynamic DC Load Firmware

**Version:** 7.1.2  
**Platform:** ESP32 DevKit1 + PlatformIO  
**Author:** Paul Versteeg

Professional C++ firmware for the DIY Dynamic DC Load electronic load controller.

## Overview

This firmware implements a precision DC electronic load with multiple operating modes and an automated battery testing system. Built on the ESP32's dual-core FreeRTOS architecture for real-time performance and thread-safe operation.

### Operating Modes

- **CC Mode**: Constant Current - Maintains a steady current draw
- **CV Mode**: Constant Voltage - Regulates to a target voltage  
- **CP Mode**: Constant Power - Active software regulation for constant power dissipation
- **CR Mode**: Constant Resistance - Simulates a constant resistance load
- **Battery Test Mode**: Automated discharge testing with recovery monitoring

### Key Features

- **Dual-Core Architecture**: Time-critical tasks on Core 0, display/UI on Core 1
- **Recovery Monitoring**: Tracks battery voltage recovery after cutoff (configurable 1-30 minutes)
- **Advanced Filtering**: 16-sample moving average for voltage/current/temperature
- **Real-Time Protocol**: Serial communication with companion Python app
- **Safety Systems**: Multiple protection layers (OVP, OCP, OPP, OTP)
- **Temperature Management**: Dual PWM-controlled cooling fans
- **Precision Measurement**: 16-bit ADC/DAC for 0.3% voltage and 0.4% current accuracy

## Hardware Requirements

- **MCU**: ESP32 DevKit1 (dual-core, 240MHz)
- **Display**: SSD1351 128x128 RGB OLED (SPI interface)
- **ADC**: ADS1115 16-bit I²C ADC for voltage/current measurement
- **DAC**: DAC8571 16-bit I²C DAC for load control
- **Input**: Rotary encoder with dual-function push button
- **Cooling**: Two PWM-controlled fans
- **Sensors**: LM35 temperature sensor on heatsink
- **Power Stage**: Dual N-FET based current sink with relay control

## Specifications

### Electrical Limits

- **Input Voltage**: 1.0V - 100V DC
- **Maximum Current**: 10.2A
- **Maximum Power**: 185W @ 25°C ambient (heatsink < 85°C)
- **Maximum Temperature**: 95°C (automatic shutdown)
- **Off-State DUT Current**: < 2µA @ 2V, < 58µA @ 60V
- **Reverse Polarity Protection**: -100V with 10A fuse

### Performance

- **Voltage Accuracy**: ±0.3%
- **Current Accuracy**: ±0.4%
- **Current Resolution**: ±156µA (CP/CR software regulation)
- **Main Loop Time**: ~18ms (critical for CP/CR stability)
- **Moving Average Window**: 16 samples for voltage/current/temperature

## Battery Test Mode

The battery test mode performs automated discharge testing with advanced monitoring:

### Features

- **Configurable Chemistry**: Li-ion, LiPo, NiCd, NiMH, Lead Acid
- **Cell Count Selection**: 1-10 cells (automatic total voltage calculation)
- **Adjustable Parameters**: 
  - Test current (0.1A - 8A) (depending on total voltage 8A <40V, 4A >40V)
  - Cutoff voltage per cell
  - Recovery monitoring time (1-30 minutes, default 5)
- **Recovery Monitoring**: Tracks voltage recovery after cutoff detection
- **Real-Time Reporting**: Sends test progress to companion Python app
- **Safe Termination**: Automatic load disconnect at cutoff
- **Safe Termination**: Watchdog protection for communication loss

### Protocol

Serial communication at 9600 baud, 10 parameters:
1. Battery chemistry (0=Li-ion, 1=LiPo, 2=NiCd, 3=NiMH, 4=LeadAcid)
2. Number of cells (1-10)
3. Test current (0.1 - 8.0A)
4. Cutoff voltage per cell
5. Auto-start delay (seconds)
6. Warmup time (seconds)
7. Overshoot checking enabled (0/1)
8. Overshoot percentage (0-100%)
9. **Recovery time** (1-30 minutes)
10. Cancel test (0=run, 1=cancel)

### Voltage Filtering

Battery mode uses filtered voltage (`dispVoltage`) instead of raw readings (`dutV`) for cutoff detection. The 16-sample moving average eliminates jitter and glitches to prevent premature test termination.

## Development

### Build System

This project uses **PlatformIO** for modern C++ development:
- Full C++ IntelliSense with proper code completion
- Integrated library and dependency management
- Cross-platform build system
- VSCode integration with debugging support
- Git-friendly project structure

### Project Structure

```
Dynamic_Load_FW/
├── include/                    # Header files
│   ├── Config.h               # Pin definitions, calibration constants, I²C addresses
│   ├── DynamicLoad.h          # Main system declarations and global variables
│   ├── BatteryMode.h          # Battery test mode declarations
│   ├── FanController.h        # Fan control system
│   ├── OLEDDisplay.h          # Display management
│   ├── RotaryEncoder.h        # Encoder input handling
│   └── Debug.h                # Debugging utilities
│
├── src/                       # Implementation files
│   ├── main.cpp               # Main entry point, setup(), loop()
│   ├── DynamicLoad.cpp        # Operating modes (CC, CV, CP, CR)
│   ├── BatteryMode.cpp        # Battery test logic and recovery monitoring
│   ├── FanController.cpp      # Temperature-based fan control
│   ├── OLEDDisplay.cpp        # OLED rendering (runs on Core 1)
│   ├── RotaryEncoder.cpp      # Encoder interrupt handling
│   └── Debug.cpp              # Debug output utilities
│
├── lib/                       # Custom libraries (currently empty)
├── test/                      # Unit tests (optional)
├── platformio.ini             # PlatformIO project configuration
├── .gitignore                 # Git exclusions
└── README.md                  # This file
```

### Setup and Build

**Prerequisites:**
1. [VSCode](https://code.visualstudio.com/) with [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
2. USB drivers for ESP32 DevKit1 (CP2102 or CH340)

**Building:**
```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor --baud 9600

# Build and upload in one command
pio run --target upload --target monitor
```

**VSCode Integration:**
- Use the PlatformIO toolbar buttons
- Build: ✓ (checkmark icon)
- Upload: → (arrow icon)
- Serial Monitor: 🔌 (plug icon)

### Dependencies

All libraries are automatically managed by PlatformIO (defined in `platformio.ini`):

| Library | Version | Purpose |
|---------|---------|---------|
| ezButton | 1.0.6 | Rotary encoder debouncing |
| Adafruit SSD1351 | 1.3.2 | OLED display driver |
| Adafruit GFX | 1.11.11 | Graphics primitives |
| ADS1X15 | 0.5.1 | 16-bit ADC communication |
| DAC8571 | 0.1.2 | 16-bit DAC control |
| MovingAverage | (built-in) | Voltage/current filtering |

### Task Distribution (FreeRTOS)

The firmware leverages ESP32's dual-core architecture:

**Core 0 (APP_CPU - Main):**
- `loop()` - Main control loop (~18ms cycle time)
- `process_encoder()` - ISR-based encoder handling
- Battery test state machine
- Load control and regulation
- Safety monitoring

**Core 1 (PRO_CPU - Display):**
- `displayTask()` - OLED updates (mutex-protected)
- `fanControlTask()` - Temperature monitoring and fan PWM
- Non-critical background operations

## Calibration

The system requires calibration for accurate measurements. See `include/Config.h` for detailed procedures.

### Calibration Constants

1. **Voltage Display** (`VLT_CALIB`):
   - Compare OLED reading with accurate voltmeter
   - Adjust until readings match (typical: 1.0000 - 1.0500)

2. **Current (ADC)** (`AMP_CALIB`):
   - Use known precision shunt resistor
   - Adjust for accurate current reading (typical: 0.9500 - 1.0500)

3. **Current (DAC)** (`DAC_AMP_CALIB`):
   - Fine-tune DAC output vs actual current
   - Ensures CC mode accuracy (typical: 0.9800 - 1.0200)

4. **CV Mode Calibration** (`cvCalFactor`):
   - Calibrates CV mode trigger point accuracy
   - Procedure: Set 50.0V @ 100mA, measure trigger voltage, calculate factor = trigger_voltage / 50.0
   - Typical value: 1.0606 (v7.0.4l)
   - Achieves ±0.04V accuracy on subsequent triggers across voltage range

All calibration constants are stored in `Config.h` with inline documentation.

## Companion Software

This firmware works with a **Python desktop application** for battery testing:
- Real-time data visualization with pyqtgraph
- Test progress monitoring and graphing
- Configurable test parameters
- Data logging and export
- Recovery monitoring display

**Repository:** [Battery Tester Python App](https://github.com/paulvee/BatteryTester) *(link TBD)*

## Version History

### v7.1.1 (March 2026) - Current
- 🔧 **Fixed**: Fan control in calibration mode - fan now properly stops during calibration
- 📝 **Changed**: Version numbering to numeric patch versions only (no more letter suffixes)

### v7.1.0 (March 2026)
- 🚀 **Major Feature**: Runtime calibration system without recompilation
- 🎛️ **Added**: Boot-time calibration mode (hold encoder button during power-on)
- 💾 **Storage**: ESP32 Preferences (NVS) for persistent calibration across firmware updates
- 🖥️ **Interface**: Serial command interface with 8 commands (CAL SHOW, CV, DUTV, DUTC, SHUNT, SAVE, RESET, EXIT)
- ⌨️ **Input**: Case-insensitive commands with backspace and Ctrl-C support
- 🔄 **Exit Options**: Button click (continue boot) or CAL EXIT command (requires power cycle)
- 🌀 **Fan Control**: Automatically stopped during calibration mode
- 📦 **Binary Distribution**: Users can calibrate without development tools
- 📝 **Documentation**: Added CALIBRATION_GUIDE.md and data/README.md
- 🔧 **Calibration Parameters**: dutVcalib, DUTCurrent, shuntVcalib, cvCalFactor

### v7.0.4l (March 2026)
- ✨ **Added**: CV mode soft-start to eliminate up to 4A startup current surge
- 🔧 **Improved**: Simplified DAC settling approach (max DAC → delay → NFETs on → delay → target DAC)
- 📊 **Calibrated**: CV mode trigger point accuracy to ±0.04V across voltage range
- 🎯 **Accuracy**: Subsequent triggers within ±0.04V from 2V to 50V (first trigger ~0.15V offset)
- ⚙️ **Code Quality**: Added DAC_MAX_VALUE constant, reduced CV safety margin to 102%
- 🔧 **Calibration**: Use `cvCalFactor` 
- ⏱️ **Timing**: Optimized DAC settling delays for glitch-free CV mode activation

### v7.0.1 (March 2026)
- ✨ **Added**: Recovery monitoring with configurable timeout (1-30 minutes)
- 🐛 **Fixed**: Premature cutoff detection using filtered voltage instead of raw
- 🔧 **Improved**: Battery mode now uses `dispVoltage` (16-sample MA) for cutoff
- 📡 **Protocol**: Extended to 10 parameters (recovery_time as 9th parameter)
- 🛡️ **Stability**: Eliminated voltage jitter causing false cutoff triggers

### v7.0.0 (February 2026)
- 🏗️ **Refactored**: Dual-core architecture with proper task distribution
- 🔒 **Safety**: Added more mutex protection for shared variables
- ⚡ **Performance**: Optimized main loop for consistent 18ms cycle time
- 🧵 **Threading**: Rotary encoder, display and fan control on Core 1
- 📊 **Monitoring**: Enhanced thread safety in OLED display updates
- 🎛️ **Watchdog**: Added ESP32 task watchdog timer support
- 🔋 **Battery**: Added NiCd and NiMH chemistry support

### v6.4.26 (Legacy - Arduino IDE)
- Final Arduino IDE-based version
- Basic battery test functionality
- Available in public repository as `ESP32_V6_4_26b.zip`

## Migration from Arduino IDE

This project has been migrated from Arduino IDE to PlatformIO for better development experience:

**Benefits:**
- True C++ IntelliSense and code completion
- Git-friendly directory structure (no `.ino` files)
- Professional dependency management
- Cross-platform builds
- Integrated debugging support
- Faster compilation with caching

**Breaking Changes from v6.4.26:**
- Project structure reorganized (`include/` and `src/` folders)
- PlatformIO required (Arduino IDE not supported)
- Serial protocol extended (10 parameters vs 8)
- Recovery monitoring feature added

## Troubleshooting

### Build Issues

**Problem:** PlatformIO not found or build fails  
**Solution:** 
- Ensure PlatformIO extension is installed in VSCode
- Reload window after installation
- Check that `platformio.ini` is in the project root

**Problem:** Library download fails  
**Solution:**
- Check internet connection
- Clear PlatformIO cache: `pio pkg uninstall`
- Manually specify library versions in `platformio.ini`

### Upload Issues

**Problem:** ESP32 not detected  
**Solution:**
- Install CP2102 or CH340 USB drivers
- Check USB cable (must support data, not just power)
- Press and hold BOOT button during upload if needed
- Try different USB port

**Problem:** Upload timeout or fails midway  
**Solution:**
- Press and hold BOOT button until upload starts
- Reduce upload speed in `platformio.ini`: `upload_speed = 460800`
- Check USB cable quality

### Runtime Issues

**Problem:** Display not working  
**Solution:**
- Check SPI connections (DC, RES, CS, SCL, SDA pins)
- Verify 3.3V power supply
- Check `Config.h` pin definitions match your wiring

**Problem:** Inaccurate voltage/current readings  
**Solution:**
- Perform calibration procedure (see Calibration section)
- Check ADC connections and I²C pullups
- Verify reference voltage stability

**Problem:** Premature battery test cutoff  
**Solution:**
- ✅ **Fixed in v7.0.1** - Now uses filtered voltage
- Ensure battery chemistry and cell count are correct
- Check cutoff voltage setting
- Monitor for voltage drops under load

**Problem:** CP/CR mode regulation  
**Solution:**
- Ensure main loop time stays around 18ms
- Check that Core 0 is not overloaded
- Reduce load or increase heatsink cooling

## Safety Warnings

⚠️ **IMPORTANT SAFETY INFORMATION**

This is a high-power electronic device that can dissipate up to 185W. Improper use can cause:
- **Fire hazard** from overheating
- **Equipment damage** from over-current or over-voltage
- **Electric shock** from exposed connections
- **Battery damage** from improper discharge

### Operating Safety

1. **Never exceed maximum ratings**:
   - 10.2A maximum current
   - 100V maximum voltage
   - 185W maximum power @ 25°C ambient
   - 95°C maximum heatsink temperature

2. **Adequate cooling required**:
   - Ensure fans are operational
   - Do not block ventilation
   - Monitor heatsink temperature
   - Derate power at elevated ambient temperatures

3. **Battery testing precautions**:
   - Never leave battery tests unattended
   - Use appropriate fire-safe containers
   - Monitor battery temperature
   - Ensure correct chemistry and cell count settings
   - Stop test if battery becomes hot or swells

4. **Electrical safety**:
   - Use proper fusing on input
   - Ensure adequate wire gauge for current levels
   - Check connections before applying power
   - Reverse polarity protection is limited to -100V

5. **Enclosure and grounding**:
   - Keep high-voltage circuits enclosed
   - Ground the chassis properly
   - Use appropriate insulation

## Known Issues and Limitations

- **Main loop timing**: Can vary under heavy I²C traffic; affects CP/CR regulation accuracy
- **Fan tachometer**: Not currently implemented in firmware
- **Serial baud rate**: Battery test uses 9600 baud
- **Arduino library dependencies**: Some libraries pulled in entire Arduino framework

## Contributing

This is a personal project but suggestions and improvements are welcome:

1. **Reporting Bugs**: Open an issue with:
   - Firmware version
   - Hardware configuration
   - Steps to reproduce
   - Serial output or photos if applicable

2. **Feature Requests**: Describe the use case and expected behavior

3. **Code Contributions**: 
   - Follow existing code style
   - Test thoroughly on hardware
   - Update README if adding features
   - Keep commits focused and well-described

## Project Links

- **Public Hardware Project**: [Dynamic-DC-Load](https://github.com/paulvee/Dynamic-DC-Load)
- **Blog - Building Process**: [Building DIY Dynamic DC Load](https://www.paulvdiyblogs.net/2024/09/building-diy-dynamic-dc-load.html)
- **Blog - Design Process**: [DIY DC Dynamic Load Design](https://www.paulvdiyblogs.net/2024/04/build-diy-dc-dynamic-load-instrument.html)
- **Hardware Files**: Schematics, PCB, 3D enclosure in public repository
- **Companion App**: Battery Tester Python application *(link TBD)*

## Related Projects

- **Arduino IDE Version (v6.4.26)**: Available in public repo as `ESP32_V6_4_26b.zip`
- **Battery Tester Python GUI**: PyQt6 application for battery test data visualization
- **3D Printed Enclosure**: STL files available in public repository

## Acknowledgments

- **Bud Bennett**: Collaborative hardware and software design and 3D enclosure
- **PlatformIO**: Modern embedded development platform
- **ESP32 Community**: FreeRTOS examples and dual-core patterns

## License

This project is provided as-is for educational and hobbyist purposes. 

**Hardware Design**: Joint project with Bud Bennett  
**Firmware**: © 2024-2026 Paul Versteeg

Feel free to use, modify, and distribute for personal and educational purposes. Commercial use requires permission.

---

## Quick Start Checklist

- [ ] Install VSCode with PlatformIO extension
- [ ] Clone/download this repository
- [ ] Open project folder in VSCode
- [ ] Let PlatformIO download dependencies (first time takes a few minutes)
- [ ] Connect ESP32 via USB
- [ ] Build and upload firmware
- [ ] Calibrate voltage and current readings
- [ ] Test with low-power load first (< 10W)
- [ ] Verify safety systems (OVP, OCP, OTP)
- [ ] Install companion Python app for battery testing (optional)

## Support

For questions, issues, or discussion:
- **GitHub Issues**: Bug reports and feature requests
- **Blog Comments**: General discussion on build process

---

**Last Updated**: March 2026  
**Firmware Version**: 7.0.4l  
**Status**: Active Development (Private Repository)
