# Changelog - Dynamic DC Load Firmware

All notable changes to the ESP32 Dynamic DC Load firmware since version 7.0.0.

---

## [7.0.4l] - 2026-03-15

### Added
- **CV mode soft-start implementation** - Eliminates 4A startup current surge
  - Simplified DAC settling: Write max DAC → 100ms delay → NFETs on → 50ms delay → target DAC → 100ms settle
  - Total settling time ~250ms (much faster than previous ramp approach)
  - No visible current glitches on oscilloscope waveform display
  - `firstCVEntry` flag ensures special handling only on first activation

### Changed
- **CV mode calibration system** - New simplified calibration constant
  - Replaced legacy `CUTIN_FUDGE` and `CV_CUTIN_VLT_FACTOR` with single `cvCalFactor` constant
  - Calibrated to 1.0606 (from 1.054333) using 50V reference point
  - Achieves ±0.04V accuracy on subsequent triggers across 2V-50V range
  - First trigger shows ~0.15V fixed offset (acceptable, due to circuit settling)
- **Code quality improvements**
  - Added `DAC_MAX_VALUE = 65535` constant replacing hardcoded values (5 locations)
  - Reduced CV mode safety margin from 105% to 102% for tighter control
  - Improved code comments for CV mode voltage relationships

### Technical Details
- CV mode trigger accuracy tested at 2V, 25V, and 50V with consistent ±0.04V precision
- Calibration procedure: Apply 50.0V @ 100mA, measure trigger voltage, calculate cvCalFactor = trigger_V / 50.0
- Soft-start prevents inrush current to CV circuit capacitance during initial activation
- Watchdog timer considerations handled (unnecessary with short ~250ms settling time)

### Files Modified
- `include/Config.h` - Added cvCalFactor = 1.0606, version bump to 7.0.4l
- `src/main.cpp` - Simplified CV soft-start, DAC_MAX_VALUE constant, reduced safety margin

---

## [7.0.3] - 2026-03-11

### Added
- **60-second communication watchdog** - Critical safety feature
  - Tracks `lastSerialActivity` timestamp on all incoming serial data
  - Automatically aborts battery test if no serial communication for 60 seconds
  - Receives heartbeat (HB) command from Python app every 30 seconds
  - Watchdog triggers updated on: AUTO_BT, test parameters, cancel (999), heartbeat (HB)
  - Displays "Comm Timeout - PC Disconnected" message on abort
  - **Safety**: Prevents runaway discharge if PC app crashes or USB cable disconnects

### Fixed
- Cancel command now handles both string '999' and numeric 999 formats
- Heartbeat commands are silently ignored (transparent to user)

### Changed
- Version bumped to 7.0.3 in Config.h

### Files Modified
- `include/BatteryMode.h` - Added lastSerialActivity variable and updateWatchdog() method
- `include/Config.h` - Version bump
- `src/BatteryMode.cpp` - Watchdog implementation and timeout checking
- `src/main.cpp` - Heartbeat command handling

---

## [7.0.2] - 2026-03-10

### Added
- **Automatic mode switch** to Battery Test mode
  - Listens for `AUTO_BT` command on serial port during normal operation
  - Automatically switches UI to Battery Test mode when command received
  - Sends `ACK_BT` acknowledgment when ready to receive test parameters
  - Seamless UX integration with Python app v2.0.h+

### Changed
- Version bumped to 7.0.2 in Config.h

### Technical Notes
- Does not interfere with firmware upload (esptool doesn't send AUTO_BT)
- Fully backward compatible - Python app works with or without this firmware feature
- Implemented in main loop's serial command handler

### Files Modified
- `include/Config.h` - Version bump
- `src/main.cpp` - AUTO_BT command handling and mode switching (32 lines added)

---

## [7.0.1] - 2026-03-10

### Added
- **Complete PlatformIO project structure** - Migration from Arduino IDE
  - Dual-core FreeRTOS architecture for thread safety
  - Complete modular code organization (include/, src/, lib/)
  - platformio.ini configuration for ESP32-WROOM-32

- **Core Features** (re-release with PlatformIO):
  - Multiple operating modes: CC, CV, CP, CR, Battery Test
  - **Recovery monitoring** with configurable timeout (1-30 minutes, parameter 9)
  - 16-sample moving average filtering for voltage/current stability
  - Serial protocol with 10 parameters (recovery_time as 9th parameter)
  - Safety protections: OVP, OCP, OPP, OTP
  - Temperature-controlled fan management
  - Precision measurement: 0.3% voltage accuracy, 0.4% current accuracy

- **Fixed premature cutoff issue**:
  - Now uses filtered voltage (16-sample moving average) for cutoff detection
  - Eliminates false positives from voltage noise spikes
  - Robust battery end-of-discharge detection

- **Documentation**:
  - Comprehensive README.md (448 lines)
  - NEXT_STEPS.md for development guide (138 lines)
  - firmware_release_v7.0.0/ folder with legacy Arduino IDE files
    - QUICK_START.txt
    - VERSION_INFO.txt
    - flash_firmware.bat
    - README.md

- **Project Structure**:
  ```
  include/          - Header files (Config.h, BatteryMode.h, DynamicLoad.h, etc.)
  src/              - Implementation files (main.cpp, BatteryMode.cpp, etc.)
  lib/MovingAverage/- Moving average filter library
  .gitignore        - Git ignore patterns
  platformio.ini    - PlatformIO build configuration
  ```

### Files Created (29 files, 3715+ lines)
- Core Headers: Config.h, BatteryMode.h, DynamicLoad.h, FanController.h, OLEDDisplay.h, RotaryEncoder.h, Debug.h
- Implementation: main.cpp, BatteryMode.cpp, FanController.cpp, OLEDDisplay.cpp, RotaryEncoder.cpp, Debug.cpp
- Library: MovingAverage (with examples and documentation)
- Build: platformio.ini, .gitignore
- Documentation: README.md, NEXT_STEPS.md
- Legacy release: firmware_release_v7.0.0/ folder contents

---

## Summary 7.0.0 → 7.0.3

**Major Changes:**
1. ✅ **Migration to PlatformIO** - Modern development environment with better tooling
2. 🔒 **Communication Watchdog** - Critical safety feature preventing runaway discharge
3. 🎯 **Auto Mode Switch** - Seamless UX when starting tests from Python app
4. 📊 **Filtered Cutoff Detection** - More reliable battery test termination
5. ⚙️ **Recovery Monitoring** - Configurable recovery timeout parameter

**Safety Improvements:**
- 60-second communication timeout with automatic test abort
- Prevents discharge if PC disconnects or crashes
- Filtered voltage prevents premature cutoffs from noise

**UX Improvements:**
- Automatic mode switching reduces manual steps
- Heartbeat protocol keeps connection alive
- Backward compatible with older Python app versions

**Development Improvements:**
- PlatformIO project structure
- Modular code organization
- Comprehensive documentation
- Git version control

---

## Version Comparison

| Feature                    | 7.0.0 | 7.0.1 | 7.0.2 | 7.0.3 |
|---------------------------|-------|-------|-------|-------|
| PlatformIO Project        | ❌    | ✅    | ✅    | ✅    |
| Recovery Monitoring       | ❌    | ✅    | ✅    | ✅    |
| Filtered Cutoff Detection | ❌    | ✅    | ✅    | ✅    |
| Auto Mode Switch          | ❌    | ❌    | ✅    | ✅    |
| Communication Watchdog    | ❌    | ❌    | ❌    | ✅    |
| Heartbeat Protocol        | ❌    | ❌    | ❌    | ✅    |

---

*Generated: March 11, 2026*
