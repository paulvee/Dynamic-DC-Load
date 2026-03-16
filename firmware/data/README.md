# Dynamic Load Calibration Reference

This folder contains a reference `.ini` file showing the calibration parameter format. This file is **NOT used by the firmware** - it's kept as documentation only.

## Calibration System Overview

The Dynamic Load firmware (v7.1.1+) uses **ESP32 Preferences (NVS storage)** for calibration values. This means:
- ✅ No filesystem uploads needed
- ✅ Values persist across firmware updates
- ✅ Calibrate via serial commands only
- ✅ Works with pre-compiled binaries

## DL_Cal_Values.ini (Reference Only)

This file shows the format of calibration parameters but is **not read by the firmware**. Use it as a reference for the parameter names and default values.

### Calibration Parameters:

- **dutVcalib**: DUT voltage calibration factor (default: 1.0)
- **DUTCurrent**: DAC-ADC calibration point in mV (default: 400.00)
- **shuntVcalib**: Current display calibration factor (default: 2.5000)
- **cvCalFactor**: CV mode trigger point calibration (default: 1.0)

## How to Calibrate Your Load

### Step 1: Enter Calibration Mode

1. Power off the Dynamic Load and disconnect the serial cable
2. **Press and hold the encoder button**
3. Power on while continuing to hold the button
4. Keep holding until you see "CALIBRATION MODE" on the display
5. Release the button and connect a serial terminal (9600 baud)

**Timing tip:** Start pressing the button when the screen blanks out from the previous session, and keep it pressed through power-on.

**Once in calibration mode:** Type `CAL` (or `cal`) in the serial monitor to display the calibration command menu. You can then use any of the commands below.

### Step 2: Use Serial Commands

Available commands: 
```
CAL SHOW              - Display current calibration values
CAL CV <value>        - Set CV mode calibration factor
CAL DUTV <value>      - Set DUT voltage calibration factor
CAL DUTC <value>      - Set DAC-ADC calibration point
CAL SHUNT <value>     - Set shunt calibration factor
CAL SAVE              - Save values to persistent storage
CAL RESET             - Reset to factory defaults
CAL EXIT              - Exit calibration mode
```

**Features:**
- Lower/upper case is accepted
- Backspace works to correct typos
- Ctrl-C cancels the current line
- Press the encoder button anytime to exit calibration mode and continue normal boot

**Two Ways to Exit:**
1. **Button Click**: Press the encoder button to exit calibration and immediately continue with normal boot sequence (no power cycle needed)
2. **CAL EXIT Command**: Type `CAL EXIT` to show completion screen and halt - requires power cycle to restart with new calibration values

### Step 3: Example Calibration Session

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

> CAL EXIT
Exiting calibration mode - power cycle to restart and load the new values
```

### Step 4: Verify Calibration

After exiting and power cycling:
1. Enter calibration mode again
2. Type `CAL SHOW` to verify your values persisted
3. Press button to exit or type `CAL EXIT`

## Calibration Examples

### CV Mode Calibration

If your CV mode triggers at the wrong voltage:
1. Set CV mode to 50.0V
2. Measure actual trigger voltage (e.g., 53.03V with DMM)
3. Calculate: `cvCalFactor = measured / target = 53.03 / 50.0 = 1.0606`
4. Enter calibration mode: `CAL CV 1.0606` → `CAL SAVE`

### Voltage Display Calibration

If the voltage reading is incorrect:
1. Apply known voltage (e.g., 10.000V from calibrated source)
2. Note displayed voltage (e.g., 9.950V)
3. Calculate: `dutVcalib = actual / displayed = 10.000 / 9.950 = 1.005025`
4. Enter calibration mode: `CAL DUTV 1.005025` → `CAL SAVE`

## Technical Details

- **Storage:** ESP32 NVS (Non-Volatile Storage) in Preferences namespace "dl_cal"
- **Persistence:** Survives power cycles, firmware updates, and reflashing
- **Precision:** Double precision floating point (6+ decimal places)
- **Load time:** Values loaded during boot process
- **No filesystem needed:** Works without LittleFS or SPIFFS

## For Binary Distribution

This system enables distributing pre-compiled `.bin` files. Users can:
1. Flash firmware using any ESP32 tool
2. Enter calibration mode (no special software needed)
3. Calibrate via any serial terminal
4. Values persist even when reflashing firmware
