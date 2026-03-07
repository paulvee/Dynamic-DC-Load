"""
Main Window for Battery Tester Application

PyQt6-based GUI matching the original Delphi application layout.
"""

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
    QPushButton, QLabel, QComboBox, QSpinBox, QStatusBar, QGroupBox,
    QMessageBox, QFileDialog, QMenuBar, QMenu, QToolBar, QSizePolicy
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QThread
from PyQt6.QtGui import QAction, QIcon, QColor
import pyqtgraph as pg
from datetime import datetime
from typing import Optional

from hardware import SerialController, ArduinoProtocol, ConnectionState, TestReading
from core import TestState, TestParameters, TestData
from utils import ConfigManager, ExportManager


class ConnectionIndicator(QWidget):
    """LED-style connection status indicator"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.state = "red"
        self.setFixedSize(25, 25)
    
    def set_state(self, state: str):
        """Set LED color: 'red', 'yellow', 'green'"""
        self.state = state
        self.update()
    
    def paintEvent(self, event):
        from PyQt6.QtGui import QPainter, QBrush, QColor
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        colors = {
            'red': QColor(255, 0, 0),
            'yellow': QColor(255, 255, 0),
            'green': QColor(0, 255, 0)
        }
        
        painter.setBrush(QBrush(colors.get(self.state, QColor(128, 128, 128))))
        painter.drawEllipse(2, 2, 21, 21)


class SerialWorker(QThread):
    """Background thread for serial communication"""
    
    data_received = pyqtSignal(object)  # Emits TestReading objects
    message_received = pyqtSignal(str)  # Emits status messages
    error_occurred = pyqtSignal(str)    # Emits error messages
    
    def __init__(self, controller: SerialController, protocol: ArduinoProtocol):
        super().__init__()
        self.controller = controller
        self.protocol = protocol
        self.running = False
    
    def run(self):
        """Read data from serial port in background"""
        self.running = True
        
        while self.running:
            if not self.controller.is_connected():
                break
            
            try:
                packets = self.protocol.read_data()
                
                for packet_type, value in packets:
                    if packet_type == 'reading':
                        self.data_received.emit(value)
                    elif packet_type == 'message':
                        self.message_received.emit(value)
                    elif packet_type in ['voltage', 'current', 'capacity', 'time']:
                        # Handle legacy separate packets if needed
                        pass
                        
            except Exception as e:
                self.error_occurred.emit(str(e))
                break
            
            self.msleep(100)  # Small delay to avoid spinning
    
    def stop(self):
        """Stop the worker thread"""
        self.running = False


class MainWindow(QMainWindow):
    """Main application window"""
    
    VERSION = "v2.0 Alpha (Python/PyQt6)"
    
    def __init__(self):
        super().__init__()
        
        # Application state
        self.controller = SerialController()
        self.protocol: Optional[ArduinoProtocol] = None
        self.config = ConfigManager()
        self.test_data = TestData(parameters=self.config.get_test_parameters())
        self.serial_worker: Optional[SerialWorker] = None
        
        # Initialize UI
        self.init_ui()
        self.load_settings()
        self.update_ui_state()
    
    def init_ui(self):
        """Initialize the user interface"""
        self.setWindowTitle(f"Battery Tester - {self.VERSION}")
        
        # Restore window geometry
        x, y, width, height = self.config.get_window_geometry()
        self.setGeometry(x, y, width, height)
        
        # Create menu bar
        self.create_menu_bar()
        
        # Create toolbar
        self.create_toolbar()
        
        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QVBoxLayout(central_widget)
        
        # Connection panel
        connection_group = self.create_connection_panel()
        main_layout.addWidget(connection_group)
        
        # Chart area
        self.chart_widget = self.create_chart()
        main_layout.addWidget(self.chart_widget, stretch=1)
        
        # Parameters and display panel
        bottom_layout = QHBoxLayout()
        
        # Left side - Test parameters
        params_group = self.create_parameters_panel()
        bottom_layout.addWidget(params_group)
        
        # Right side - Current readings display
        display_group = self.create_display_panel()
        bottom_layout.addWidget(display_group)
        
        main_layout.addLayout(bottom_layout)
        
        # Control buttons
        button_layout = self.create_button_panel()
        main_layout.addLayout(button_layout)
        
        # Status bar
        self.create_status_bar()
    
    def create_menu_bar(self):
        """Create menu bar"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("&File")
        
        new_action = QAction("&New Test...", self)
        new_action.setShortcut("Ctrl+N")
        new_action.triggered.connect(self.new_test)
        file_menu.addAction(new_action)
        
        file_menu.addSeparator()
        
        save_action = QAction("&Save As...", self)
        save_action.setShortcut("Ctrl+S")
        save_action.triggered.connect(self.save_chart)
        file_menu.addAction(save_action)
        
        file_menu.addSeparator()
        
        export_action = QAction("&Export as CSV...", self)
        export_action.triggered.connect(self.export_csv)
        file_menu.addAction(export_action)
        
        file_menu.addSeparator()
        
        print_action = QAction("&Print...", self)
        print_action.setShortcut("Ctrl+P")
        print_action.triggered.connect(self.print_chart)
        file_menu.addAction(print_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("E&xit", self)
        exit_action.setShortcut("Alt+F4")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Edit menu
        edit_menu = menubar.addMenu("&Edit")
        
        copy_action = QAction("&Copy", self)
        copy_action.setShortcut("Ctrl+C")
        copy_action.triggered.connect(self.copy_chart)
        edit_menu.addAction(copy_action)
        
        # Help menu
        help_menu = menubar.addMenu("&Help")
        
        help_action = QAction("&Help...", self)
        help_action.setShortcut("F1")
        help_action.triggered.connect(self.show_help)
        help_menu.addAction(help_action)
        
        help_menu.addSeparator()
        
        about_action = QAction("&About...", self)
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
    
    def create_toolbar(self):
        """Create toolbar with quick action buttons"""
        toolbar = QToolBar("Main Toolbar")
        self.addToolBar(Qt.ToolBarArea.TopToolBarArea, toolbar)
        
        # Note: Icons would normally be added here
        # For now, using text-only buttons
        
        self.new_btn_toolbar = toolbar.addAction("New", self.new_test)
        self.save_btn_toolbar = toolbar.addAction("Save", self.save_chart)
        self.print_btn_toolbar = toolbar.addAction("Print", self.print_chart)
        self.copy_btn_toolbar = toolbar.addAction("Copy", self.copy_chart)
        toolbar.addSeparator()
        self.export_btn_toolbar = toolbar.addAction("Export CSV", self.export_csv)
    
    def create_connection_panel(self) -> QGroupBox:
        """Create connection panel for COM port selection"""
        group = QGroupBox("Connection")
        layout = QHBoxLayout()
        
        layout.addWidget(QLabel("COM Port:"))
        
        self.port_combo = QComboBox()
        self.refresh_ports()
        self.port_combo.currentIndexChanged.connect(self.on_port_changed)
        layout.addWidget(self.port_combo)
        
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        layout.addWidget(self.connect_btn)
        
        self.led_indicator = ConnectionIndicator()
        self.led_indicator.set_state('red')
        layout.addWidget(self.led_indicator)
        
        layout.addStretch()
        
        group.setLayout(layout)
        return group
    
    def create_chart(self) -> pg.PlotWidget:
        """Create chart widget for voltage and current plotting"""
        # Create plot widget
        plot_widget = pg.PlotWidget()
        plot_widget.setBackground('w')
        plot_widget.showGrid(x=True, y=True, alpha=0.3)
        
        # Set labels
        plot_widget.setLabel('left', 'Voltage', units='V', color='blue')
        plot_widget.setLabel('bottom', 'Time', units='minutes')
        plot_widget.setTitle(f"{self.test_data.parameters.capacity_mah} mAh Lithium Battery Discharge Curve")
        
        # Create voltage plot (left axis, blue)
        self.voltage_curve = plot_widget.plot(
            pen=pg.mkPen(color='b', width=2),
            name='Voltage'
        )
        
        # Create second Y-axis for current (right axis, green)
        self.current_viewbox = pg.ViewBox()
        plot_widget.plotItem.scene().addItem(self.current_viewbox)
        plot_widget.plotItem.getAxis('right').linkToView(self.current_viewbox)
        self.current_viewbox.setXLink(plot_widget.plotItem)
        plot_widget.setLabel('right', 'Current', units='mA', color='green')
        plot_widget.showAxis('right')
        
        self.current_curve = pg.PlotDataItem(
            pen=pg.mkPen(color='g', width=2),
            name='Current'
        )
        self.current_viewbox.addItem(self.current_curve)
        
        # Handle view resize
        def update_views():
            self.current_viewbox.setGeometry(plot_widget.plotItem.vb.sceneBoundingRect())
            self.current_viewbox.linkedViewChanged(plot_widget.plotItem.vb, self.current_viewbox.XAxis)
        
        plot_widget.plotItem.vb.sigResized.connect(update_views)
        update_views()
        
        return plot_widget
    
    def create_parameters_panel(self) -> QGroupBox:
        """Create test parameters input panel"""
        group = QGroupBox("Test Parameters")
        layout = QGridLayout()
        
        # Load current (mA)
        layout.addWidget(QLabel("Load current (mA):"), 0, 0)
        self.current_spin = QSpinBox()
        self.current_spin.setRange(5, 1500)
        self.current_spin.setValue(self.test_data.parameters.current_ma)
        self.current_spin.valueChanged.connect(self.on_current_changed)
        layout.addWidget(self.current_spin, 0, 1)
        
        # Cutoff voltage (V)
        layout.addWidget(QLabel("Cutoff voltage (V):"), 1, 0)
        self.voltage_combo = QComboBox()
        self.populate_voltage_combo()
        layout.addWidget(self.voltage_combo, 1, 1)
        
        # Capacity (mAh)
        layout.addWidget(QLabel("Capacity (mAh):"), 2, 0)
        self.capacity_spin = QSpinBox()
        self.capacity_spin.setRange(5, 10000)
        self.capacity_spin.setValue(self.test_data.parameters.capacity_mah)
        self.capacity_spin.valueChanged.connect(self.on_capacity_changed)
        layout.addWidget(self.capacity_spin, 2, 1)
        
        # Estimated time
        layout.addWidget(QLabel("Estimated time:"), 3, 0)
        self.time_label = QLabel("--:--")
        layout.addWidget(self.time_label, 3, 1)
        self.update_estimated_time()
        
        group.setLayout(layout)
        return group
    
    def create_display_panel(self) -> QGroupBox:
        """Create current readings display panel"""
        group = QGroupBox("Current Readings")
        layout = QGridLayout()
        
        # Elapsed time
        layout.addWidget(QLabel("Elapsed Time:"), 0, 0)
        self.elapsed_label = QLabel("--:--:--")
        self.elapsed_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.elapsed_label, 0, 1)
        
        # Voltage
        layout.addWidget(QLabel("Voltage:"), 1, 0)
        self.voltage_label = QLabel("-------- V")
        self.voltage_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.voltage_label, 1, 1)
        
        # Current
        layout.addWidget(QLabel("Current:"), 2, 0)
        self.current_label = QLabel("-------- mA")
        self.current_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.current_label, 2, 1)
        
        # Capacity
        layout.addWidget(QLabel("Capacity:"), 3, 0)
        self.capacity_label = QLabel("-------- mAh")
        self.capacity_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.capacity_label, 3, 1)
        
        group.setLayout(layout)
        return group
    
    def create_button_panel(self) -> QHBoxLayout:
        """Create control button panel"""
        layout = QHBoxLayout()
        
        self.start_btn = QPushButton("Start Test")
        self.start_btn.clicked.connect(self.start_test)
        self.start_btn.setEnabled(False)
        layout.addWidget(self.start_btn)
        
        self.cancel_btn = QPushButton("Cancel")
        self.cancel_btn.clicked.connect(self.cancel_test)
        self.cancel_btn.setEnabled(False)
        layout.addWidget(self.cancel_btn)
        
        self.new_test_btn = QPushButton("New Test")
        self.new_test_btn.clicked.connect(self.new_test)
        self.new_test_btn.setEnabled(False)
        layout.addWidget(self.new_test_btn)
        
        layout.addStretch()
        
        return layout
    
    def create_status_bar(self):
        """Create status bar with multiple panels"""
        self.statusbar = QStatusBar()
        self.setStatusBar(self.statusbar)
        
        self.status_label1 = QLabel("")
        self.status_label2 = QLabel("")
        self.status_label3 = QLabel("Ready")
        
        self.statusbar.addWidget(self.status_label1, stretch=2)
        self.statusbar.addWidget(self.status_label2, stretch=2)
        self.statusbar.addWidget(self.status_label3, stretch=1)
    
    def refresh_ports(self):
        """Refresh list of available COM ports"""
        self.port_combo.clear()
        ports = SerialController.get_available_ports()
        
        for port in ports:
            info = SerialController.get_port_info(port)
            self.port_combo.addItem(f"{port}", port)
        
        # Restore last used port
        last_port_idx = self.config.get_last_port_index()
        if last_port_idx < self.port_combo.count():
            self.port_combo.setCurrentIndex(last_port_idx)
    
    def populate_voltage_combo(self):
        """Populate voltage cutoff combo box"""
        voltages = [
            4.20, 4.15, 4.10, 4.05, 4.00, 3.95, 3.90, 3.85, 3.80, 3.75,
            3.70, 3.65, 3.60, 3.55, 3.50, 3.45, 3.40, 3.35, 3.30, 3.25,
            3.20, 3.15, 3.10, 3.05, 3.00, 2.95, 2.90, 2.85, 2.80, 2.75,
            2.70, 2.65, 2.60, 2.55, 2.50, 0.00
        ]
        
        for v in voltages:
            self.voltage_combo.addItem(f"{v:.2f}")
        
        # Set default to 3.00V (index 24)
        self.voltage_combo.setCurrentIndex(24)
    
    def load_settings(self):
        """Load saved settings"""
        params = self.config.get_test_parameters()
        self.current_spin.setValue(params.current_ma)
        self.capacity_spin.setValue(params.capacity_mah)
        self.config.set_cutoff_voltage(params.cutoff_voltage)
        
        # Find matching voltage in combo
        for i in range(self.voltage_combo.count()):
            if abs(float(self.voltage_combo.itemText(i)) - params.cutoff_voltage) < 0.01:
                self.voltage_combo.setCurrentIndex(i)
                break
    
    def update_estimated_time(self):
        """Update estimated test time display"""
        current = self.current_spin.value()
        capacity = self.capacity_spin.value()
        
        if current > 0:
            time_minutes = int((capacity / current) * 60)
            hours = time_minutes // 60
            mins = time_minutes % 60
            self.time_label.setText(f"{hours:02d}:{mins:02d}")
            
            # Update status bar (if it exists)
            if hasattr(self, 'status_label2'):
                max_time = time_minutes + 30
                max_hours = max_time // 60
                max_mins = max_time % 60
                self.status_label2.setText(
                    f"Maximum test time: {max_hours} hr : {max_mins} min"
                )
    
    def update_ui_state(self):
        """Update UI element states based on current state"""
        is_connected = self.controller.is_connected()
        is_running = self.test_data.state == TestState.RUNNING
        
        self.connect_btn.setEnabled(self.port_combo.count() > 0)
        self.start_btn.setEnabled(is_connected and not is_running)
        self.cancel_btn.setEnabled(is_running)
        self.new_test_btn.setEnabled(is_connected and not is_running)
        
        # Update LED
        if not is_connected:
            self.led_indicator.set_state('red')
            self.connect_btn.setText("Connect")
        elif is_running:
            self.led_indicator.set_state('green')
        else:
            self.led_indicator.set_state('yellow')
            self.connect_btn.setText("Disconnect")
        
        # Enable/disable parameters during test
        self.current_spin.setEnabled(not is_running)
        self.voltage_combo.setEnabled(not is_running)
        self.capacity_spin.setEnabled(not is_running)
    
    # Event handlers
    def on_port_changed(self):
        """Handle COM port selection change"""
        if self.controller.is_connected():
            self.controller.disconnect()
            self.update_ui_state()
    
    def on_current_changed(self):
        """Handle current value change"""
        self.update_estimated_time()
        # Adjust increment based on value
        value = self.current_spin.value()
        if value < 100:
            self.current_spin.setSingleStep(1)
        elif value < 200:
            self.current_spin.setSingleStep(5)
        elif value < 500:
            self.current_spin.setSingleStep(10)
        else:
            self.current_spin.setSingleStep(25)
    
    def on_capacity_changed(self):
        """Handle capacity value change"""
        self.update_estimated_time()
        # Adjust increment
        value = self.capacity_spin.value()
        if value < 100:
            self.capacity_spin.setSingleStep(1)
        elif value < 200:
            self.capacity_spin.setSingleStep(5)
        elif value < 500:
            self.capacity_spin.setSingleStep(10)
        elif value < 1500:
            self.capacity_spin.setSingleStep(25)
        else:
            self.capacity_spin.setSingleStep(50)
    
    def toggle_connection(self):
        """Toggle serial connection"""
        if self.controller.is_connected():
            self.disconnect_serial()
        else:
            self.connect_serial()
    
    def connect_serial(self):
        """Connect to selected serial port"""
        port = self.port_combo.currentData()
        if not port:
            QMessageBox.warning(self, "Connection Error", "No port selected")
            return
        
        if self.controller.connect(port):
            self.protocol = ArduinoProtocol(self.controller.serial)
            self.config.set_last_port_index(self.port_combo.currentIndex())
            self.status_label3.setText(self.controller.get_connection_string())
            self.update_ui_state()
        else:
            QMessageBox.critical(
                self, 
                "Connection Error",
                f"Failed to connect to {port}:\n{self.controller.last_error}"
            )
    
    def disconnect_serial(self):
        """Disconnect from serial port"""
        if self.serial_worker:
            self.serial_worker.stop()
            self.serial_worker.wait()
            self.serial_worker = None
        
        self.controller.disconnect()
        self.status_label3.setText("Disconnected")
        self.update_ui_state()
    
    def start_test(self):
        """Start battery discharge test"""
        # Get parameters
        params = TestParameters(
            current_ma=self.current_spin.value(),
            cutoff_voltage=float(self.voltage_combo.currentText()),
            capacity_mah=self.capacity_spin.value(),
            sample_interval_sec=10,
            proportional_gain=50,
            beep_enabled=False
        )
        
        # Validate
        is_valid, error = params.validate()
        if not is_valid:
            QMessageBox.warning(self, "Invalid Parameters", error)
            return
        
        # Warning for 0V cutoff
        if params.cutoff_voltage == 0.0:
            reply = QMessageBox.warning(
                self,
                "Warning",
                "A value of '0.00V' relies on the battery's internal safety protection to end the test.\n\n" +
                "This could be dangerous. Continue?",
                QMessageBox.StandardButton.Ok | QMessageBox.StandardButton.Cancel
            )
            if reply != QMessageBox.StandardButton.Ok:
                return
        
        # Clear previous test data
        self.test_data = TestData(parameters=params)
        self.test_data.state = TestState.STARTING
        
        # Clear chart
        self.voltage_curve.setData([], [])
        self.current_curve.setData([], [])
        
        # Send parameters to controller
        max_time = params.calculate_max_time_minutes()
        success = self.protocol.send_test_parameters(
            current_ma=params.current_ma,
            cutoff_voltage=params.cutoff_voltage,
            time_limit_minutes=max_time,
            sample_interval_sec=params.sample_interval_sec,
            kp=params.proportional_gain,
            beep_enabled=params.beep_enabled
        )
        
        if not success:
            QMessageBox.critical(self, "Error", "Failed to send test parameters")
            self.test_data.state = TestState.ERROR
            return
        
        # Start serial worker thread
        self.test_data.state = TestState.RUNNING
        self.serial_worker = SerialWorker(self.controller, self.protocol)
        self.serial_worker.data_received.connect(self.on_data_received)
        self.serial_worker.message_received.connect(self.on_message_received)
        self.serial_worker.error_occurred.connect(self.on_error_occurred)
        self.serial_worker.start()
        
        self.status_label1.setText(f"Start Time: {datetime.now().strftime('%a %d %b %Y %H:%M')}")
        self.update_ui_state()
    
    def cancel_test(self):
        """Cancel current test"""
        reply = QMessageBox.question(
            self,
            "Cancel Test",
            "Are you sure you want to cancel the current test?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            if self.protocol:
                self.protocol.cancel_test()
            self.stop_test("User cancelled")
    
    def stop_test(self, reason: str):
        """Stop the current test"""
        if self.serial_worker:
            self.serial_worker.stop()
            self.serial_worker.wait()
            self.serial_worker = None
        
        self.test_data.state = TestState.COMPLETED
        self.test_data.completion_message = reason
        self.status_label1.setText(f"Test completed: {reason}")
        self.update_ui_state()
    
    def new_test(self):
        """Start a new test"""
        if self.test_data.state == TestState.RUNNING:
            QMessageBox.warning(self, "Test Running", "Please stop current test first")
            return
        
        # Ask for confirmation
        from .dialogs import NewTestDialog
        dialog = NewTestDialog(parent=self)
        if dialog.exec():
            # Clear displays
            self.test_data.clear()
            self.elapsed_label.setText("--:--:--")
            self.voltage_label.setText("-------- V")
            self.current_label.setText("-------- mA")
            self.capacity_label.setText("-------- mAh")
            self.voltage_curve.setData([], [])
            self.current_curve.setData([], [])
            self.status_label1.setText("")
            self.update_ui_state()
    
    def on_data_received(self, reading: TestReading):
        """Handle received data from controller"""
        # Add to test data
        self.test_data.add_point(
            voltage=reading.voltage,
            current_ma=reading.current_ma,
            capacity_mah=reading.capacity_mah,
            elapsed_seconds=reading.elapsed_seconds
        )
        
        # Update displays
        self.elapsed_label.setText(reading.elapsed_time)
        self.voltage_label.setText(f"{reading.voltage:.3f} V")
        self.current_label.setText(f"{reading.current_ma:.1f} mA")
        self.capacity_label.setText(f"{reading.capacity_mah:.2f} mAh")
        
        # Update chart
        times, voltages = self.test_data.get_voltage_series()
        _, currents = self.test_data.get_current_series()
        
        self.voltage_curve.setData(times, voltages)
        self.current_curve.setData(times, currents)
    
    def on_message_received(self, message: str):
        """Handle status message from controller"""
        self.status_label3.setText(message)
        
        # Check for test completion
        if any(keyword in message for keyword in ['Finished', 'Exceeded', 'Error', 'Cancelled', 'Cutoff']):
            self.stop_test(message)
    
    def on_error_occurred(self, error: str):
        """Handle serial communication error"""
        QMessageBox.critical(self, "Communication Error", f"Serial error: {error}")
        self.stop_test(f"Error: {error}")
    
    # Menu actions
    def save_chart(self):
        """Save chart as image"""
        if self.test_state == TestState.IDLE or not self.test_data.data_points:
            QMessageBox.warning(self, "No Data", "No test data to save. Run a test first.")
            return
        
        success, error = ExportManager.export_chart_image(
            self.chart, self.test_data, self.test_params, self
        )
        
        if success:
            QMessageBox.information(self, "Save Chart", "Chart saved successfully!")
        elif error:
            QMessageBox.critical(self, "Export Error", error)
    
    def export_csv(self):
        """Export data as CSV"""
        if self.test_state == TestState.IDLE or not self.test_data.data_points:
            QMessageBox.warning(self, "No Data", "No test data to export. Run a test first.")
            return
        
        success, error = ExportManager.export_to_csv(
            self.test_data, self.test_params, self
        )
        
        if success:
            QMessageBox.information(self, "Export CSV", "Data exported successfully!")
        elif error:
            QMessageBox.critical(self, "Export Error", error)
    
    def print_chart(self):
        """Print chart"""
        if self.test_state == TestState.IDLE or not self.test_data.data_points:
            QMessageBox.warning(self, "No Data", "No test data to print. Run a test first.")
            return
        
        success, error = ExportManager.print_chart(
            self.chart, self.test_data, self.test_params, self
        )
        
        if success:
            QMessageBox.information(self, "Print", "Chart sent to printer successfully!")
        elif error:
            QMessageBox.critical(self, "Print Error", error)
    
    def copy_chart(self):
        """Copy chart to clipboard"""
        success, error = ExportManager.copy_chart_to_clipboard(self.chart, self)
        
        if success:
            self.statusBar().showMessage("Chart copied to clipboard", 3000)
        elif error:
            QMessageBox.critical(self, "Copy Error", error)
    
    def show_help(self):
        """Show help dialog"""
        QMessageBox.information(
            self,
            "Help",
            "Battery Tester Help\n\n" +
            "1. Select COM port and click Connect\n" +
            "2. Set test parameters (current, voltage, capacity)\n" +
            "3. Click Start Test\n" +
            "4. Monitor real-time discharge curve\n" +
            "5. Export results when complete"
        )
    
    def show_about(self):
        """Show about dialog"""
        from .dialogs import AboutDialog
        dialog = AboutDialog(self.VERSION, self)
        dialog.exec()
    
    def closeEvent(self, event):
        """Handle window close event"""
        # Save settings
        self.config.save_test_parameters(self.test_data.parameters)
        x, y = self.x(), self.y()
        width, height = self.width(), self.height()
        self.config.save_window_geometry(x, y, width, height)
        
        # Stop test if running
        if self.test_data.state == TestState.RUNNING:
            reply = QMessageBox.question(
                self,
                "Test Running",
                "A test is currently running. Exit anyway?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
            )
            if reply != QMessageBox.StandardButton.Yes:
                event.ignore()
                return
            
            if self.protocol:
                self.protocol.cancel_test()
        
        # Cleanup
        if self.serial_worker:
            self.serial_worker.stop()
            self.serial_worker.wait()
        
        if self.controller.is_connected():
            self.controller.disconnect()
        
        event.accept()
