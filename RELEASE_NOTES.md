# Battery Tester v2.1.0 - Windows Release

## 🎉 What's New in v2.1.0

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

## 📥 Download

Download **BatteryTester-Windows.exe** and run it directly.

## 💻 System Requirements

- **OS**: Windows 10 or Windows 11
- **Hardware**: Serial/USB port for ESP32 communication
- **Permissions**: Serial port access (typically automatic)

## 🚀 Installation

1. Download `BatteryTester-Windows.exe`
2. Save it anywhere (Desktop, Documents, etc.)
3. Double-click to run - no installation needed!
4. On first run, the app will create `config.ini` for your settings

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

- [Source Code](https://github.com/paulvee/Battery-tester-Python)
- [ESP32 Firmware](https://github.com/paulvee/DL-Firmware-for-VSC)
- [Hardware Project](https://github.com/paulvee/Dynamic-DC-Load)

## 🛠️ Building from Source

See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for how to build your own executable.

---

**Note for Linux/macOS users**: This release is Windows-only. To use on other platforms:
1. Clone the repository
2. Install Python dependencies: `pip install -r requirements.txt`
3. Run: `python main.py`

Or build your own executable using the build script on your platform.
