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
- **System dependency required:** PyQt6's `xcb` platform plugin needs `libxcb-cursor0`, which is
  not preinstalled on many distros (confirmed missing by default on Linux Mint 21/22 and Ubuntu).
  Without it, the built executable aborts on startup with:
  `qt.qpa.plugin: Could not load the Qt platform plugin "xcb"`.
  Install it with:
  ```bash
  sudo apt update && sudo apt install -y libxcb-cursor0
  ```
  This applies both to the machine building the executable and to any machine running it
  afterwards (Debian/Ubuntu/Mint family; other distros use the equivalent package, e.g.
  `xcb-util-cursor` on Fedora/Arch).

#### Verifying build readiness before running `build_executable.py` (Linux)

These checks catch the two most common Linux build blockers *before* spending time on a build:

```bash
# 1. Confirm Python 3.8+ is available
python3 --version

# 2. Create and activate a virtual environment, then install dependencies
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

# 3. Verify PyQt6 can actually load Qt (catches missing libxcb-cursor0 etc. early,
#    without needing a real display via QT_QPA_PLATFORM=offscreen)
QT_QPA_PLATFORM=offscreen python -c "from PyQt6.QtWidgets import QApplication; QApplication([]); print('PyQt6 loaded OK')"

# 4. Confirm a display session is available for the real GUI
echo "DISPLAY=$DISPLAY  WAYLAND_DISPLAY=$WAYLAND_DISPLAY  XDG_SESSION_TYPE=$XDG_SESSION_TYPE"

# 5. Confirm serial port permissions (needed to talk to the Arduino/hardware)
groups $USER | tr ' ' '\n' | grep -x dialout && echo "user IS in dialout group" || echo "user NOT in dialout group"
```

If step 3 fails with a `libxcb-cursor.so.0` or similar `qt.qpa.plugin` error, install
`libxcb-cursor0` as shown above and re-run the check. If step 5 reports the user is not in
`dialout`, run `sudo usermod -a -G dialout $USER` and log out/in before testing serial
communication.

After building, do a quick smoke test of the actual binary before distributing it:

```bash
chmod +x dist/BatteryTester
timeout 5 ./dist/BatteryTester; echo "EXIT_CODE=$?"
```

An exit code of `124` means `timeout` killed a still-running (i.e. successfully launched) GUI
process — that's the expected, healthy result. A crash or Qt platform plugin error here means a
system dependency (see above) is still missing.

#### Installing an app launcher (Linux)

To make the built executable launchable from the desktop's app menu (Cinnamon, GNOME, KDE, etc.)
instead of always running it from a terminal, register it as a user-level desktop application.
This does **not** require `sudo` — it only affects your own user account.

Because each rebuild produces a version-named binary (e.g. `BatteryTester_v2.1.2-linux`, then
`BatteryTester_v2.1.3-linux`, ...), the launcher points at a stable symlink rather than a
specific versioned file, so you only need to update one symlink after each rebuild instead of
editing the launcher itself.

```bash
# 1. Create a stable support directory (one-time)
mkdir -p ~/.local/share/battery-tester

# 2. Convert the Windows .ico icon to PNG (Linux desktop menus don't reliably render .ico)
python3 -c "
from PIL import Image
img = Image.open('icon.ico')
img.save('/home/$USER/.local/share/battery-tester/icon.png')
"

# 3. Symlink the current release binary to a stable, version-independent path
ln -sf "$(pwd)/release/BatteryTester_v2.1.2-linux" ~/.local/share/battery-tester/BatteryTester

# 4. Create the desktop entry (registers the app for your user only)
cat > ~/.local/share/applications/battery-tester.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=Battery Tester
Comment=Dynamic DC Load Battery Discharge Tester
Exec=/home/USERNAME/.local/share/battery-tester/BatteryTester
Icon=/home/USERNAME/.local/share/battery-tester/icon.png
Terminal=false
Categories=Utility;Electronics;
StartupNotify=true
EOF
# Replace USERNAME with your actual username in the Exec/Icon lines above

# 5. Make it executable and refresh the desktop menu cache
chmod +x ~/.local/share/applications/battery-tester.desktop
update-desktop-database ~/.local/share/applications
desktop-file-validate ~/.local/share/applications/battery-tester.desktop && echo "Desktop file is VALID"
```

"Battery Tester" should now appear in the app menu (search for "Battery"); log out/in if it
doesn't show immediately.

**After rebuilding a newer version**, just repoint the symlink — no need to touch the
`.desktop` file:

```bash
ln -sf "$(pwd)/release/BatteryTester_v<NEW_VERSION>-linux" ~/.local/share/battery-tester/BatteryTester
```

Requires Pillow (`pip install Pillow`, or it's already available if installed system-wide/via
another dependency) for the icon conversion step. If you'd rather skip Python, ImageMagick's
`convert icon.ico icon.png` works the same way.

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

### Linux: "Could not load the Qt platform plugin 'xcb'" / app aborts on startup
- Missing system library `libxcb-cursor0` (Qt6 requirement, not bundled by PyInstaller)
- Fix: `sudo apt update && sudo apt install -y libxcb-cursor0`, then re-run the executable
- Verify with the offscreen `QApplication` check described above

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
