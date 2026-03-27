# Dynamic Load Firmware v7.1.8 - Installation Guide

This package contains pre-compiled firmware binaries for the ESP32 Dynamic Load controller.

## What's New in v7.1.8
- **Two-point DUT voltage calibration** — Replaces single `dutVcalib` factor with a two-point interpolation model
  - Below `vRefLow` (default 2.5V): hardware trimmer handles accuracy; no software correction applied
  - Between `vRefLow` and `vRefHigh`: linear interpolation using `vCalHigh` correction factor
  - Above `vRefHigh`: flat correction at `vCalHigh` (no extrapolation beyond calibrated range)
  - New commands: `CAL VH <actual_V> <oled_V>` and `CAL VREF <voltage>`
  - Removed: `CAL DUTV` (replaced by `CAL VH`)
- **Calibration display precision** — Increased from 6 to 9 decimal places for `vCalHigh`, `cvCalFactor`, `iCalLow`, `iCalHigh`
- **CAL EXIT improvement** — Waits for button release before restarting; `Serial.flush()` + 100ms delay before `ESP.restart()`
- **Boot diagnostic** — Prints reset reason at startup (software reset / power-on / etc.)

## What's New in v7.1.7
- **Calibration guide update** — Terminal compatibility table, expanded exit procedure, corrected troubleshooting entries
- **C44 note** — The 22 µF capacitor on the EN line (C44 on the PCB) must always be installed; it prevents reset-on-connect

## What's New in v7.1.6
- **Two-point current calibration** — Compensates for DAC non-linearity across the full current range
- New commands: `CAL CURRL <set_mA> <oled_mA>` and `CAL CURRH <set_mA> <oled_mA>`
- No external ammeter needed — calibrate by comparing set-point to OLED display
- Readings are collected in normal operating mode, then entered in calibration mode
- **Serial lock-up fixes** — Removed `while(!Serial)` and added `Serial.setTxBufferSize(0)` to prevent firmware hanging when the serial monitor is closed
- **Cal mode flood fix** — Firmware waits for `CAL` command before printing any output
- **Improved CAL EXIT** — OLED and serial instruct user to disconnect USB before closing terminal; rotary button triggers clean restart

## What's New in v7.1.2
- Battery Tester Protocol — Optimized serial communication for Battery Tester v2.1.2+
- Streamlined data reporting for improved performance
- Enhanced compatibility with new Battery Tester features (Stop button, auto-ranging)

## What's New in v7.1.1

### Bug Fixes
- ✅ **Fan control in calibration mode** — Fan now properly stops during calibration mode
  - Fixed PWM channel initialization order in boot sequence
  - Fan reliably stops for quiet operation during calibration

### v7.1.0 Features (included)
- ✅ **Runtime calibration system** — Calibrate without recompilation
- ✅ **Boot-time calibration mode** — Hold encoder button during power-on
- ✅ **ESP32 Preferences storage** — Calibration persists across firmware updates
- ✅ **Serial command interface** — Calibration commands via serial terminal
- ✅ **Binary distribution friendly** — No development tools needed for calibration

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
  0x10000 firmware.bin
```

**Linux/macOS:**
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
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
