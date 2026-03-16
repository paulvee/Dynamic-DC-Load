# Calibration System - User Guide

## Overview

As of v7.0.4, the Dynamic Load firmware supports **runtime calibration** via serial commands. Calibration values are stored in the ESP32's non-volatile memory and persist across reboots and firmware updates.

**Key Benefits:**
- ✅ No recompilation needed to change calibration
- ✅ No filesystem uploads required
- ✅ Works with pre-compiled binaries
- ✅ Values persist across firmware updates
- ✅ Simple serial command interface

## Calibration Parameters

The following values can be calibrated:

| Parameter | Description | Default | Notes |
|-----------|-------------|---------|-------|
| `dutVcalib` | DUT voltage display calibration | 1.0 | Adjust if voltage reading is incorrect |
| `DUTCurrent` | DAC-ADC calibration point (mV) | 400.00 | deviation stored, but not used |
| `shuntVcalib` | Current display calibration | 2.5000 | Adjust if current reading is incorrect |
| `cvCalFactor` | CV mode trigger voltage calibration | 1.0 | **Most commonly adjusted** |

## Quick Start: Entering Calibration Mode

### Method: Button Hold at Boot

1. **Power off** the Dynamic Load completely
2. **Press and hold** the encoder button
3. **Power on** while continuing to hold the button
4. **Keep holding** until you see "CALIBRATION MODE" on the display
5. **Release** the button
6. **Connect** a serial terminal (9600 baud, COM port varies)

**Important Timing:** Start pressing the button when the screen blanks out from the previous session, and keep it pressed continuously through power-on until the calibration screen appears.

### Display Shows:
```
CALIBRATION MODE

Connect serial
Type CAL on
serial monitor
or click to
cancel
```

## Serial Command Reference

Once in calibration mode, use these commands:

### View Current Values
```
CAL SHOW
```
Displays all current calibration values with full precision.

### Set CV Mode Calibration
```
CAL CV 1.0606
```
Sets the CV mode trigger voltage calibration factor.

### Set Voltage Display Calibration
```
CAL DUTV 1.005
```
Adjusts the DUT voltage reading calibration.

### Set Current Calibration Point
```
CAL DUTC 400.00
```
deviation is stored here, but not used.

### Set Current Display Calibration
```
CAL SHUNT 2.5000
```
Adjusts the current reading calibration.

### Save Values to Memory
```
CAL SAVE
```
**Important:** Values are NOT saved until you run this command. Changes are temporary until saved with CAL SAVE.

### Reset to Factory Defaults
```
CAL RESET
```
Resets all values to defaults from Config.h. Use `CAL SAVE` afterwards to persist.

### Exit Calibration Mode
```
CAL EXIT
```
Exits calibration mode. Power cycle to return to normal operation.

**Or** press the encoder button at any time to exit.

## Command Line Features

- **Backspace:** Delete the last character
- **Ctrl-C:** Cancel current line and start over
- **Echo:** Characters are echoed as you type
- **Case insensitive:** `cal show`, `CAL SHOW`, `Cal Show` all work

## Complete Calibration Session Example

```
> CAL SHOW
=== Current Calibration Values ===
dutVcalib    : 1.000000
DUTCurrent   : 400.00
shuntVcalib  : 2.5000
cvCalFactor  : 1.000000
==================================

> CAL CV 1.0606
cvCalFactor set to: 1.060600
Use 'CAL SAVE' to persist

> CAL SAVE
Calibration values saved to Preferences

> CAL SHOW
=== Current Calibration Values ===
dutVcalib    : 1.000000
DUTCurrent   : 400.00
shuntVcalib  : 2.5000
cvCalFactor  : 1.060600
==================================

> CAL EXIT
Exiting calibration mode - power cycle to restart
```

## Calibration Procedures

### CV Mode Calibration (Most Common)

**Problem:** CV mode triggers at wrong voltage (e.g., set 50V but triggers at 53V)

**Note:** For an extended and detailed description of CV mode calibration, see the blog post:
https://www.paulvdiyblogs.net/2024/09/building-diy-dynamic-dc-load.html

**Procedure:**
1. Apply a known voltage from a calibrated power supply (e.g., 50.00V)
2. Set Dynamic Load to CV mode at that voltage (50.0V)
3. Note the actual trigger voltage shown on the supply (e.g., 53.03V)
4. Calculate calibration factor:
   ```
   cvCalFactor = measured_voltage / target_voltage
   cvCalFactor = 53.03 / 50.0 = 1.0606
   ```
5. Enter calibration mode and set:
   ```
   CAL CV 1.0606
   CAL SAVE
   ```
6. Power cycle and test again

### Voltage Display Calibration

**Problem:** Voltage display doesn't match calibrated DMM

**Procedure:**
1. Apply a known voltage (e.g., 10.000V from calibrated source)
2. Note the Dynamic Load's displayed voltage (e.g., 9.950V)
3. Calculate calibration factor:
   ```
   dutVcalib = actual_voltage / displayed_voltage
   dutVcalib = 10.000 / 9.950 = 1.005025
   ```
4. Enter calibration mode and set:
   ```
   CAL DUTV 1.005025
   CAL SAVE
   ```

### Current Display Calibration

**Problem:** Current display doesn't match calibrated ammeter

**Procedure:**
1. Set a known current load (e.g., 1.000A constant current mode)
2. Measure with a calibrated ammeter in series
3. Note measured vs. displayed current
4. Calculate adjustment factor:
   ```
   shuntVcalib = (measured / displayed) × current_shuntVcalib
   ```
5. Enter calibration mode and set:
   ```
   CAL SHUNT 2.5125
   CAL SAVE
   ```

## Verifying Calibration Persists

After saving calibration:

1. Type `CAL EXIT` or press the button
2. Power cycle the Dynamic Load
3. Enter calibration mode again (hold button at boot)
4. Type `CAL SHOW`
5. Verify your saved values are displayed

If values show as defaults, calibration was not saved - repeat `CAL SAVE`.

## Troubleshooting

### Can't Enter Calibration Mode

**Symptoms:** Display shows normal operation, not calibration screen

**Solutions:**
- Start pressing button earlier (when screen blanks)
- Hold button continuously through entire boot sequence
- Don't release until "CALIBRATION MODE" appears
- Try multiple times - timing can be tricky

### Values Not Saving

**Symptoms:** After power cycle, values return to defaults

**Solutions:**
- Make sure you ran `CAL SAVE` before exiting
- Check serial output for "Calibration values saved to Preferences"
- If save failed, try again

### Commands Not Working

**Symptoms:** "ERROR: Unknown command" messages

**Solutions:**
- Check command spelling: `CAL SHOW` not `SHOW CAL`
- Ensure space between CAL and parameter/value
- Use Backspace or Ctrl-C to fix typos
- Valid format: `CAL <command>` or `CAL <param> <value>`

### Serial Monitor Issues

**Symptoms:** No echo, garbled text

**Solutions:**
- Verify baud rate: 9600
- Try different serial terminal (PuTTY, Arduino IDE, PlatformIO)
- Check COM port is correct
- Try disconnect/reconnect USB

## Technical Details

### Storage Technology
- **Method:** ESP32 Preferences library
- **Namespace:** "dl_cal"
- **Storage:** NVS (Non-Volatile Storage) partition
- **Persistence:** Survives firmware reflashing
- **Precision:** Double precision (15+ significant digits)

### Memory Location
- Stored in separate NVS partition on ESP32 flash
- Not part of firmware image
- Not affected by `pio run --target upload`
- Erased only by explicit NVS erase or full chip erase

### Loading Process
At boot, the firmware:
1. Initializes Preferences library
2. Checks for saved calibration in "dl_cal" namespace
3. Loads saved values if found
4. Uses defaults from Config.h if not found
5. Prints status to serial monitor

## For Developers

### Adding New Calibration Values

To add a new calibration parameter:

1. Add `#define DEFAULT_NEW_PARAM value` to Config.h
2. Add `extern double newParam;` to DynamicLoad.h
3. Add `double newParam = DEFAULT_NEW_PARAM;` to main.cpp
4. Update CalibrationManager::loadCalibration()
5. Update CalibrationManager::saveCalibration()
6. Update CalibrationManager::printCalibration()
7. Add command parsing in CalibrationManager::processCommand()

### Resetting All Calibration (Factory Reset)

To completely erase calibration from NVS:
```cpp
preferences.begin("dl_cal", false);
preferences.clear();
preferences.end();
```

Or use the built-in command:
```
CAL RESET
CAL SAVE
```
