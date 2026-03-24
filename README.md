# DL-Firmware-for-VSC

[![Hypercommit](https://img.shields.io/badge/Hypercommit-DB2475)](https://hypercommit.com/dynamic-dc-load)

**ESP32 Dynamic DC Load Firmware — v7.1.6**  
Platform: ESP32 DevKit1 | Build system: PlatformIO

This is the development repository for the ESP32-based Dynamic DC Load firmware. It provides precision DC load control with constant current, voltage, power and resistance modes, plus automated battery testing.

## Documentation

| Document | Description |
|----------|-------------|
| [CALIBRATION_GUIDE.md](CALIBRATION_GUIDE.md) | How to calibrate voltage, current and CV mode at runtime |
| [CHANGELOG.md](CHANGELOG.md) | Version history and release notes |
| [TEST_PLAN.md](TEST_PLAN.md) | Test procedures |

## Quick Start

**Prerequisites:** VSCode + PlatformIO extension, CH340 or CP2102 USB drivers

```bash
# Clone and open in VSCode
git clone https://github.com/paulvee/DL-Firmware-for-VSC.git

# Build and upload
pio run --target upload

# Monitor serial output (9600 baud)
pio device monitor
```

## Operating Modes

- **CC** — Constant Current
- **CV** — Constant Voltage
- **CP** — Constant Power
- **CR** — Constant Resistance
- **Battery Test** — Automated discharge testing with recovery monitoring

## Hardware

- MCU: ESP32 DevKit1 (dual-core, 240 MHz)
- Display: SSD1351 128×128 RGB OLED (SPI)
- ADC: ADS1115 16-bit I²C
- DAC: DAC8571 16-bit I²C
- Input: Rotary encoder with push button
- Cooling: Dual PWM-controlled fans
- Max load: 10.2A / 100V / 185W

## Calibration

Runtime calibration via serial commands — no recompilation needed. See [CALIBRATION_GUIDE.md](CALIBRATION_GUIDE.md).

V7.1.6 introduces **two-point current calibration** (`CAL CURRL` at low current, `CAL CURRH` at high current) replacing the old single-point shunt calibration. This significantly improves current accuracy across the full operating range.

> ⚠️ Use **PuTTY** or **Tera Term** (not the PlatformIO or Arduino IDE serial monitor) when entering calibration commands. See the guide for details.

## Companion App

Works with the [Battery Tester Python app](BattTester/) for real-time battery test monitoring and data logging. A pre-built Windows executable is available in the [Releases](https://github.com/paulvee/Dynamic-DC-Load/releases) section.

## Blog

Full build documentation and background at: https://www.paulvdiyblogs.net/2024/09/building-diy-dynamic-dc-load.html
