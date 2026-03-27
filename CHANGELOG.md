# Changelog - Dynamic DC Load Firmware

All notable changes to the ESP32 Dynamic DC Load firmware since version 7.0.0.

---

## [7.1.8] - 2026-03-27

### Added
- **Two-point DUT voltage calibration** — replaces single `dutVcalib` trim factor with a low anchor and high reference point plus linear interpolation between them
  - New NVS parameters: `vCalHigh`, `vRefLow`, `vRefHigh`
  - New commands: `CAL VH <actual_V> <oled_V>` (sets high-point factor and reference voltage) and `CAL VREF <voltage>` (sets low anchor, default 2.5 V)
  - Below `vRefLow`: factor = 1.0 — hardware trimmer calibrates this region, software leaves it untouched
  - `vRefLow` to `vRefHigh`: correction factor linearly interpolated from 1.0 to `vCalHigh`
  - Above `vRefHigh`: factor flat at `vCalHigh`
  - Typical workflow: trim hardware to 2.500 V at low voltage, then enter `CAL VH <actual> <oled>` at a high voltage (e.g., 60 V) to capture the resistor-divider linearity error
- **Boot reset-reason log** — startup now prints the reset cause (`Power-on / EN button`, `Software reset (ESP.restart)`, etc.) to the serial monitor for diagnostics

### Changed
- **`dutVcalib` retired** — replaced by `vCalHigh` / `vRefLow` / `vRefHigh` NVS keys. First boot after update resets voltage cal to defaults (all factors 1.0); re-calibration required
- **Calibration display precision increased** — `dutVcalib`, `cvCalFactor`, `iCalLow`, `iCalHigh` now displayed to 9 decimal places (was 6); entered values echo back at full precision
- **CAL EXIT restart** — rotary button now waits for button release before calling `ESP.restart()`, plus `Serial.flush()` + 100 ms settle before reset for a clean restart sequence
- **Calibration command menu** — aligned description column for all commands; `CAL DUTV` removed (replaced by `CAL VH` / `CAL VREF`)

### Notes
- Old `dutVcalib` NVS key is orphaned on upgrade — no migration needed, defaults to 1.0 on first boot
- Current calibration (`iCalLow`, `iCalHigh`, etc.) is unaffected and survives the upgrade

---

## [7.1.7] - 2026-03-25

### Changed
- **CALIBRATION_GUIDE.md** — Major documentation update:
  - Added serial terminal compatibility table (PuTTY ✅, Arduino IDE 2.x ✅, Tera Term 5 ⚠️ config needed, VSCode ❌, PlatformIO monitor ❌) with per-terminal notes and configuration instructions
  - Added note that DTR/RTS behaviour is controlled by the host application and may change with future software updates
  - Expanded `CAL EXIT` section with numbered exit sequence and inline recovery box for the "closed serial monitor before pulling cable" scenario
  - Added new troubleshooting entry: **ESP32 Stuck in Reset After Closing Serial Monitor** — corrected symptom description (OLED freezes on last screen, EN button ineffective), explains why the EN button cannot override a bridge-asserted EN hold, updated recovery to: pull USB cable + power cycle the board + reconnect USB
  - Added **C44** PCB cross-reference at all capacitor mentions: the 22 µF capacitor on the EN line (C44) must always be installed; it absorbs the DTR pulse on port-open and prevents reset-on-connect
  - Corrected and expanded the **ESP32 Resets When Typing Commands** troubleshooting entry with full affected/working/config-needed terminal lists

---

## [7.1.6] - 2026-03-24

### Added
- **Two-point current calibration** — replaces single `shuntVcalib` trim factor with a low and high reference point plus linear interpolation between them
  - New NVS parameters: `iCalLow`, `iRefLow`, `iCalHigh`, `iRefHigh`
  - New commands: `CAL CURRL <set_mA> <oled_mA>` and `CAL CURRH <set_mA> <oled_mA>`
  - Values entered as mA; auto-converted to A internally
  - Correction factor applied flat below low reference and above high reference; interpolated between
  - Goal: set current (encoder) matches OLED displayed current across full operating range

### Changed
- `CAL SHUNT` command removed; replaced by `CAL CURRL` / `CAL CURRH`
- `shuntVcalib` NVS key retired; new keys are `iCalLow`, `iRefLow`, `iCalHigh`, `iRefHigh` (default 1.0/0.1A/1.0/8.0A — safe on first boot)
- Updated `CALIBRATION_GUIDE.md` with new two-point procedure and command reference

### Fixed
- **Boot log garbled output** — Task startup messages were split across two `Serial.print()` calls, allowing other tasks to interleave between the text and the core number. Replaced with single atomic `Serial.printf()` calls with CRLF line endings across all five task files (`main.cpp`, `RotaryEncoder.cpp`, `BatteryMode.cpp`, `FanController.cpp`, `OLEDDisplay.cpp`).
- **System locks up when serial monitor closed** — Two root causes fixed:
  1. `while (!Serial)` in `setup()` blocked indefinitely waiting for a USB host connection. This was always a no-op on older ESP32 Arduino core versions, but espressif32 platform 6.13.0 (released Feb 26 2026) changed `HardwareSerial::operator bool()` to check DTR state, making it actually block when no terminal is connected. Removing this line fixes the issue on all platform versions.
  2. `Serial.setTxBufferSize(0)` called before `Serial.begin()`. Disables the TX ring buffer so all Serial writes use direct FIFO — bytes are silently dropped when no terminal is connected rather than blocking the calling task.
- **Calibration mode message flood on terminal connect** — When `while (!Serial)` was removed, the boot path no longer waited for a terminal before printing calibration startup messages. The TX FIFO filled immediately, and all buffered bytes flooded the terminal screen the moment a connection was opened. Fixed by adding an explicit `Serial.available()` wait loop inside the calibration mode block — the firmware waits up to 60 seconds for the user to type `CAL` + Enter before printing anything. This completely separates the terminal-connect event from the calibration output.

### Known Limitations — USB Serial RTS/DTR Reset Behaviour
This is a well-known, long-standing issue in the Arduino/ESP32 ecosystem and is not specific to this firmware.

**Root cause:** The USB-to-serial bridge chip (typically CP2102 or CH340) on most ESP32 development boards wires DTR and RTS to the automatic reset circuit. This was designed to allow programming tools to reset the chip before flashing. The two signals are intentionally swapped to create a short reset pulse on DTR toggle — a deliberate (but now archaic) design choice made early in the Arduino ecosystem that has never been corrected.

**Consequence:** Any application that asserts or toggles DTR/RTS when opening or closing the serial port will trigger the EN (reset) line on the ESP32. Three common triggers:
- **Opening the port** — most terminals and monitors assert DTR on connect, which resets the ESP32.
- **Closing the port** — when a terminal closes, it de-asserts DTR, which can hold EN low (perpetual reset state) depending on the RC time constant of the capacitor on the EN line.
- **Sending a break condition** — some IDEs and monitors do this on disconnect.

**What we fixed (hardware):** Adding C44, the EN capacitor with a 22 µF value, on the DL PCB. (on the ESP board itself, there is most likely already a 100 nF) We do this to significantly increase the RC time constant of the auto-reset circuit. Populating C44 makes the reset pulse from opening the port too short to latch, eliminating unwanted resets on port open. The PlatformIO `monitor_dtr = 0` / `monitor_rts = 0` settings in `platformio.ini` also suppress DTR/RTS assertion in the PlatformIO serial monitor specifically.

**What cannot be fully fixed in firmware:** The ESP32 processor has no control over what happens to its EN pin from the host side. Once EN is pulled low by the host's USB-serial driver, the CPU is already in reset and cannot run any code to recover. This means the close-port reset can only be avoided by breaking the USB connection *before* the terminal is closed.

**Affected tools:** PuTTY, Arduino IDE serial monitor, PlatformIO serial monitor (despite `monitor_dtr=0`). Flow control settings in PuTTY do not help — DTR toggling on connect/disconnect is independent of flow control.

**Not affected:** The Battery Tester desktop application manages the serial port correctly and does not trigger this issue.

**Required workaround for calibration sessions:** After typing `CAL EXIT`, the OLED and serial monitor both display:
> *"Disconnect the USB cable first, before you close the serial monitor."*
The user must physically disconnect the USB cable before closing PuTTY (or any other terminal). After reconnecting, the ESP32 boots normally. Alternatively, pressing the rotary encoder button after disconnecting will also trigger a clean `ESP.restart()`.

### Notes
- Old `shuntVcalib` NVS key is simply orphaned — no migration needed
- Calibration procedure requires no external ammeter; collect set vs OLED readings during normal operation, then enter values in cal mode

---

## [7.1.3] - 2026-03-24

### Changed
- **Calibration system refactored** - Separated hardware gain from user calibration
  - Added fixed `I_GAIN` constant (2.5) in `Config.h` — hardware current conversion factor, not user-adjustable
  - `shuntVcalib` is now a trim factor around 1.0 (was 2.5) — much clearer calibration intent
  - Current calculation is now `I_GAIN * shuntVcalib * raw_voltage`
- **Renamed `DUTCurrent` to `DAC_ADC_TOLERANCE`** — better describes actual purpose (measured mV at DAC-ADC calibration point)
- **Fixed serial output formatting** - Added `\r\n` prefix to `CAL SAVE`, `CAL RESET`, and `CAL EXIT` responses to prevent tight output in terminal
- **Increased `shuntVcalib` display precision** from 4 to 6 decimal places in `CAL SHOW` output

### Notes
- Users with a previously saved `shuntVcalib` of 2.5 in NVS must run `CAL SHUNT 1.0` + `CAL SAVE` after flashing

---

## [7.1.2] - 2026-03-17

### Changed
- **Battery Tester Protocol** - Optimized serial communication for Battery Tester v2.1.0+
  - Streamlined data reporting for improved performance
  - Enhanced compatibility with new Battery Tester features (Stop button, auto-ranging)

### Notes
- This version is recommended for Battery Tester v2.1.0 and later

---

## [7.1.1] - 2026-03-16

### Fixed
- **Fan control in calibration mode** - Fan now properly stops during calibration mode
  - Moved fan PWM channel setup earlier in boot sequence (before calibration mode check)
  - Previously `ledcWrite()` had no effect because channel wasn't configured yet
  - Fan now reliably stops when entering calibration mode for quiet operation

### Changed
- **Version numbering** - Switched to numeric patch versions only (no more letter suffixes)
  - Modified `increment_version.py` to increment patch number (X.Y.Z → X.Y.Z+1)
  - Cleaner version scheme aligned with semantic versioning standards

---

## [7.1.0] - 2026-03-16

### Added
- **Runtime Calibration System** - Major feature enabling calibration without recompilation
  - Boot-time calibration mode: Hold encoder button during power-on to enter
  - Serial command interface with 8 commands (CAL SHOW, CAL CV, CAL DUTV, CAL DUTC, CAL SHUNT, CAL SAVE, CAL RESET, CAL EXIT)
  - ESP32 Preferences (NVS) storage for persistent calibration across firmware updates
  - Case-insensitive command parsing with backspace and Ctrl-C support
  - Four calibration parameters: dutVcalib, DUTCurrent, shuntVcalib, cvCalFactor
  - Two-stage display: Simple welcome screen → Full command menu after first input
  - Button click exit: Quick cancel and continue boot (no power cycle needed)
  - CAL EXIT command: Formal completion requiring power cycle

### Changed
- **Calibration constants architecture**
  - Moved from hardcoded `const double` in Config.h to runtime `double` variables
  - Defined `#define DEFAULT_*` values in Config.h as fallback defaults
  - Calibration values loaded at boot from Preferences, using defaults if not found
  - CalibrationManager class handles all load/save/reset operations

### Technical Details
- **Storage**: ESP32 NVS partition in "dl_cal" namespace
- **Persistence**: Survives firmware updates, OTA updates, and reflashing
- **Precision**: Double precision floating point (15+ significant digits)
- **Boot sequence**: Check button → Show splash → Enter cal mode if button held
- **Fan control**: Automatically stopped during calibration mode
- **Button timing**: Must hold through entire boot sequence (start when screen blanks)
- **Baud rate**: 9600 baud for serial communication

### User Experience
- Binary distribution friendly: No PlatformIO needed for calibration
- Serial terminal only (PuTTY, Arduino IDE Serial Monitor, etc.)
- Calibration persists across firmware updates
- Quick cancel via button click or formal exit via CAL EXIT command

### Files Added
- `include/CalibrationManager.h` - Calibration manager class definition
- `src/CalibrationManager.cpp` - Preferences-based calibration implementation (~180 lines)
- `data/DL_Cal_Values.ini` - Reference file (documentation only, not used by firmware)
- `data/README.md` - Calibration system user guide
- `CALIBRATION_GUIDE.md` - Complete calibration procedures and examples

### Files Modified
- `include/Config.h` - Version 7.1.0, DEFAULT_* calibration defines, updated comments
- `include/DynamicLoad.h` - Added extern declarations for runtime calibration variables
- `src/main.cpp` - Added calibration mode boot sequence, display, and command loop (~150 lines)

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
