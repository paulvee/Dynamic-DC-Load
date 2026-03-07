#!/usr/bin/env python3
"""
Battery Tester Application - Main Entry Point

Python port of the Delphi Battery Tester application.
For testing lithium batteries using Arduino/ESP32 Dynamic Load controller.
"""

import sys
from PyQt6.QtWidgets import QApplication
from gui import MainWindow


def main():
    """Main application entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("Battery Tester")
    app.setOrganizationName("BatteryTester")
    
    # Create and show main window
    window = MainWindow()
    window.show()
    
    # Run application
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
