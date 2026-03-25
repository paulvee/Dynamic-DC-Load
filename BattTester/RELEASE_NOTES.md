# Battery Tester v2.1.2 - Windows Release

## 🎉 What's New in v2.1.2

### Bug Fixes
- **Communication Timeout Handling** - The app now automatically stops the test if the Dynamic Load reports a communication timeout (`Comm Timeout` or `PC Disconnected`). A warning dialog is shown so you know exactly why the test was terminated, rather than leaving the app hanging in an ambiguous state.

---

## What's New in v2.1.1

### Bug Fixes
- **Auto-ranging Axis Jump Fixed** - Toggling the voltage auto-range checkbox no longer causes a jarring axis jump. The initial range calculation now uses the same algorithm as live updates, and the axis only redraws when the range actually changes (prevents flicker). Minimum displayed span is 0.5 V to avoid over-zooming on stable voltages.
- **Gridline Alignment** - Auto-range mode now snaps the Y-axis to clean intervals (0.1 V, 0.2 V, 0.5 V, 1.0 V, 2.0 V, 5.0 V) so gridlines align neatly with the axis edges.

---

## What's New in v2.1.0

### New Features
- **Stop Button** - Gracefully stop discharge and monitor recovery voltage without canceling the test
- **Simplified Cutoff Voltage Input** - Single editable field with automatic recommendations based on battery type
- **Automatic Calculations** - Cutoff voltage and test time automatically calculated from battery parameters
- **Voltage Auto-ranging** - Optional automatic voltage axis scaling (keeps data between 10-90% of chart)
- **Current Trace Toggle** - Show/hide current graph independently

### Improvements
- **Streamlined Protocol** - Removed obsolete parameters (loop_delay, tolerance, beep) for firmware v7.1.2+
- **Better Data Handling** - Automatic sorting of data points by time for reliable chart rendering
- **Enhanced Tooltips** - Clearer guidance for all settings and controls
- **Per-cell Voltage Display** - Shows total and per-cell voltages for multi-cell batteries
- **Updated UI** - Cleaner layout with improved parameter organization

### Technical
- Compatible with ESP32 Dynamic Load firmware v7.1.2 or higher
- Standalone Windows executable (~64MB)
- No Python installation required
- All dependencies included (PyQt6, pyqtgraph, pandas, numpy)

## 📥 Download & Installation

1. Download `BatteryTester_v2.1.2.exe` from the [Releases page](https://github.com/paulvee/Dynamic-DC-Load/releases)
2. Save it anywhere (Desktop, Documents, etc.)
3. Double-click to run — no installation needed!
4. On first run, the app will create `config.ini` for your settings

> **⚠️ Windows SmartScreen Warning** — When running the `.exe` for the first time, Windows may show **"Windows protected your PC"**. This is expected for any unsigned executable and is not malware.
>
> **To run it:** click **"More info"** → **"Run anyway"**. This is a one-time confirmation per machine.
>
> **If you see "This app can't run on your PC"** instead: right-click the `.exe` → **Properties** → check **Unblock** at the bottom → click **OK**, then run again.
>
> The app is fully open source — source code is in the `BattTester/` folder of this repository.

## 📝 Usage

1. Connect your ESP32 Dynamic Load via USB
2. Launch BatteryTester
3. Select the correct COM port
4. Configure test parameters
5. Start your battery test

For detailed instructions, see the [README](README.md).

## 🐛 Known Issues

- First run may take 10-20 seconds to start (PyInstaller unpacking)
- Some antivirus software may flag the executable (false positive - it's unsigned)

## 🔗 Links

- [Source Code & Firmware](https://github.com/paulvee/Dynamic-DC-Load)
- [Releases](https://github.com/paulvee/Dynamic-DC-Load/releases)

## 🛠️ Building from Source

See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for how to build your own executable.

---

**Note for Linux/macOS users**: This release is Windows-only. To use on other platforms:
1. Clone the repository
2. Install Python dependencies: `pip install -r requirements.txt`
3. Run: `python main.py`

Or build your own executable using the build script on your platform.
