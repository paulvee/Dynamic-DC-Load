"""
Serial Port Management for Battery Tester

Handles serial port connection, disconnection, and error handling.
"""

import serial
import serial.tools.list_ports
from typing import Optional, List
from enum import Enum


class ConnectionState(Enum):
    """Serial connection states"""
    DISCONNECTED = "disconnected"
    CONNECTING = "connecting"
    CONNECTED = "connected"
    ERROR = "error"


class SerialController:
    """
    Manages serial port connection to Arduino battery tester controller.
    """
    
    def __init__(self, port: Optional[str] = None, baud_rate: int = 9600):
        self.port = port
        self.baud_rate = baud_rate
        self.serial: Optional[serial.Serial] = None
        self.state = ConnectionState.DISCONNECTED
        self.last_error: Optional[str] = None
    
    @staticmethod
    def get_available_ports() -> List[str]:
        """
        Get list of available serial ports.
        
        Returns list of port names (e.g., ['COM1', 'COM3'] on Windows,
        ['/dev/ttyUSB0', '/dev/ttyACM0'] on Linux).
        """
        ports = serial.tools.list_ports.comports()
        return [port.device for port in sorted(ports)]
    
    @staticmethod
    def get_port_info(port_name: str) -> Optional[str]:
        """
        Get detailed information about a serial port.
        
        Returns description string or None if port not found.
        """
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if port.device == port_name:
                return f"{port.device}: {port.description}"
        return None
    
    def connect(self, port: Optional[str] = None) -> bool:
        """
        Connect to serial port.
        
        Args:
            port: Port name (e.g., 'COM3'). If None, uses previously set port.
        
        Returns:
            True if connection successful, False otherwise.
        """
        if port:
            self.port = port
        
        if not self.port:
            self.last_error = "No port specified"
            self.state = ConnectionState.ERROR
            return False
        
        try:
            self.state = ConnectionState.CONNECTING
            
            # Close existing connection if any
            if self.serial and self.serial.is_open:
                self.serial.close()
            
            # Open new connection
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baud_rate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1,
                write_timeout=1
            )
            
            if self.serial.is_open:
                self.state = ConnectionState.CONNECTED
                self.last_error = None
                return True
            else:
                self.state = ConnectionState.ERROR
                self.last_error = "Failed to open port"
                return False
                
        except serial.SerialException as e:
            self.state = ConnectionState.ERROR
            self.last_error = str(e)
            self.serial = None
            return False
    
    def disconnect(self) -> bool:
        """
        Disconnect from serial port.
        
        Returns:
            True if disconnection successful, False otherwise.
        """
        try:
            if self.serial and self.serial.is_open:
                self.serial.close()
            
            self.state = ConnectionState.DISCONNECTED
            self.last_error = None
            return True
            
        except serial.SerialException as e:
            self.last_error = str(e)
            self.state = ConnectionState.ERROR
            return False
    
    def is_connected(self) -> bool:
        """Check if currently connected to serial port"""
        return (
            self.serial is not None and 
            self.serial.is_open and 
            self.state == ConnectionState.CONNECTED
        )
    
    def read(self, size: int = 1) -> bytes:
        """
        Read bytes from serial port.
        
        Args:
            size: Number of bytes to read
        
        Returns:
            Bytes read, or empty bytes if error
        """
        if not self.is_connected():
            return b''
        
        try:
            return self.serial.read(size)
        except serial.SerialException as e:
            self.last_error = str(e)
            self.state = ConnectionState.ERROR
            return b''
    
    def write(self, data: bytes) -> bool:
        """
        Write bytes to serial port.
        
        Args:
            data: Bytes to write
        
        Returns:
            True if write successful, False otherwise
        """
        if not self.is_connected():
            self.last_error = "Not connected"
            return False
        
        try:
            self.serial.write(data)
            return True
        except serial.SerialException as e:
            self.last_error = str(e)
            self.state = ConnectionState.ERROR
            return False
    
    def in_waiting(self) -> int:
        """
        Get number of bytes waiting in input buffer.
        
        Returns:
            Number of bytes available, or 0 if error
        """
        if not self.is_connected():
            return 0
        
        try:
            return self.serial.in_waiting
        except serial.SerialException:
            return 0
    
    def flush_input(self):
        """Flush input buffer"""
        if self.is_connected():
            try:
                self.serial.reset_input_buffer()
            except serial.SerialException:
                pass
    
    def flush_output(self):
        """Flush output buffer"""
        if self.is_connected():
            try:
                self.serial.reset_output_buffer()
            except serial.SerialException:
                pass
    
    def get_connection_string(self) -> str:
        """
        Get connection string for display.
        
        Returns:
            String like "COM3: 9600, 8, N, 1"
        """
        if not self.port:
            return "Not connected"
        
        return f"{self.port}: {self.baud_rate}, 8, N, 1"
    
    def __del__(self):
        """Cleanup: close serial port on deletion"""
        if self.serial and self.serial.is_open:
            self.serial.close()
