# Building Battery Tester Executables

## Overview

The Battery Tester app can be built into standalone executables for Windows, macOS, and Linux using PyInstaller.

**Important:** You must build on each target platform - cross-compilation is not supported.

## Prerequisites

1. Python 3.8 or higher
2. All dependencies installed: `pip install -r requirements.txt`
3. PyInstaller: Will be auto-installed by build script if needed

## Quick Build

```bash
# Activate virtual environment (recommended)
# Windows:
.venv\Scripts\activate

# macOS/Linux:
source .venv/bin/activate

# Run build script
python build_executable.py
```

The executable will be created in the `dist/` folder.

## Platform-Specific Notes

### Windows

- Icon: `icon.ico` (included from original Delphi project)
- Output: `dist/BatteryTester.exe`
- No console window (GUI only)

### macOS

- Icon: Needs `icon.icns` format
- Convert from .ico: 
  ```bash
  # Install imagemagick if needed: brew install imagemagick
  convert icon.ico icon.icns
  ```
- Output: `dist/BatteryTester` (macOS app bundle can be created with `--windowed` flag)
- Note: On first run, users may need to right-click → Open to bypass Gatekeeper

### Linux

- Icon: Can use `icon.ico` (will work with most desktop environments)
- Output: `dist/BatteryTester`
- Make executable: `chmod +x dist/BatteryTester`
- Users need serial port permissions: `sudo usermod -a -G dialout $USER` (logout/login required)

## Distribution

After building:

1. Test the executable on the target platform
2. Copy `dist/BatteryTester` (or .exe) to distribution folder
3. Include `config.ini.example` as reference (app will create config.ini on first run)
4. Package with README instructions

## Troubleshooting

### "Module not found" errors
- Ensure all dependencies in requirements.txt are installed
- Check the build script's `--hidden-import` list

### Large executable size
- Normal for PyInstaller (includes Python runtime + all dependencies)
- Typical size: 50-100 MB
- Use `--onedir` instead of `--onefile` for faster startup (creates folder with dependencies)

### Icon not appearing
- **Windows**: Verify `icon.ico` exists in project root
- **macOS**: Convert to .icns format first
- **Linux**: Some desktop environments may not show application icons

### Serial port access denied
- **Windows**: Usually works without special permissions
- **macOS**: Grant terminal/app "Full Disk Access" in System Preferences → Security & Privacy
- **Linux**: Add user to dialout group (see above)

## Manual Build (Advanced)

If you prefer manual control:

```bash
pyinstaller --onefile --windowed --name BatteryTester --icon=icon.ico \
    --add-data "config.ini.example:." \
    --hidden-import PyQt6.QtCore \
    --hidden-import PyQt6.QtGui \
    --hidden-import PyQt6.QtWidgets \
    main.py
```

## Clean Build

To start fresh:

```bash
# Remove build artifacts
rm -rf build/ dist/ *.spec

# Or let the build script clean (it will prompt)
python build_executable.py
```
