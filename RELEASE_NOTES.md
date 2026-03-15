# Battery Tester v2.0.1 - Windows Release

## 🎉 What's New

### Features
- **Standalone Windows executable** - No Python installation required
- **Original Delphi icon** - Familiar look from the previous version
- **Single file distribution** - Just download and run
- **All dependencies included** - PyQt6, pyqtgraph, pandas, numpy bundled

### Improvements
- Professional executable packaging with PyInstaller
- Clean GUI with no console window
- ~63MB single file (includes entire Python runtime)

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
