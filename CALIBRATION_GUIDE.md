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
| `DAC_ADC_TOLERANCE` | Measured voltage at DAC-ADC calibration point (mV) | 400.00 | Deviation stored, but not actively used |
| `iCalLow` | Current correction factor at low calibration point | 1.0 | Set automatically by `CAL CURRL` |
| `iRefLow` | Reference current at the low calibration point (stored in A) | 0.1 A (100 mA) | Set automatically by `CAL CURRL` |
| `iCalHigh` | Current correction factor at high calibration point | 1.0 | Set automatically by `CAL CURRH` |
| `iRefHigh` | Reference current at the high calibration point (stored in A) | 8.0 A (8000 mA) | Set automatically by `CAL CURRH` |
| `cvCalFactor` | CV mode trigger voltage calibration | 1.0 | **Most commonly adjusted** |

## Quick Start: Entering Calibration Mode

### Method: Button Hold at Boot

1. **Connect** a serial terminal (9600 baud, see note below)
2. **Press and hold** the encoder button
3. **Momentarily press the EN button on the ESP32 board** to reset the ESP while continuing to hold the button
4. **Keep holding** until you see "CALIBRATION MODE" on the display
5. **Release** the rotary encoder button

> **⚠️ Serial monitor compatibility — read before connecting:**
>
> | Terminal | Works? | Notes |
> |---|---|---|
> | **PuTTY** | ✅ Yes | Recommended. Set flow control to **XON/XOFF** or **None**. Works out of the box. |
> | **Arduino IDE** (v2.x) | ✅ Yes | Tested with v2.3.7 and v2.3.8. Works out of the box. Note: behaviour may change with future IDE updates. |
> | **Tera Term 5** | ⚠️ Config needed | Works reliably once configured. Before opening the port: **Setup → Serial Port → uncheck DTR and RTS**. Without this, DTR asserted on connect causes a reset/reconnect flood. |
> | **VSCode built-in serial monitor** | ❌ No | Asserts DTR/RTS on data send. ESP32 resets the moment you type a command. |
> | **PlatformIO `pio device monitor`** | ❌ No | Same RTS toggle issue as VSCode monitor, even with `monitor_dtr=0` / `monitor_rts=0` in `platformio.ini`. |
>
> **Recommended:** Use **PuTTY** (reliable, always works out of the box) or **Arduino IDE 2.x**.
>
> ⚠️ **Note:** Serial monitor DTR/RTS handling is controlled entirely by the host application, not the firmware. Any update to PuTTY, Arduino IDE, Tera Term, or VSCode could change this behaviour. If calibration mode stops working after a software update, DTR/RTS handling in that terminal is the first thing to check.

**Important Timing:** Start pressing the rotary encoder button before you reset the ESP32 or start pressing when the screen blanks out from the previous session, and keep it pressed continuously through power-on until the calibration screen appears.

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
CAL CV 1.0000
```
Sets the CV mode trigger voltage calibration factor.

### Set Voltage Display Calibration
```
CAL DUTV 1.0000 (can be 0.xxxx or 1.xxxx)
```
Adjusts the DUT voltage reading calibration.

### Set DAC-ADC Calibration Point
```
CAL DUTC 400.00
```
Stores the measured voltage at the DAC-ADC calibration point based on component tolerances. Deviation is stored but not actively used in calculations.

### Calibrate Low Current Point
```
CAL CURRL 100 99
```
Sets the low-current calibration point. Supply two values in **mA**: the **set current** (what the encoder was set to) then the **OLED reading** (what the display showed, converted to mA). The firmware calculates the correction factor as `set / oled` and stores the reference internally in A.

### Calibrate High Current Point
```
CAL CURRH 3000 3022
```
Same as above but for the high-current point. Use the highest current your setup can comfortably deliver. The firmware applies linear interpolation between the two points for any current in between, and uses the nearest flat factor outside that range.

> **Note:** Both commands accept mA values (e.g. `100 99`) or A values (e.g. `0.100 0.099`) — values > 10 are automatically treated as mA and converted.

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
Exits calibration mode. The OLED and serial monitor will display instructions to safely terminate the session.

**Or** press the encoder button at any time to exit.

> **⚠️ Important — correct exit sequence:**
> 1. Type `CAL EXIT` and press Enter
> 2. Read the OLED and serial monitor message
> 3. **Disconnect the USB cable first**
> 4. Close the serial monitor
> 5. Reconnect the USB cable — the ESP32 boots normally into measurement mode
> 6. Alternatively, after disconnecting the USB cable, press the rotary encoder button to restart
>
> **If you already closed the serial monitor without disconnecting first:** The ESP32 will be unresponsive — the OLED freezes on its last screen, the EN button has no effect. To recover:
> 1. Pull the USB cable
> 2. Power cycle the board (disconnect and reconnect the DC power supply)
> 3. Reconnect the USB cable — the ESP32 boots normally
> Opening the serial monitor or PuTTY after reconnecting is safe; the 10µF capacitor on the EN line prevents a reset-on-connect. This is **C44** on the PCB — it must always be installed.

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
DAC_ADC_TOLER: 400.00
iCalLow      : 1.000000
iRefLow  (A) : 0.100
iCalHigh     : 1.000000
iRefHigh (A) : 8.000
cvCalFactor  : 1.000000
==================================

> CAL CV 1.0000
cvCalFactor set to: 1.000000
Use 'CAL SAVE' to persist

> CAL DUTV 1.0000
dutVcalib set to: 1.000000
Use 'CAL SAVE' to persist

> CAL CURRL 100 99
iCalLow set to: 1.010101
iRefLow set to: 0.100
Use 'CAL SAVE' to persist

> CAL CURRH 3000 3022
iCalHigh set to: 0.992720
iRefHigh set to: 3.000
Use 'CAL SAVE' to persist

> CAL SAVE
Calibration values saved to Preferences

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

### Current Display Calibration (Two-Point)

In this calibration you are making the OLED current display match what you set with the rotary encoder. When you set 100 mA, you want the OLED to show 100 mA — and the power supply current should agree. Any deviation is measured at two points (low and high), and the firmware interpolates between them so the correction applies accurately across the full range.

No ammeter is required. Connect your power supply to the load (sense leads on, sense enabled), set the voltage to 10V and the current limit to just above your intended high calibration point.

**Phase 1 — Collect the readings in normal operation**

**1.** Boot the Dynamic Load normally (do not hold the encoder button).

**2.** In CC mode, use the encoder to set the load to **100 mA** and turn the load ON. Give the display a moment to settle.

**3.** Note the OLED current reading. For example: encoder set = **100 mA**, OLED shows **0.099 A** → note this as **99 mA**.

**4.** Turn the encoder to increase the current to the highest value your setup allows (e.g. 3000 mA, 6000 mA, 8000 mA). Wait for the display to settle.

**5.** Note the OLED reading again. For example: encoder set = **3000 mA**, OLED shows **3.022 A** → note this as **3022 mA**.

> **Note:** The high point does not have to be 8 A — use whatever maximum current your setup can actually deliver.

**6.** Turn the load OFF.

**Phase 2 — Enter calibration mode and type the values**

**7.** Enter calibration mode: hold the encoder button, momentarily press the EN button on the ESP32 while keeping it pressed, and hold until you see "CALIBRATION MODE" on the display. Release the button.

**8.** Type the low-point pair in mA — set current first, then OLED reading — and press Enter:
```
CAL CURRL 100 99
```

**9.** Type the high-point pair in mA and press Enter:
```
CAL CURRH 3000 3022
```

**10.** Save:
```
CAL SAVE
```

**11.** Exit calibration mode, power cycle, and verify. Set the load to a mid-range current and check that the OLED matches. If anything looks off, repeat Phase 1 for just that point and redo the corresponding `CAL CURRL` or `CAL CURRH` — you don't need to redo both.

> **Note:** The two correction factors act independently. Below your low reference current the low-point factor is used flat. Above your high reference current the high-point factor is used flat. In between, the firmware interpolates linearly.

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

**Symptoms:** Typed commands are not recognised or produce no output

**Solutions:**
- Check baud rate is 9600
- Ensure local echo is enabled in your terminal so you can see what you type
- Commands are case-insensitive; try `cal show` or `CAL SHOW`
- Press Enter after the command (not just Ctrl+Enter)

### ESP32 Stuck in Reset After Closing Serial Monitor

**Symptoms:** OLED freezes on its last screen, ESP32 is completely unresponsive — the EN button has no effect

**Cause:** The serial monitor was closed while the USB cable was still connected. This de-asserts DTR on the host side, which pulls the ESP32 EN pin low through the auto-reset circuit and holds the processor in a perpetual reset state. Because EN is held low by the USB-serial bridge, the EN button cannot override it — only removing USB power breaks the hold.

**Recovery:**
1. Pull the USB cable
2. Power cycle the board (disconnect and reconnect the DC power supply)
3. Reconnect the USB cable — the ESP32 boots normally

Opening a serial monitor or PuTTY after reconnecting is safe; the 10µF capacitor on the EN line absorbs the DTR pulse on connect and prevents a reset. This is **C44** on the PCB — it must always be installed.

**Prevention:** Always disconnect the USB cable *before* closing the serial monitor. The OLED and serial monitor both display this reminder after `CAL EXIT`.

---

### ESP32 Resets When Typing Commands

**Symptoms:** ESP32 reboots (OLED goes dark, serial disconnects) the moment you send a command; works fine when idle

**Cause:** This is a well-known, long-standing issue with the Arduino/ESP32 auto-reset circuit. The USB-to-serial bridge (CP2102/CH340) wires DTR and RTS to the ESP32 EN pin via an RC circuit, intended for automatic programming resets. Some serial monitors toggle these lines during data transmission, which pulses EN and resets the CPU. This cannot be fixed in firmware — the processor is in reset before it can react.

**Affected monitors (do not use for calibration):**
- **VSCode built-in serial monitor** — resets ESP32 on the first keystroke
- **PlatformIO `pio device monitor`** — same issue, even with `monitor_dtr=0` / `monitor_rts=0` in `platformio.ini`

**Working monitors (out of the box):**
- **PuTTY** ✅ — Baud: 9600 | Data: 8 | Stop: 1 | Parity: None | Flow control: **XON/XOFF** or **None**
- **Arduino IDE 2.x** ✅ — Tested with v2.3.7 and v2.3.8. Note: behaviour could change with future IDE updates.

**Works with manual configuration:**
- **Tera Term 5** ⚠️ — Asserts DTR on connect by default, causing a reset/reconnect flood. Fix before opening the port: **Setup → Serial Port → uncheck DTR and RTS**. Once configured, works reliably. Note: initial garbage characters after a failed attempt are TX FIFO leftovers — they clear on the next clean connect.

The PlatformIO `pio device monitor` works normally for all non-interactive (read-only) firmware use.

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
