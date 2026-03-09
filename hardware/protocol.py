"""
Arduino Battery Tester Serial Protocol Implementation

This module handles communication with the Arduino/ESP32 battery tester controller.

Protocol Details:
-----------------
**Setup (PC → Arduino):**
Sends 9 newline-terminated parameters:
1. target_mA (int)
2. cutoff_voltage (float)  
3. time_limit (int) - in minutes
4. sampleTime (int) - in seconds
5. kP (int) - proportional gain
6. offset (float) - not used in v3.04
7. tolerance (int) - not used in v3.04
8. beep (int) - not used in v3.04
9. cancel (int) - 0 to start, 999 to cancel

**Data Stream (Arduino → PC):**

v3.04 Format (Combined packet):
GRAPHVS[days]:[HH]:[MM]:[SS]![mAh]![current_mA]![voltage]GRAPHVEND

Example: GRAPHVS0:01:23:45!42.3!50!3.456GRAPHVEND

Legacy Format (Separate packets - for older firmware):
- Time: STARTT[HH:MM:SS]ENDT
- Voltage: GRAPHVS[float]GRAPHVEND  
- Current: GRAPHCS[float]GRAPHCEND
- Capacity: STARTMAH[float]ENDMAH
- Message: MSGST[string]MSGEND

**Control Commands (PC → Arduino):**
- Cancel: "999\\n"
- Continue: "0\\n"
"""

import serial
import time
import re
from typing import Optional, Tuple, List, Callable
from dataclasses import dataclass


@dataclass
class TestReading:
    """Single test data reading from Arduino"""
    elapsed_time: str  # Format: "D:HH:MM:SS" or "HH:MM:SS"
    capacity_mah: float
    current_ma: float
    voltage: float
    
    @property
    def elapsed_seconds(self) -> int:
        """Convert elapsed time to total seconds"""
        parts = self.elapsed_time.split(':')
        if len(parts) == 4:  # D:HH:MM:SS
            days, hours, mins, secs = map(int, parts)
            return days * 86400 + hours * 3600 + mins * 60 + secs
        elif len(parts) == 3:  # HH:MM:SS
            hours, mins, secs = map(int, parts)
            return hours * 3600 + mins * 60 + secs
        return 0


class ArduinoProtocol:
    """
    Arduino Battery Tester Protocol Handler
    
    Supports both v3.04 combined packet format and legacy separate packet format.
    """
    
    # Packet delimiters
    VOLTAGE_START = b'GRAPHVS'
    VOLTAGE_END = b'GRAPHVEND'
    
    CURRENT_START = b'GRAPHCS'
    CURRENT_END = b'GRAPHCEND'
    
    TIME_START = b'STARTT'
    TIME_END = b'ENDT'
    
    MAH_START = b'STARTMAH'
    MAH_END = b'ENDMAH'
    
    MSG_START = b'MSGST'
    MSG_END = b'MSGEND'
    
    def __init__(self, serial_port: serial.Serial):
        self.serial = serial_port
        self.buffer = b''
        self.last_reading: Optional[TestReading] = None
        
    def parse_combined_packet(self, data: str) -> Optional[TestReading]:
        """
        Parse v3.04 combined data packet format.
        
        Format: GRAPHVS[days]:[HH]:[MM]:[SS]![mAh]![current_mA]![voltage]GRAPHVEND
        Example: GRAPHVS0:01:23:45!42.3!50!3.456GRAPHVEND
        """
        try:
            # Split by '!' delimiter
            parts = data.split('!')
            if len(parts) >= 4:
                time_str = parts[0]  # "0:01:23:45"
                mah = float(parts[1])
                current = float(parts[2])
                voltage = float(parts[3])
                
                return TestReading(
                    elapsed_time=time_str,
                    capacity_mah=mah,
                    current_ma=current,
                    voltage=voltage
                )
        except (ValueError, IndexError) as e:
            print(f"Error parsing combined packet: {e}")
        return None
    
    def parse_buffer(self) -> List[Tuple[str, any]]:
        """
        Parse incoming data buffer and extract packets.
        
        Returns list of (packet_type, value) tuples.
        Supports both combined v3.04 format and legacy separate packets.
        """
        packets = []
        
        # Check for combined v3.04 format packet (most common)
        if self.VOLTAGE_START in self.buffer and self.VOLTAGE_END in self.buffer:
            start_idx = self.buffer.find(self.VOLTAGE_START) + len(self.VOLTAGE_START)
            end_idx = self.buffer.find(self.VOLTAGE_END)
            
            if end_idx > start_idx:
                raw_data = self.buffer[start_idx:end_idx].decode('ascii', errors='ignore')
                
                # Check if it's combined format (contains '!')
                if '!' in raw_data:
                    reading = self.parse_combined_packet(raw_data)
                    if reading:
                        self.last_reading = reading
                        packets.append(('reading', reading))
                else:
                    # Legacy format - just voltage
                    try:
                        voltage = float(raw_data)
                        packets.append(('voltage', voltage))
                    except ValueError:
                        pass
                
                # Remove processed data from buffer
                self.buffer = self.buffer[end_idx + len(self.VOLTAGE_END):]
        
        # Check for legacy separate packets
        if self.TIME_START in self.buffer and self.TIME_END in self.buffer:
            start_idx = self.buffer.find(self.TIME_START) + len(self.TIME_START)
            end_idx = self.buffer.find(self.TIME_END)
            if end_idx > start_idx:
                value = self.buffer[start_idx:end_idx].decode('ascii', errors='ignore')
                packets.append(('time', value))
                self.buffer = self.buffer[end_idx + len(self.TIME_END):]
        
        if self.CURRENT_START in self.buffer and self.CURRENT_END in self.buffer:
            start_idx = self.buffer.find(self.CURRENT_START) + len(self.CURRENT_START)
            end_idx = self.buffer.find(self.CURRENT_END)
            if end_idx > start_idx:
                try:
                    value = float(self.buffer[start_idx:end_idx].decode('ascii'))
                    packets.append(('current', value))
                except ValueError:
                    pass
                self.buffer = self.buffer[end_idx + len(self.CURRENT_END):]
        
        if self.MAH_START in self.buffer and self.MAH_END in self.buffer:
            start_idx = self.buffer.find(self.MAH_START) + len(self.MAH_START)
            end_idx = self.buffer.find(self.MAH_END)
            if end_idx > start_idx:
                try:
                    value = float(self.buffer[start_idx:end_idx].decode('ascii'))
                    packets.append(('capacity', value))
                except ValueError:
                    pass
                self.buffer = self.buffer[end_idx + len(self.MAH_END):]
        
        # Check for message packets (test completion, errors)
        if self.MSG_START in self.buffer and self.MSG_END in self.buffer:
            start_idx = self.buffer.find(self.MSG_START) + len(self.MSG_START)
            end_idx = self.buffer.find(self.MSG_END)
            if end_idx > start_idx:
                value = self.buffer[start_idx:end_idx].decode('ascii', errors='ignore')
                packets.append(('message', value))
                self.buffer = self.buffer[end_idx + len(self.MSG_END):]
        
        return packets
    
    def read_data(self) -> List[Tuple[str, any]]:
        """
        Read available data from serial port and parse packets.
        
        Returns list of (packet_type, value) tuples.
        """
        if self.serial.in_waiting > 0:
            data = self.serial.read(self.serial.in_waiting)
            self.buffer += data
            return self.parse_buffer()
        return []
    
    def send_test_parameters(
        self,
        current_ma: int,
        cutoff_voltage: float,
        time_limit_minutes: int,
        sample_interval_sec: int,
        loop_delay: int = 0,
        tolerance: int = 1,
        beep_enabled: bool = False
    ) -> bool:
        """
        Send test parameters to Arduino.
        
        Parameters are sent as newline-terminated values in sequence:
        1. current_ma
        2. cutoff_voltage  
        3. time_limit (minutes)
        4. sample_interval (seconds)
        5. loop_delay (offset - not used in v3.04)
        6. tolerance (not used in v3.04)
        7. beep (not used in v3.04)
        8. cancel flag (0 = start test)
        
        Returns True if successful, False otherwise.
        """
        try:
            self.serial.write(f"{current_ma}\n".encode())
            time.sleep(0.15)
            
            self.serial.write(f"{cutoff_voltage:.2f}\n".encode())
            time.sleep(0.15)
            
            self.serial.write(f"{time_limit_minutes}\n".encode())
            time.sleep(0.15)
            
            self.serial.write(f"{sample_interval_sec}\n".encode())
            time.sleep(0.15)
            
            self.serial.write(f"{loop_delay}\n".encode())
            time.sleep(0.15)
            
            self.serial.write(f"{tolerance}\n".encode())
            time.sleep(0.15)
            
            beep_flag = '1' if beep_enabled else '0'
            self.serial.write(f"{beep_flag}\n".encode())
            time.sleep(0.15)
            
            # Start test (0 = start, 999 = cancel)
            self.serial.write("0\n".encode())
            
            return True
        except serial.SerialException as e:
            print(f"Error sending parameters: {e}")
            return False
    
    def cancel_test(self) -> bool:
        """
        Send cancel command to Arduino (999).
        
        Returns True if successful, False otherwise.
        """
        try:
            self.serial.write("999\n".encode())
            return True
        except serial.SerialException as e:
            print(f"Error sending cancel: {e}")
            return False
    
    def reset_controller(self) -> bool:
        """
        Reset Arduino controller via DTR signal.
        
        Returns True if successful, False otherwise.
        """
        try:
            self.serial.dtr = True
            time.sleep(0.5)
            self.serial.dtr = False
            time.sleep(2)  # Wait for Arduino to boot
            return True
        except serial.SerialException as e:
            print(f"Error resetting controller: {e}")
            return False
    
    def clear_buffer(self):
        """Clear internal parsing buffer"""
        self.buffer = b''
        if self.serial.in_waiting > 0:
            self.serial.read(self.serial.in_waiting)
