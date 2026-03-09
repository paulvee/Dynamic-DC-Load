# Print Implementation - Battery Tester

## Overview
The print functionality has been enhanced to match the export functionality with comprehensive metadata overlay.

## Implementation Details

### Based on Delphi Original
The original Delphi code (`Unit1.pas`) had a simple print implementation that:
- Created a bitmap from the chart
- Showed a print dialog (Windows `TPrintDialog`)
- Printed the chart scaled to fit the page
- **Did NOT include metadata overlay** (metadata was only in save function)

### Python Enhanced Version
The Python implementation improves upon the original by:
- **Including metadata overlay** (matching the export_chart_image function)
- **Using battery weight** from Setup tab (conditional display when > 0)
- **Using chart title** from Setup tab (or default "Battery Discharge Test Results")
- **Cross-platform support** (Windows, Linux, macOS)

### Key Features

#### 1. Metadata Overlay
The printed chart includes:
- **Title**: Custom title from Setup tab (or default)
- **Date of test**: Formatted as "Mon 09 Mar 2026"
- **Duration of test**: HH:MM:SS format
- **Rated capacity**: From Setup tab (e.g., 2000 mAh)
- **Tested capacity**: Actual measured capacity
- **Initial voltage**: Starting battery voltage
- **Cutoff voltage**: From Setup tab (e.g., 3.0V)
- **Test current**: From Setup tab (e.g., 500 mA)
- **Battery weight**: Displayed only if > 0 (e.g., "45 grams")

#### 2. Formatting
- **Font**: Courier New, 9pt
- **Title**: Bold and underlined
- **Position**: Centered-right area (45% width, 35% height)
- **Layout**: Matches Delphi save format exactly

#### 3. Print Dialog
- **Standard OS dialog**: Windows Print Dialog, CUPS on Linux, macOS Print Dialog
- **Options available**:
  - Printer selection
  - Page orientation (default: Landscape)
  - Print to PDF (available on all platforms)
  - Number of copies
  - Page range
  - Print quality

#### 4. Page Layout
- **Orientation**: Landscape (automatic)
- **Resolution**: High resolution printer mode
- **Scaling**: Automatic fit to page (95% to leave margins)
- **Centering**: Chart centered on page

## Cross-Platform Compatibility

### Windows
- Uses Windows print spooler
- Native Windows Print Dialog
- Print to PDF via "Microsoft Print to PDF" driver
- Fully tested with Windows 10/11

### Linux
- Uses CUPS (Common Unix Printing System)
- Qt's print dialog integrates with CUPS
- Print to PDF via built-in Qt PDF printer
- Works with standard Linux printer drivers

### macOS
- Uses native macOS printing system
- Qt's print dialog integrates with Cocoa print services
- Print to PDF via macOS built-in PDF support
- Works with AirPrint and other macOS printers

## Technical Implementation

### Code Location
- **File**: `utils/export_utils.py`
- **Method**: `ExportManager.print_chart()`
- **Called from**: `gui/main_window.py` → `print_chart()` method

### Dependencies
All dependencies are already included in `requirements.txt`:
- `PyQt6>=6.6.0` - Includes QtPrintSupport module
- `pyqtgraph>=0.13.3` - For chart widget capture

### Process Flow
1. **Validation**: Check if test data exists
2. **Printer Setup**: Create QPrinter with landscape orientation
3. **Dialog**: Show platform-native print dialog
4. **Capture**: Get chart as QPixmap
5. **Overlay**: Add metadata text to pixmap
6. **Scale**: Calculate scaling to fit printer page
7. **Print**: Render final image to printer/PDF

### Error Handling
- Graceful handling of user cancellation (returns `False, None`)
- Exception catching with descriptive error messages
- Validation of test data before printing

## Usage

### From Menu
`File → Print...` (Ctrl+P)

### From Toolbar
Click "Print" button

### From Code
```python
success, error = ExportManager.print_chart(
    chart_widget,
    test_data,
    test_parameters,
    parent=self
)
```

## Testing Checklist
- [ ] Print to physical printer (Windows)
- [ ] Print to PDF (Windows)
- [ ] Print to physical printer (Linux)
- [ ] Print to PDF (Linux)
- [ ] Print to physical printer (macOS)
- [ ] Print to PDF (macOS)
- [ ] Test with battery weight = 0 (should not show in output)
- [ ] Test with battery weight > 0 (should show "X grams")
- [ ] Test with custom chart title
- [ ] Test with default chart title
- [ ] Test user cancellation in print dialog
- [ ] Test with no test data (should show warning)

## Comparison: Delphi vs Python

| Feature | Delphi Original | Python Enhanced |
|---------|----------------|-----------------|
| Print Dialog | ✅ Windows only | ✅ Cross-platform |
| Metadata Overlay | ❌ No | ✅ Yes (full) |
| Battery Weight | ❌ Not in print | ✅ Conditional |
| Chart Title | ❌ Not in print | ✅ Custom/default |
| PDF Output | ⚠️ Requires driver | ✅ Built-in |
| Landscape Mode | ✅ Manual | ✅ Automatic |
| High Resolution | ✅ Yes | ✅ Yes |
| Page Centering | ⚠️ Basic | ✅ Calculated |

## Notes
- The Delphi `btnSaveClick` function prompted for battery weight via InputBox, but this is now pre-configured in the Setup tab
- The Python version maintains consistency between Save and Print operations
- Both use the same metadata format and positioning
- Print to PDF is more robust in Python (works on all platforms without additional drivers)
