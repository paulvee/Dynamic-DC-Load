"""
Configuration Management for Battery Tester

Handles saving and loading application settings to/from config.ini file.
Compatible with Delphi version's INI format.
"""

import configparser
import os
from typing import Optional
from core.models import TestParameters


class ConfigManager:
    """
    Manages application configuration persistence.
    
    Maintains compatibility with original Delphi app's BatteryTester.ini format.
    """
    
    SECTION = 'BattTest'
    
    def __init__(self, config_path: str = 'config.ini'):
        self.config_path = config_path
        self.config = configparser.ConfigParser()
        self.load()
    
    def load(self) -> bool:
        """
        Load configuration from file.
        
        Returns True if loaded successfully, False if file doesn't exist.
        """
        if os.path.exists(self.config_path):
            try:
                self.config.read(self.config_path)
                return True
            except Exception as e:
                print(f"Error loading config: {e}")
                self.create_defaults()
                return False
        else:
            self.create_defaults()
            return False
    
    def create_defaults(self):
        """Create default configuration"""
        self.config[self.SECTION] = {
            'Cutoff': '5',                    # Cutoff voltage index (matches Delphi combo box)
            'TimeLimit': '180',               # Capacity in mAh (not time!)
            'SampleTime': '10',               # Sample interval in seconds
            'LoopDelay': '0',                 # Loop delay (not used)
            'Tolerance': '1',                 # Current tolerance (not used)
            'Port': '0',                      # COM port index
            'Reset': 'False',                 # Reset controller on new test
            'Beep': 'False',                  # Beep notification
            'MaxCurrent': '2500',             # Maximum discharge current (mA)
            'LastCurrent': '50',              # Last used current setting
            'LastCapacity': '100',            # Last used capacity
            'WindowX': '100',                 # Window position
            'WindowY': '100',
            'WindowWidth': '1200',            # Window size
            'WindowHeight': '800'
        }
        self.save()
    
    def save(self) -> bool:
        """
        Save configuration to file.
        
        Returns True if saved successfully.
        """
        try:
            with open(self.config_path, 'w') as f:
                self.config.write(f)
            return True
        except Exception as e:
            print(f"Error saving config: {e}")
            return False
    
    def get_test_parameters(self) -> TestParameters:
        """
        Load test parameters from config.
        
        Returns TestParameters object with saved values.
        """
        return TestParameters(
            current_ma=self.get_int('LastCurrent', 50),
            cutoff_voltage=self.get_cutoff_voltage(),
            capacity_mah=self.get_int('TimeLimit', 100),
            sample_interval_sec=self.get_int('SampleTime', 10),
            loop_delay_ms=self.get_int('LoopDelay', 0),
            tolerance_percent=self.get_int('Tolerance', 1),
            beep_enabled=self.get_bool('Beep', False),
            battery_weight=self.get_battery_weight(),
            chart_title=self.get_chart_title()
        )
    
    def save_test_parameters(self, params: TestParameters):
        """Save test parameters to config"""
        self.set_value('LastCurrent', params.current_ma)
        self.set_value('TimeLimit', params.capacity_mah)
        self.set_value('SampleTime', params.sample_interval_sec)
        self.set_value('LoopDelay', params.loop_delay_ms)
        self.set_value('Tolerance', params.tolerance_percent)
        self.set_value('Beep', params.beep_enabled)
        self.set_battery_weight(params.battery_weight)
        self.set_chart_title(params.chart_title)
        self.save()
    
    def get_cutoff_voltage(self) -> float:
        """
        Get cutoff voltage from index.
        
        Maps Delphi's combo box index to voltage value.
        """
        voltage_map = {
            0: 4.20, 1: 4.15, 2: 4.10, 3: 4.05, 4: 4.00,
            5: 3.95, 6: 3.90, 7: 3.85, 8: 3.80, 9: 3.75,
            10: 3.70, 11: 3.65, 12: 3.60, 13: 3.55, 14: 3.50,
            15: 3.45, 16: 3.40, 17: 3.35, 18: 3.30, 19: 3.25,
            20: 3.20, 21: 3.15, 22: 3.10, 23: 3.05, 24: 3.00,
            25: 2.95, 26: 2.90, 27: 2.85, 28: 2.80, 29: 2.75,
            30: 2.70, 31: 2.65, 32: 2.60, 33: 2.55, 34: 2.50,
            35: 0.00
        }
        index = self.get_int('Cutoff', 5)
        return voltage_map.get(index, 3.0)
    
    def set_cutoff_voltage(self, voltage: float):
        """Set cutoff voltage by finding closest match"""
        voltage_map = {
            0: 4.20, 1: 4.15, 2: 4.10, 3: 4.05, 4: 4.00,
            5: 3.95, 6: 3.90, 7: 3.85, 8: 3.80, 9: 3.75,
            10: 3.70, 11: 3.65, 12: 3.60, 13: 3.55, 14: 3.50,
            15: 3.45, 16: 3.40, 17: 3.35, 18: 3.30, 19: 3.25,
            20: 3.20, 21: 3.15, 22: 3.10, 23: 3.05, 24: 3.00,
            25: 2.95, 26: 2.90, 27: 2.85, 28: 2.80, 29: 2.75,
            30: 2.70, 31: 2.65, 32: 2.60, 33: 2.55, 34: 2.50,
            35: 0.00
        }
        # Find closest match
        closest_index = min(voltage_map.keys(), 
                          key=lambda k: abs(voltage_map[k] - voltage))
        self.set_value('Cutoff', closest_index)
        self.save()
    
    def get_int(self, key: str, default: int = 0) -> int:
        """Get integer value from config"""
        try:
            return self.config.getint(self.SECTION, key, fallback=default)
        except ValueError:
            return default
    
    def get_float(self, key: str, default: float = 0.0) -> float:
        """Get float value from config"""
        try:
            return self.config.getfloat(self.SECTION, key, fallback=default)
        except ValueError:
            return default
    
    def get_bool(self, key: str, default: bool = False) -> bool:
        """Get boolean value from config"""
        try:
            return self.config.getboolean(self.SECTION, key, fallback=default)
        except ValueError:
            return default
    
    def get_string(self, key: str, default: str = '') -> str:
        """Get string value from config"""
        return self.config.get(self.SECTION, key, fallback=default)
    
    def set_value(self, key: str, value):
        """Set configuration value"""
        if self.SECTION not in self.config:
            self.config[self.SECTION] = {}
        self.config[self.SECTION][key] = str(value)
    
    def get_last_port_index(self) -> int:
        """Get last used COM port index"""
        return self.get_int('Port', 0)
    
    def set_last_port_index(self, index: int):
        """Save last used COM port index"""
        self.set_value('Port', index)
        self.save()
    
    def get_reset_controller(self) -> bool:
        """Get reset controller setting"""
        return self.get_bool('Reset', False)
    
    def set_reset_controller(self, enabled: bool):
        """Set reset controller setting"""
        self.set_value('Reset', enabled)
        self.save()
    
    def get_window_geometry(self) -> tuple[int, int, int, int]:
        """Get window position and size"""
        x = self.get_int('WindowX', 100)
        y = self.get_int('WindowY', 100)
        width = self.get_int('WindowWidth', 1200)
        height = self.get_int('WindowHeight', 800)
        
        # Ensure reasonable minimum size
        if width < 800:
            width = 800
        if height < 600:
            height = 600
        
        # Validate window position - ensure it's visible on screen
        try:
            from PyQt6.QtGui import QGuiApplication
            from PyQt6.QtCore import QRect
            
            # Get available screen geometry
            screens = QGuiApplication.screens()
            if screens:
                # Check if window would be visible on any screen
                window_rect = QRect(x, y, width, height)
                visible_on_screen = False
                
                for screen in screens:
                    screen_geom = screen.geometry()
                    # Window is visible if at least 100x100 pixels are on screen
                    if window_rect.intersects(screen_geom):
                        intersection = window_rect.intersected(screen_geom)
                        if intersection.width() >= 100 and intersection.height() >= 100:
                            visible_on_screen = True
                            break
                
                # If not visible on any screen, center on primary screen
                if not visible_on_screen:
                    primary_screen = QGuiApplication.primaryScreen()
                    if primary_screen:
                        screen_geom = primary_screen.availableGeometry()
                        x = screen_geom.x() + (screen_geom.width() - width) // 2
                        y = screen_geom.y() + (screen_geom.height() - height) // 2
                        # Ensure we're within bounds
                        x = max(screen_geom.x(), min(x, screen_geom.x() + screen_geom.width() - width))
                        y = max(screen_geom.y(), min(y, screen_geom.y() + screen_geom.height() - height))
        except Exception:
            # Fallback: just ensure no negative values
            if x < 0:
                x = 100
            if y < 0:
                y = 100
            
        return x, y, width, height
    
    def save_window_geometry(self, x: int, y: int, width: int, height: int):
        """Save window position and size"""
        # Validate position before saving
        if y < 0:
            y = 0
        if x < 0:
            x = 0
        # Ensure reasonable minimum size
        if width < 800:
            width = 800
        if height < 600:
            height = 600
            
        self.set_value('WindowX', x)
        self.set_value('WindowY', y)
        self.set_value('WindowWidth', width)
        self.set_value('WindowHeight', height)
        self.save()
    
    def get_max_current(self) -> int:
        """Get maximum discharge current limit (mA)"""
        return self.get_int('MaxCurrent', 2500)
    
    def set_max_current(self, current_ma: int):
        """Save maximum discharge current limit (mA)"""
        self.set_value('MaxCurrent', current_ma)
        self.save()
    
    def get_beep_enabled(self) -> bool:
        """Get beep notification setting"""
        return self.get_bool('Beep', False)
    
    def set_beep_enabled(self, enabled: bool):
        """Save beep notification setting"""
        self.set_value('Beep', enabled)
        self.save()
    
    def get_battery_weight(self) -> int:
        """Get battery weight in grams (0 = not tested)"""
        return self.get_int('BatteryWeight', 0)
    
    def set_battery_weight(self, weight_grams: int):
        """Save battery weight in grams"""
        self.set_value('BatteryWeight', weight_grams)
        self.save()
    
    def get_chart_title(self) -> str:
        """Get chart title for saved images"""
        return self.get_string('ChartTitle', '')
    
    def set_chart_title(self, title: str):
        """Save chart title for saved images"""
        self.set_value('ChartTitle', title)
        self.save()
