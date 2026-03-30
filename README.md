# Dynamic DC Load Firmware

The hardware information is located here: https://github.com/paulvee/Dynamic-DC-Load-Hardware

**ESP32 Dynamic DC Load Firmware — v7.1.8**  
Platform: ESP32 DevKit1 | Build system: PlatformIO  
Author: Paul Versteeg

Professional C++ firmware for the DIY Dynamic DC Load electronic load controller.

## Documentation

| Document | Description |
|----------|-------------|
| [CALIBRATION_GUIDE.md](CALIBRATION_GUIDE.md) | How to calibrate voltage, current and CV mode at runtime |
| [CHANGELOG.md](CHANGELOG.md) | Version history and release notes |
| [TEST_PLAN.md](TEST_PLAN.md) | Test procedures |

Full build documentation and background: https://www.paulvdiyblogs.net/2024/09/building-diy-dynamic-dc-load.html

## Overview

This firmware implements a precision DC electronic load with multiple operating modes and an automated battery testing system. Built on the ESP32's dual-core FreeRTOS architecture for real-time performance and thread-safe operation.

### Operating Modes

- **CC** — Constant Current: maintains a steady current draw
- **CV** — Constant Voltage: regulates to a target voltage
- **CP** — Constant Power: active software regulation for constant power dissipation
- **CR** — Constant Resistance: simulates a constant resistance load
- **Battery Test** — Automated discharge testing with recovery monitoring

### Key Features

- **Dual-Core Architecture**: time-critical tasks on Core 0, display/UI on Core 1
- **Recovery Monitoring**: tracks battery voltage recovery after cutoff (configurable 1–30 minutes)
- **Advanced Filtering**: 16-sample moving average for voltage/current/temperature
- **Real-Time Protocol**: serial communication with companion Python app
- **Safety Systems**: multiple protection layers (OVP, OCP, OPP, OTP)
- **Temperature Management**: dual PWM-controlled cooling fans
- **Precision Measurement**: 16-bit ADC/DAC — two-point current calibration for accuracy across full current range

## Quick Start

**Prerequisites:** VSCode + PlatformIO extension, CH340 or CP2102 USB drivers

```bash
# Clone and open in VSCode
git clone https://github.com/paulvee/Dynamic-DC-Load.git

# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200
```

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32 DevKit1 (dual-core, 240 MHz) |
| Display | SSD1351 128×128 RGB OLED (SPI) |
| ADC | ADS1115 16-bit I²C — voltage/current measurement |
| DAC | DAC8571 16-bit I²C — load control |
| Input | Rotary encoder with dual-function push button |
| Cooling | Two PWM-controlled fans with tachometer feedback |
| Sensor | LM35 temperature sensor on heatsink |
| Power Stage | Dual N-FET based current sink with relay control |

## Specifications

### Electrical Limits

| Parameter | Value |
|-----------|-------|
| Input Voltage | 1.0V – 100V DC |
| Maximum Current | 10.2A |
| Maximum Power | 185W @ 25°C ambient (heatsink < 85°C) |
| Maximum Temperature | 95°C (automatic shutdown) |
| Off-State DUT Current | < 2µA @ 2V, < 58µA @ 60V |
| Reverse Polarity Protection | –100V with 10A fuse |

### Performance

| Parameter | Value |
|-----------|-------|
| Voltage Accuracy | ±0.3% |
| Current Accuracy | ±0.4% (after two-point calibration) |
| Current Resolution | ±156µA (CP/CR software regulation) |
| Main Loop Time | ~18ms (critical for CP/CR stability) |
| Moving Average Window | 16 samples (voltage/current/temperature) |

## Calibration

Runtime calibration via serial commands — no recompilation needed. See [CALIBRATION_GUIDE.md](CALIBRATION_GUIDE.md).

V7.1.8 introduces **two-point DUT voltage calibration** (`CAL VH <actual_V> <oled_V>`) — below `vRefLow` (2.5V) the hardware trimmer controls accuracy; above it the firmware linearly interpolates a correction factor up to `vRefHigh` (60V).

V7.1.6 introduces **two-point current calibration** (`CAL CURRL` at low current, `CAL CURRH` at high current), replacing the old single-point shunt calibration. This significantly improves current accuracy across the full operating range.

> ⚠️ Use **PuTTY** or **Tera Term** (not the PlatformIO or Arduino IDE serial monitor) when entering calibration commands. See the guide for details.

## Battery Test Mode

Automated discharge testing with advanced monitoring:

- **Configurable Chemistry**: NiMH, NiCd, Alkaline, LiPo, LiFePO4, LiHV
- **Cell Count Selection**: 1–10 cells (automatic voltage calculation)
- **Adjustable Parameters**: test current, cutoff voltage, recovery time (1–30 min)
- **Recovery Monitoring**: tracks voltage recovery after cutoff detection
- **Real-Time Reporting**: sends test progress to companion Python app
- **Safe Termination**: automatic load disconnect at cutoff
- **Voltage Filtering**: uses 16-sample moving average for cutoff detection — eliminates jitter and prevents premature test termination

## Companion App

Works with the [Battery Tester Python app](BattTester/) for real-time battery test monitoring and data logging. A pre-built Windows executable is available in the [Releases](https://github.com/paulvee/Dynamic-DC-Load/releases) section.

## Project Structure

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
│   ├── BatteryMode.cpp        # Battery test logic and recovery monitoring
│   ├── FanController.cpp      # Temperature-based fan control
│   ├── OLEDDisplay.cpp        # OLED rendering (runs on Core 1)
│   ├── RotaryEncoder.cpp      # Encoder interrupt handling
│   └── Debug.cpp              # Debug output utilities
│
├── data/                      # Calibration data (DL_Cal_Values.ini)
├── firmware_release_v7.1.8/   # Pre-built binary release
├── BattTester/                # Battery Tester Python app source
├── lib/                       # Custom libraries
├── platformio.ini             # PlatformIO project configuration
└── README.md                  # This file
```

## Task Distribution (FreeRTOS)

**Core 0 (APP_CPU — Main):**
- `loop()` — main control loop (~18ms cycle time)
- `process_encoder()` — ISR-based encoder handling
- Battery test state machine, load control, safety monitoring

**Core 1 (PRO_CPU — Display):**
- `displayTask()` — OLED updates (mutex-protected)
- `fanControlTask()` — temperature monitoring and fan PWM

## Dependencies

All libraries managed automatically by PlatformIO:

| Library | Purpose |
|---------|---------|
| ezButton | Rotary encoder debouncing |
| Adafruit SSD1351 | OLED display driver |
| Adafruit GFX | Graphics primitives |
| ADS1X15 | 16-bit ADC communication |
| DAC8571 | 16-bit DAC control |
| MovingAverage | Voltage/current filtering |

## Troubleshooting

**ESP32 not detected on upload:**
- Install CP2102 or CH340 USB drivers
- Use a data-capable USB cable (not charge-only)
- Hold BOOT button during upload if needed

**Upload timeout:**
- Add `upload_speed = 460800` to `platformio.ini`
- Try a different USB port or cable

**Library download fails:**
- Check internet connection
- Clear cache: `pio pkg uninstall --global`
