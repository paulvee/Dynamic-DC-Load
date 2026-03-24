# Battery Tester - Python Version

A modern Python/PyQt6 application for battery discharge testing and analysis. Works with ESP32-based Dynamic Load controller (firmware v7.1.2 or higher) to test NiMH, NiCd, Alkaline, LiPo, LiFePO4, and LiHV batteries.

## Requirements

- Python 3.9 or higher
- ESP32 Dynamic Load controller with firmware v7.1.2 or higher
- USB serial connection
- Dependencies: PyQt6, pyqtgraph, pandas, numpy, pyserial

## Quick Start

1. Install Python 3.9 or higher
2. Clone or download this repository
3. Create and activate a virtual environment
4. Install dependencies: `pip install -r requirements.txt`
5. Connect your ESP32 Dynamic Load via USB
6. Run: `python main.py`
7. Select your COM port and click Connect
8. Configure test parameters and click Start

## Windows SmartScreen Warning

When running the downloaded `.exe` for the first time, Windows may show:
**"Windows protected your PC"** — this is expected for any unsigned executable and does not indicate malware.

**To run it:**
1. Click **"More info"** in the SmartScreen dialog
2. Click **"Run anyway"**

This is a one-time confirmation per machine. After that it runs without prompting.

**If you see "This app can't run on your PC"** (different message):
1. Right-click the `.exe` → **Properties**
2. At the bottom, check **Unblock** → click **OK**
3. Then run it again (you may still need to click "More info" → "Run anyway" once)

> The app is open source — you can inspect the full source code in the `BattTester/` folder of this repository.

## Installation

### Prerequisites
- Python 3.9 or higher
- ESP32/Arduino Dynamic Load controller connected via USB

### Setup

1. **Navigate to Project Directory**

Windows:
```bash
cd "C:\Users\pwver\Documents\Hobby\Battery Tester\Batt Tester"
```

Linux/Mac:
```bash
cd /path/to/batt_tester
```

2. **Create Virtual Environment**
```bash
python -m venv venv
```

3. **Activate Virtual Environment**

Windows:
```bash
venv\Scripts\activate
```

Linux/Mac:
```bash
source venv/bin/activate
```

4. **Install Dependencies**
```bash
pip install -r requirements.txt
```

## Usage

Navigate to the project directory first, then run:

```bash
cd "C:\Users\pwver\Documents\Hobby\Battery Tester\Batt Tester"
python main.py
```

### Stop vs Cancel

The application provides two ways to end a test:

- **Stop Button**: Gracefully stops the discharge and enters recovery monitoring mode. Use this for normal test completion to collect battery recovery data.
- **Cancel Button**: Immediately terminates the test (requires confirmation). Use this only when you need to abort the test completely.

## Project Structure

```
Batt Tester/
├── main.py                 # Application entry point
├── requirements.txt        # Python dependencies
├── config.ini             # User configuration (auto-generated)
│
├── gui/                   # User interface
│   ├── main_window.py     # Main application window
│   ├── about_dialog.py    # About dialog
│   └── new_test_dialog.py # New test confirmation
│
├── hardware/              # Hardware communication
│   ├── serial_controller.py  # Serial port management
│   ├── protocol.py           # ESP32 protocol implementation
│   └── port_scanner.py       # COM port detection
│
├── core/                  # Business logic
│   ├── test_manager.py    # Test state machine
│   ├── data_collector.py  # Real-time data buffering
│   ├── calculations.py    # Calculations and utilities
│   └── models.py          # Data models
│
├── utils/                 # Helper utilities
│   ├── config_manager.py  # Configuration persistence
│   ├── csv_exporter.py    # CSV export
│   └── image_exporter.py  # Chart image export
│
└── resources/             # Application resources
    └── icons/             # Icon files
```

## Features

### Testing & Monitoring
- ✅ Real-time battery discharge monitoring
- ✅ Live voltage and current charting with pyqtgraph
- ✅ Optional voltage axis autoranging (keeps data between 10-90% of chart window)
- ✅ Toggle current trace visibility on chart
- ✅ Multiple battery chemistry support (NiMH, NiCd, Alkaline, LiPo, LiFePO4, LiHV)
- ✅ Support for multiple cells like battery packs
- ✅ Configurable discharge current, cutoff voltage, and time limits
- ✅ Recovery voltage monitoring after discharge
- ✅ Stop button for graceful test termination (preserves recovery data)
- ✅ Cancel button for immediate test abort

### Data Management
- ✅ Data export (CSV, PNG, JPG)
- ✅ Automatic data point sorting (handles out-of-order data)
- ✅ Optional chart title and battery weight metadata
- ✅ Dark/light chart background themes
- ✅ Configuration persistence

### Hardware Communication
- ✅ Serial communication with ESP32 Dynamic Load controller
- ✅ Firmware v7.1.2+ protocol support
- ✅ Automatic port scanning and detection
- ✅ Verbose logging option for debugging

### User Interface
- ✅ Simplified cutoff voltage input with recommended defaults
- ✅ Automatic cutoff voltage calculation based on battery chemistry and cell count
- ✅ Automatic test time calculation based on battery capacity and discharge current
- ✅ Per-cell voltage display for multi-cell batteries
- ✅ Auto-populated test parameters based on battery type
- ✅ Cross-platform support (Windows, Linux, macOS)

## Serial Protocol

The application communicates with the ESP32 controller using a custom ASCII protocol. Designed for firmware v7.1.2+ with streamlined parameter set:
- Current (mA)
- Cutoff voltage (V)
- Time limit (minutes)
- Sample interval (seconds)
- Recovery time (minutes)

See hardware documentation for detailed protocol specifications.

## Building Executable

To create a standalone executable:

```bash
python build_executable.py
```

The executable will be created in the `dist/` folder (~64MB for Windows). It includes all dependencies and can be distributed without requiring Python installation.

## Development Status

✅ **Active Development** - Fully functional with ongoing improvements

Recent updates:
- Simplified UI with single editable cutoff voltage field
- Added Stop button for graceful test termination
- Streamlined firmware protocol (v7.x compatibility)
- Fixed data sorting for reliable chart rendering
- Improved tooltips and user guidance

## License

MIT License

Copyright (c) 2026 Paul Versteeg & Bud Bennett

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Credits

**Authors:** Paul Versteeg & Bud Bennett (2026)  
**Original Concept:** Based on Delphi Battery Tester by John Lowen (2019, in memory)  
**Firmware:** ESP32 Dynamic Load controller v7.1.2+
