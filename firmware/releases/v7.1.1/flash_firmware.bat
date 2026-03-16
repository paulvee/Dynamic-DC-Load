@echo off
echo ============================================
echo Dynamic Load Firmware v7.0.3 Flash Tool
echo ============================================
echo.
echo This script will flash the firmware to your ESP32.
echo Make sure your ESP32 is connected via USB.
echo.
pause

echo.
echo Detecting COM ports...
python -m serial.tools.list_ports
echo.

set /p COM_PORT="Enter your COM port (e.g., COM3): "

echo.
echo Flashing firmware to %COM_PORT%...
echo.

python -m esptool --chip esp32 --port %COM_PORT% --baud 460800 write_flash -z 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo SUCCESS! Firmware v7.0.3 flashed successfully.
    echo ============================================
    echo.
    echo You can now disconnect and reconnect your ESP32.
    echo The Dynamic Load should start automatically.
) else (
    echo.
    echo ============================================
    echo ERROR: Flashing failed!
    echo ============================================
    echo.
    echo Common solutions:
    echo 1. Make sure Python and esptool are installed: pip install esptool
    echo 2. Check that the correct COM port was entered
    echo 3. Try holding the BOOT button during flashing
    echo 4. Try a different USB cable or port
)

echo.
pause
