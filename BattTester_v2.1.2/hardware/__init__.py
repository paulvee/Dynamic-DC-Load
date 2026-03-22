"""
Hardware communication package for Battery Tester.

This package handles all serial communication with the Arduino/ESP32 controller.
"""

from .serial_controller import SerialController, ConnectionState
from .protocol import ArduinoProtocol, TestReading

__all__ = ['SerialController', 'ConnectionState', 'ArduinoProtocol', 'TestReading']
