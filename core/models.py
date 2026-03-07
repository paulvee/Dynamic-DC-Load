"""
Data Models for Battery Tester Application
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Optional
from enum import Enum


class TestState(Enum):
    """Test execution states"""
    IDLE = "idle"
    STARTING = "starting"
    RUNNING = "running"
    PAUSED = "paused"
    COMPLETED = "completed"
    CANCELLED = "cancelled"
    ERROR = "error"


@dataclass
class TestParameters:
    """Test configuration parameters"""
    current_ma: int = 50  # Discharge current in mA
    cutoff_voltage: float = 3.0  # Minimum voltage to end test
    capacity_mah: int = 100  # Rated battery capacity
    sample_interval_sec: int = 10  # Data sample interval
    proportional_gain: int = 50  # PID kP value
    loop_delay_ms: int = 0  # Loop delay (not used in v3.04)
    tolerance_percent: int = 1  # Current tolerance (not used in v3.04)
    beep_enabled: bool = False  # Beep on completion
    
    def calculate_max_time_minutes(self) -> int:
        """
        Calculate maximum test time based on capacity and current.
        
        Formula: (capacity / current) * 60 + 30 minutes safety margin
        """
        if self.current_ma == 0:
            return 180  # Default fallback
        
        base_time = (self.capacity_mah / self.current_ma) * 60
        return int(base_time + 30)
    
    def validate(self) -> tuple[bool, Optional[str]]:
        """
        Validate test parameters.
        
        Returns:
            (is_valid, error_message)
        """
        if self.current_ma < 5 or self.current_ma > 1500:
            return False, "Current must be between 5 and 1500 mA"
        
        if self.cutoff_voltage < 0 or self.cutoff_voltage > 20:
            return False, "Cutoff voltage must be between 0 and 20 V"
        
        if self.capacity_mah < 5 or self.capacity_mah > 10000:
            return False, "Capacity must be between 5 and 10000 mAh"
        
        if self.sample_interval_sec < 1 or self.sample_interval_sec > 300:
            return False, "Sample interval must be between 1 and 300 seconds"
        
        return True, None


@dataclass
class DataPoint:
    """Single data point from test"""
    timestamp: datetime
    elapsed_seconds: int
    voltage: float
    current_ma: float
    capacity_mah: float
    
    def __post_init__(self):
        """Ensure timestamp is a datetime object"""
        if isinstance(self.timestamp, str):
            self.timestamp = datetime.fromisoformat(self.timestamp)
    
    @property
    def elapsed_minutes(self) -> float:
        """Get elapsed time in minutes"""
        return self.elapsed_seconds / 60.0
    
    @property
    def elapsed_time_str(self) -> str:
        """Format elapsed time as HH:MM:SS"""
        hours = self.elapsed_seconds // 3600
        minutes = (self.elapsed_seconds % 3600) // 60
        seconds = self.elapsed_seconds % 60
        return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


@dataclass
class TestData:
    """Complete test data collection"""
    parameters: TestParameters
    data_points: List[DataPoint] = field(default_factory=list)
    start_time: Optional[datetime] = None
    end_time: Optional[datetime] = None
    state: TestState = TestState.IDLE
    initial_voltage: Optional[float] = None
    final_voltage: Optional[float] = None
    completion_message: Optional[str] = None
    
    def add_point(self, voltage: float, current_ma: float, 
                  capacity_mah: float, elapsed_seconds: int) -> DataPoint:
        """
        Add a new data point to the collection.
        
        Args:
            voltage: Battery voltage in volts
            current_ma: Discharge current in mA
            capacity_mah: Accumulated capacity in mAh
            elapsed_seconds: Time elapsed since test start
        
        Returns:
            The created DataPoint
        """
        if self.start_time is None:
            self.start_time = datetime.now()
            self.initial_voltage = voltage
        
        point = DataPoint(
            timestamp=datetime.now(),
            elapsed_seconds=elapsed_seconds,
            voltage=voltage,
            current_ma=current_ma,
            capacity_mah=capacity_mah
        )
        
        self.data_points.append(point)
        return point
    
    def get_latest_point(self) -> Optional[DataPoint]:
        """Get the most recent data point"""
        return self.data_points[-1] if self.data_points else None
    
    def get_voltage_series(self) -> tuple[List[float], List[float]]:
        """
        Get voltage data series for plotting.
        
        Returns:
            (time_minutes, voltage_values)
        """
        if not self.data_points:
            return [], []
        
        times = [p.elapsed_minutes for p in self.data_points]
        voltages = [p.voltage for p in self.data_points]
        return times, voltages
    
    def get_current_series(self) -> tuple[List[float], List[float]]:
        """
        Get current data series for plotting.
        
        Returns:
            (time_minutes, current_values)
        """
        if not self.data_points:
            return [], []
        
        times = [p.elapsed_minutes for p in self.data_points]
        currents = [p.current_ma for p in self.data_points]
        return times, currents
    
    def clear(self):
        """Clear all test data"""
        self.data_points.clear()
        self.start_time = None
        self.end_time = None
        self.state = TestState.IDLE
        self.initial_voltage = None
        self.final_voltage = None
        self.completion_message = None
    
    @property
    def duration_seconds(self) -> int:
        """Get total test duration in seconds"""
        if not self.data_points:
            return 0
        return self.data_points[-1].elapsed_seconds
    
    @property
    def duration_str(self) -> str:
        """Get formatted duration string"""
        if not self.data_points:
            return "00:00:00"
        return self.data_points[-1].elapsed_time_str
    
    @property
    def final_capacity_mah(self) -> float:
        """Get final measured capacity"""
        if not self.data_points:
            return 0.0
        return self.data_points[-1].capacity_mah
    
    @property
    def capacity_percent(self) -> float:
        """Get measured capacity as percentage of rated capacity"""
        if self.parameters.capacity_mah == 0:
            return 0.0
        return (self.final_capacity_mah / self.parameters.capacity_mah) * 100.0
    
    def export_summary(self) -> dict:
        """
        Export test summary as dictionary.
        
        Useful for reports and data export.
        """
        return {
            'test_date': self.start_time.isoformat() if self.start_time else None,
            'duration': self.duration_str,
            'rated_capacity_mah': self.parameters.capacity_mah,
            'tested_capacity_mah': self.final_capacity_mah,
            'capacity_percent': f"{self.capacity_percent:.1f}%",
            'initial_voltage': f"{self.initial_voltage:.3f}V" if self.initial_voltage else None,
            'final_voltage': f"{self.final_voltage:.3f}V" if self.final_voltage else None,
            'cutoff_voltage': f"{self.parameters.cutoff_voltage:.2f}V",
            'test_current': f"{self.parameters.current_ma}mA",
            'data_points': len(self.data_points),
            'completion_message': self.completion_message,
            'state': self.state.value
        }
