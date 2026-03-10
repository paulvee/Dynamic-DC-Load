"""
Main Window for Battery Tester Application

PyQt6-based GUI matching the original Delphi application layout.
"""

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
    QPushButton, QLabel, QComboBox, QSpinBox, QDoubleSpinBox, QStackedWidget,
    QStatusBar, QGroupBox, QMessageBox, QFileDialog, QMenuBar, QMenu,
    QToolBar, QSizePolicy, QTabWidget, QTextEdit, QStyledItemDelegate, QApplication, QCheckBox
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QThread, QSize
from PyQt6.QtGui import QAction, QIcon, QColor
import pyqtgraph as pg
from datetime import datetime
from typing import Optional

from hardware import SerialController, ArduinoProtocol, ConnectionState, TestReading
from core import TestState, TestParameters, TestData
from utils import ConfigManager, ExportManager


class CompactDelegate(QStyledItemDelegate):
    """Custom item delegate for compact dropdown spacing"""
    def sizeHint(self, option, index):
        size = super().sizeHint(option, index)
        size.setHeight(22)  # Compact height for items
        return size


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
    
    VERSION = "v2.0.e (Python/PyQt6)"
    
    def __init__(self):
        super().__init__()
        
        # Application state
        self.controller = SerialController()
        self.protocol: Optional[ArduinoProtocol] = None
        self.config = ConfigManager()
        self.test_data = TestData(parameters=self.config.get_test_parameters())
        self.serial_worker: Optional[SerialWorker] = None
        self.last_port_list = []  # Track port changes
        
        # Initialize UI
        self.init_ui()
        self.load_settings()
        self.update_ui_state()
        
        # Start port monitoring timer (check every 2 seconds)
        self.port_monitor_timer = QTimer()
        self.port_monitor_timer.timeout.connect(self.check_port_changes)
        self.port_monitor_timer.start(2000)  # 2 second interval
    
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
        
        # Connection panel at top
        connection_group = self.create_connection_panel()
        main_layout.addWidget(connection_group)
        
        # Create tab widget
        self.tab_widget = QTabWidget()
        
        # Graph tab
        graph_tab = self.create_graph_tab()
        self.tab_widget.addTab(graph_tab, "Graph")
        
        # Setup tab
        control_tab = self.create_control_tab()
        self.tab_widget.addTab(control_tab, "Setup")
        
        main_layout.addWidget(self.tab_widget)
        self.tab_widget.currentChanged.connect(self.on_tab_changed)

        # Status bar
        self.create_status_bar()
    
    def create_menu_bar(self):
        """Create menu bar"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("&File")
        
        new_action = QAction("&New Test", self)
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
        self.port_combo.setMinimumWidth(450)  # Wide enough for port descriptions
        self.port_combo.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToContents)
        self.refresh_ports()
        # Reduce dropdown item spacing with custom delegate
        self.port_combo.setItemDelegate(CompactDelegate(self.port_combo))
        self.port_combo.currentIndexChanged.connect(self.on_port_changed)
        layout.addWidget(self.port_combo)
        
        # Refresh button
        self.refresh_btn = QPushButton("Refresh")
        self.refresh_btn.clicked.connect(self.refresh_ports)
        self.refresh_btn.setToolTip("Refresh COM port list")
        layout.addWidget(self.refresh_btn)
        
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
        plot_widget.setLabel('left', 'Voltage', units='V')
        plot_widget.setLabel('bottom', 'Time', units='seconds')
        plot_widget.setTitle(f"{self.test_data.parameters.capacity_mah} mAh Lithium Battery Discharge Curve")
        
        # Create voltage plot (left axis, green)
        self.voltage_curve = plot_widget.plot(
            pen=pg.mkPen(color='g', width=2),
            name='Voltage'
        )
        
        # Create second Y-axis for current (right axis, green)
        self.current_viewbox = pg.ViewBox()
        plot_widget.plotItem.scene().addItem(self.current_viewbox)
        plot_widget.plotItem.getAxis('right').linkToView(self.current_viewbox)
        self.current_viewbox.setXLink(plot_widget.plotItem)
        self.current_viewbox.invertY(False)  # Ensure Y-axis is not inverted
        self.current_viewbox.enableAutoRange(axis=pg.ViewBox.YAxis, enable=True)  # Enable Y-axis auto-scaling
        plot_widget.setLabel('right', 'Current', units='mA')
        plot_widget.showAxis('right')
        
        self.current_curve = pg.PlotDataItem(
            pen=pg.mkPen(color='b', width=2),
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
    
    def create_test_parameters_panel(self) -> QGroupBox:
        """Create test parameters input panel (left column)"""
        group = QGroupBox("Test Parameters")
        layout = QGridLayout()
        layout.setHorizontalSpacing(5)  # Minimal gap between labels and widgets
        layout.setVerticalSpacing(4)    # Consistent vertical spacing between rows
        layout.setColumnStretch(0, 0)  # Don't stretch label column
        layout.setColumnStretch(1, 1)  # Allow widget column to expand

        # Battery type
        label = QLabel("Battery type:")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 0, 0)
        self.battery_type_combo = QComboBox()
        self.battery_type_combo.addItems([
            "LiPo/Li-Ion (3.7v)",
            "LiHV (3.8v)",
            "LiFePO4 (3.2v)",
            "NiMH (1.2v)",
            "NiCd (1.2v)",
            "E-Block NiMH (9v)",
            "Alkaline (1.5v)",
            "Other"
        ])
        self.battery_type_combo.setItemDelegate(CompactDelegate(self.battery_type_combo))
        layout.addWidget(self.battery_type_combo, 0, 1)

        # Cell count row (shown only for Lithium types)
        self.cell_count_label = QLabel("Cell count:")
        self.cell_count_label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(self.cell_count_label, 1, 0)
        self.cell_count_combo = QComboBox()
        self.cell_count_combo.addItems([str(n) for n in range(1, 25)] + ["Enter max voltage"])
        self.cell_count_combo.setItemDelegate(CompactDelegate(self.cell_count_combo))
        self.cell_count_combo.setToolTip("Number of cells in series")
        self.cell_count_combo.currentIndexChanged.connect(self.on_cell_count_changed)
        layout.addWidget(self.cell_count_combo, 1, 1)

        # Connect battery type change AFTER cell_count_combo exists
        self.battery_type_combo.currentIndexChanged.connect(self.on_battery_type_changed)

        # Load current (mA)
        label = QLabel("Load current (mA):")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 2, 0)
        self.current_spin = QSpinBox()
        max_current = self.config.get_max_current()
        self.current_spin.setRange(5, max_current)
        self.current_spin.setValue(self.test_data.parameters.current_ma)
        self.current_spin.setToolTip("Discharge current")
        self.current_spin.valueChanged.connect(self.on_current_changed)
        layout.addWidget(self.current_spin, 2, 1)

        # Rated Capacity (mAh)
        label = QLabel("Rated Capacity (mAh):")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 3, 0)
        self.capacity_spin = QSpinBox()
        self.capacity_spin.setRange(5, 10000)
        self.capacity_spin.setValue(self.test_data.parameters.capacity_mah)
        self.capacity_spin.valueChanged.connect(self.on_capacity_changed)
        layout.addWidget(self.capacity_spin, 3, 1)

        # Add spacing after user input fields
        layout.setRowMinimumHeight(3, 60)

        # Cutoff voltage (V) — combo for list mode, spinbox for free-entry mode
        label = QLabel("Cutoff Voltage (V):")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 4, 0)

        self.voltage_input_stack = QStackedWidget()

        self.voltage_combo = QComboBox()
        self.populate_voltage_combo()
        self.voltage_combo.setItemDelegate(CompactDelegate(self.voltage_combo))
        self.voltage_combo.setToolTip("Minimum discharge voltage (cell count × per-cell cutoff for battery type)")
        self.voltage_combo.currentIndexChanged.connect(self.on_voltage_combo_changed)
        self.voltage_input_stack.addWidget(self.voltage_combo)   # index 0

        self.voltage_spinbox = QDoubleSpinBox()
        self.voltage_spinbox.setRange(0.0, 200.0)
        self.voltage_spinbox.setDecimals(2)
        self.voltage_spinbox.setSingleStep(0.1)
        self.voltage_spinbox.setValue(4.20)
        self.voltage_spinbox.setToolTip("Minimum discharge voltage (custom value)")
        self.voltage_spinbox.valueChanged.connect(self.update_effective_cutoff_display)
        self.voltage_input_stack.addWidget(self.voltage_spinbox)  # index 1

        # Constrain stack to single-line height
        self.voltage_input_stack.setFixedHeight(self.voltage_combo.sizeHint().height())

        layout.addWidget(self.voltage_input_stack, 4, 1)

        # Effective cutoff (shown only in cell-count mode)
        self.effective_cutoff_label = QLabel("Effective cutoff:")
        self.effective_cutoff_label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(self.effective_cutoff_label, 5, 0)
        self.effective_cutoff_value = QLabel("---")
        layout.addWidget(self.effective_cutoff_value, 5, 1)

        # Battery Weight (grams)
        label = QLabel("Battery Weight (grams):")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 6, 0)
        self.battery_weight_input = QSpinBox()
        self.battery_weight_input.setRange(0, 1000)
        self.battery_weight_input.setSingleStep(1)
        self.battery_weight_input.setValue(0)
        self.battery_weight_input.setSpecialValueText("a good quality indicator")
        self.battery_weight_input.setToolTip(
            "Battery weight in grams.\n"
            "Included when saving/printing chart images.\n"
            "Not shown when set to 0."
        )
        self.battery_weight_input.valueChanged.connect(self.on_battery_weight_changed)
        self.update_battery_weight_style()  # Set initial style
        layout.addWidget(self.battery_weight_input, 6, 1)

        # Add spacing after cutoff voltage group
        layout.setRowMinimumHeight(6, 50)

        # Max discharge time (calculated + editable)
        label = QLabel("Max. discharge time:")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 7, 0)
        
        # Create horizontal layout for calculated time + editable field
        time_layout = QHBoxLayout()
        time_layout.setSpacing(5)
        
        self.time_label = QLabel("--:--")
        self.time_label.setStyleSheet("font-weight: normal;")
        self.time_label.setToolTip("Auto-calculated discharge time (Rated Capacity ÷ Load Current)")
        time_layout.addWidget(self.time_label)
        
        from PyQt6.QtWidgets import QLineEdit
        self.time_input = QLineEdit()
        self.time_input.setPlaceholderText("Override HH:MM")
        self.time_input.setMaximumWidth(130)
        self.time_input.setToolTip(
            "Calculated from rated capacity ÷ load current.\n"
            "Leave empty to use calculated time, or enter HH:MM to override."
        )
        time_layout.addWidget(self.time_input)
        time_layout.addStretch()
        
        layout.addLayout(time_layout, 7, 1)
        self.update_estimated_time()

        # Push all rows to the top — absorbs any extra vertical space
        layout.setRowStretch(8, 1)

        # Set initial visibility based on default battery type
        self.on_battery_type_changed()

        group.setLayout(layout)
        return group
    
    def create_system_settings_panel(self) -> QGroupBox:
        """Create system settings panel (right column)"""
        group = QGroupBox("System Settings")
        layout = QGridLayout()
        layout.setHorizontalSpacing(5)
        layout.setVerticalSpacing(4)
        layout.setColumnStretch(0, 0)  # Don't stretch label column
        layout.setColumnStretch(1, 1)  # Allow widget column to expand

        # Maximum discharge current (hardware limit)
        label = QLabel("Max Discharge Current (mA):")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 0, 0)
        self.max_current_spin = QSpinBox()
        self.max_current_spin.setRange(100, 5000)
        self.max_current_spin.setSingleStep(100)
        self.max_current_spin.setValue(self.config.get_max_current())
        self.max_current_spin.setToolTip(
            "Maximum current that can be sent to the controller.\n"
            "This limits the 'Load current' field for safety."
        )
        self.max_current_spin.valueChanged.connect(self.on_max_current_changed)
        layout.addWidget(self.max_current_spin, 0, 1)

        # Add spacing after max current
        layout.setRowMinimumHeight(0, 50)

        # Beep on completion checkbox
        label = QLabel("Beep on Completion:")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 1, 0)
        self.beep_enabled_checkbox = QCheckBox()
        self.beep_enabled_checkbox.setChecked(self.config.get_beep_enabled())
        self.beep_enabled_checkbox.setToolTip(
            "Play a beep sound when the battery test completes."
        )
        self.beep_enabled_checkbox.stateChanged.connect(self.on_beep_enabled_changed)
        layout.addWidget(self.beep_enabled_checkbox, 1, 1)

        # Add spacing after beep checkbox
        layout.setRowMinimumHeight(1, 50)

        # Chart title
        label = QLabel("Chart Title:")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 2, 0)
        from PyQt6.QtWidgets import QLineEdit
        self.chart_title_input = QLineEdit()
        self.chart_title_input.setPlaceholderText("Enter a title for saved/printed charts (optional)")
        self.chart_title_input.setToolTip(
            "Optional title added to saved/printed chart images."
        )
        self.chart_title_input.textChanged.connect(self.on_chart_title_changed)
        layout.addWidget(self.chart_title_input, 2, 1)

        # Dark chart background checkbox
        label = QLabel("Dark Chart Background:")
        label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(label, 3, 0)
        self.dark_chart_checkbox = QCheckBox()
        self.dark_chart_checkbox.setChecked(self.config.get_dark_chart())
        self.dark_chart_checkbox.setToolTip(
            "Use a dark background for the discharge chart."
        )
        self.dark_chart_checkbox.stateChanged.connect(self.on_dark_chart_changed)
        layout.addWidget(self.dark_chart_checkbox, 3, 1)

        # Add spacing after dark chart checkbox
        layout.setRowMinimumHeight(3, 50)

        # Push all rows to the top — absorbs any extra vertical space
        layout.setRowStretch(4, 1)

        group.setLayout(layout)
        return group
    
    def create_display_panel(self) -> QGroupBox:
        """Create current readings display panel"""
        group = QGroupBox("Current Readings")
        layout = QGridLayout()
        layout.setHorizontalSpacing(5)  # Tight horizontal spacing
        layout.setVerticalSpacing(3)    # Tight vertical spacing
        
        # Left column - Current readings (bold/large)
        row = 0
        layout.addWidget(QLabel("Time:"), row, 0)
        self.elapsed_label = QLabel("--:--:--")
        self.elapsed_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.elapsed_label, row, 1)
        
        row += 1
        layout.addWidget(QLabel("Capacity:"), row, 0)
        self.capacity_label = QLabel("-------")
        self.capacity_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.capacity_label, row, 1)
        
        row += 1
        layout.addWidget(QLabel("Voltage:"), row, 0)
        self.voltage_label = QLabel("-------")
        self.voltage_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.voltage_label, row, 1)
        self.autorange_voltage_checkbox = QCheckBox("Auto Range")
        self.autorange_voltage_checkbox.setChecked(False)
        self.autorange_voltage_checkbox.setToolTip(
            "Automatically scale the voltage Y-axis so data stays\n"
            "between 10% and 90% of the chart window."
        )
        self.autorange_voltage_checkbox.stateChanged.connect(self.on_autorange_voltage_changed)
        layout.addWidget(self.autorange_voltage_checkbox, row, 2)

        row += 1
        layout.addWidget(QLabel("Current:"), row, 0)
        self.current_label = QLabel("-------")
        self.current_label.setStyleSheet("font-size: 14pt; font-weight: bold;")
        layout.addWidget(self.current_label, row, 1)
        self.graph_current_checkbox = QCheckBox("Graph Current")
        self.graph_current_checkbox.setChecked(True)
        self.graph_current_checkbox.setToolTip("Show or hide the current trace on the real-time chart.")
        self.graph_current_checkbox.stateChanged.connect(self.on_graph_current_changed)
        layout.addWidget(self.graph_current_checkbox, row, 2)

        # Add vertical spacer between columns
        layout.setColumnMinimumWidth(2, 20)
        
        # Right column - Test settings (normal font)
        row = 0
        layout.addWidget(QLabel("Battery type:"), row, 3)
        self.display_battery_type = QLabel("---")
        layout.addWidget(self.display_battery_type, row, 4)
        
        row += 1
        layout.addWidget(QLabel("Load current (mA):"), row, 3)
        self.display_current = QLabel("---")
        layout.addWidget(self.display_current, row, 4)
        
        row += 1
        layout.addWidget(QLabel("Cutoff Voltage (V):"), row, 3)
        self.display_cutoff = QLabel("---")
        layout.addWidget(self.display_cutoff, row, 4)
        
        row += 1
        layout.addWidget(QLabel("Rated Capacity (mAh):"), row, 3)
        self.display_capacity = QLabel("---")
        layout.addWidget(self.display_capacity, row, 4)
        
        row += 1
        layout.addWidget(QLabel("Max. discharge time:"), row, 3)
        self.display_max_time = QLabel("---")
        layout.addWidget(self.display_max_time, row, 4)
        
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
    
    def create_graph_tab(self) -> QWidget:
        """Create the Graph tab with chart, readings, and messages"""
        tab = QWidget()
        layout = QVBoxLayout(tab)
        
        # Chart area
        self.chart_widget = self.create_chart()
        layout.addWidget(self.chart_widget)
        
        # Current readings display
        display_group = self.create_display_panel()
        layout.addWidget(display_group)
        
        # Controller message area
        message_group = QGroupBox("Controller Message:")
        message_layout = QVBoxLayout()
        self.message_text = QTextEdit()
        self.message_text.setReadOnly(True)
        self.message_text.setMaximumHeight(100)
        message_layout.addWidget(self.message_text)
        message_group.setLayout(message_layout)
        layout.addWidget(message_group)
        
        # Control buttons
        button_layout = self.create_button_panel()
        layout.addLayout(button_layout)
        
        return tab
    
    def create_control_tab(self) -> QWidget:
        """Create the Setup tab with parameters"""
        tab = QWidget()
        main_layout = QVBoxLayout(tab)
        
        # Horizontal layout for two-column design
        columns_layout = QHBoxLayout()
        
        # Left column: Test parameters
        test_params_group = self.create_test_parameters_panel()
        columns_layout.addWidget(test_params_group, stretch=1)
        
        # Add spacing between columns
        columns_layout.addSpacing(30)
        
        # Right column: System settings (same size as left)
        system_settings_group = self.create_system_settings_panel()
        columns_layout.addWidget(system_settings_group, stretch=1)
        
        main_layout.addLayout(columns_layout)
        main_layout.addStretch()
        
        return tab
    
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
        
        if not ports:
            # No ports found - add placeholder
            self.port_combo.addItem("No COM ports found", None)
            self.statusBar().showMessage("No serial ports detected. Connect Arduino/ESP32 and click Refresh.", 5000)
        else:
            for port in ports:
                info = SerialController.get_port_info(port)
                # Show port and description if available
                if info:
                    display_text = info
                else:
                    display_text = port
                self.port_combo.addItem(display_text, port)
            
            # Restore last used port
            last_port_idx = self.config.get_last_port_index()
            if last_port_idx < self.port_combo.count():
                self.port_combo.setCurrentIndex(last_port_idx)
            
            self.statusBar().showMessage(f"Found {len(ports)} serial port(s)", 3000)
        
        # Update tracked port list
        self.last_port_list = ports
        
        # Update UI state to enable/disable Connect button
        self.update_ui_state()
    
    def check_port_changes(self):
        """Periodically check for port changes (plug/unplug events)"""
        # Don't check if we're connected or running a test
        if self.controller.is_connected() or self.test_data.state == TestState.RUNNING:
            return
        
        current_ports = SerialController.get_available_ports()
        
        # Check if port list has changed
        if set(current_ports) != set(self.last_port_list):
            # Save current selection
            current_port = self.port_combo.currentData()
            
            # Refresh the list
            self.refresh_ports()
            
            # Try to restore selection if port still exists
            if current_port:
                for i in range(self.port_combo.count()):
                    if self.port_combo.itemData(i) == current_port:
                        self.port_combo.setCurrentIndex(i)
                        break
            
            # Show notification
            added = set(current_ports) - set(self.last_port_list)
            removed = set(self.last_port_list) - set(current_ports)
            
            if added:
                self.statusBar().showMessage(f"Port connected: {', '.join(added)}", 3000)
            if removed:
                self.statusBar().showMessage(f"Port disconnected: {', '.join(removed)}", 3000)
    
    def populate_voltage_combo(self, per_cell: bool = False):
        """Populate voltage cutoff combo box.

        When per_cell is True, shows per-cell cutoff voltages based on battery chemistry.
        When False, shows simplified list of common voltages with custom option.
        """
        self.voltage_combo.clear()
        if per_cell:
            # Per-cell voltage ranges based on battery chemistry
            battery_type = self.battery_type_combo.currentText()
            
            if battery_type.startswith("LiFePO4"):
                # LiFePO4: 3.0 V down to 2.0 V in 0.1 V steps
                voltages = [round(3.0 - i * 0.1, 1) for i in range(11)]
                default_index = 5  # 2.5V per cell
            elif battery_type.startswith("LiPo") or battery_type.startswith("LiHV"):
                # LiPo/LiHV: 3.5 V down to 2.5 V in 0.1 V steps
                voltages = [round(3.5 - i * 0.1, 1) for i in range(11)]
                default_index = 5  # 3.0V per cell
            elif battery_type.startswith("NiMH") or battery_type.startswith("NiCd"):
                # NiMH/NiCd: 1.2 V down to 0.7 V in 0.05 V steps
                voltages = [round(1.2 - i * 0.05, 2) for i in range(11)]
                default_index = 6  # 0.90V per cell
            elif battery_type.startswith("Alkaline"):
                # Alkaline: 1.5 V down to 0.7 V in 0.1 V steps
                voltages = [round(1.5 - i * 0.1, 2) for i in range(9)]
                default_index = 7  # 0.80V per cell
            else:
                # Generic: 3.5 V down to 0.5 V in 0.5 V steps
                voltages = [round(3.5 - i * 0.5, 2) for i in range(7)]
                default_index = 1  # 3.0V per cell
            
            for v in voltages:
                self.voltage_combo.addItem(f"{v:.2f}")
            self.voltage_combo.setCurrentIndex(default_index)
        else:
            # Simplified list of common cutoff voltages
            voltages = [
                ("7.50", "7.50 V (E-Block NiMH)"),
                ("3.00", "3.00 V (Li-Ion/LiPo)"),
                ("0.90", "0.90 V (NiMH/NiCd)"),
                ("0.80", "0.80 V (Alkaline)"),
                ("0.00", "0.00 V (No cutoff)"),
                ("custom", "Custom voltage...")
            ]
            for value, label in voltages:
                self.voltage_combo.addItem(label, value)
            # Default to 3.00 V
            self.voltage_combo.setCurrentIndex(1)
    
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
        
        # Load battery weight and chart title
        self.battery_weight_input.setValue(params.battery_weight)
        self.update_battery_weight_style()
        self.chart_title_input.setText(params.chart_title)

        # Apply saved chart theme
        self.apply_chart_theme(self.config.get_dark_chart())

        self.update_estimated_time()
        self.update_display_parameters()
    
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
        # Guard: Don't update if UI not fully initialized yet
        if not hasattr(self, 'connect_btn'):
            return
        
        is_connected = self.controller.is_connected()
        is_running = self.test_data.state == TestState.RUNNING
        has_valid_port = self.port_combo.count() > 0 and self.port_combo.currentData() is not None
        
        self.connect_btn.setEnabled(is_connected and not is_running or has_valid_port and not is_connected)
        self.refresh_btn.setEnabled(not is_connected)  # Disable refresh when connected
        self.start_btn.setEnabled(is_connected and not is_running)
        self.cancel_btn.setEnabled(is_running)
        self.new_test_btn.setEnabled(is_connected and not is_running)
        
        # Update LED
        if not is_connected:
            self.led_indicator.set_state('red')
            self.connect_btn.setText("Connect")
        else:
            # Connected - show green whether idle or running
            self.led_indicator.set_state('green')
            self.connect_btn.setText("Disconnect")
        
        # Enable/disable parameters during test
        self.current_spin.setEnabled(not is_running)
        self.voltage_combo.setEnabled(not is_running)
        self.voltage_spinbox.setEnabled(not is_running)
        self.cell_count_combo.setEnabled(not is_running)
        self.capacity_spin.setEnabled(not is_running)
    
    # Event handlers
    def on_port_changed(self):
        """Handle COM port selection change"""
        if self.controller.is_connected():
            self.controller.disconnect()
        
        # Update UI state to enable/disable Connect button based on valid port selection
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
    
    def on_max_current_changed(self):
        """Handle max current limit change"""
        max_current = self.max_current_spin.value()
        
        # Save to config
        self.config.set_max_current(max_current)
        
        # Update the Load current spinbox range
        current_value = self.current_spin.value()
        self.current_spin.setRange(5, max_current)
        
        # If current value exceeds new max, clamp it
        if current_value > max_current:
            self.current_spin.setValue(max_current)
    
    def on_beep_enabled_changed(self):
        """Handle beep enabled checkbox change"""
        beep_enabled = self.beep_enabled_checkbox.isChecked()
        
        # Save to config
        self.config.set_beep_enabled(beep_enabled)
    
    def on_battery_weight_changed(self):
        """Handle battery weight input change"""
        weight = self.battery_weight_input.value()
        
        # Save to config
        self.config.set_battery_weight(weight)
        
        # Update style to dim special value text
        self.update_battery_weight_style()
    
    def update_battery_weight_style(self):
        """Update battery weight spinbox style to dim special value text"""
        if self.battery_weight_input.value() == 0:
            # Dim the text when showing special value (like placeholder text)
            # Use a gray color similar to placeholder text
            self.battery_weight_input.setStyleSheet("QSpinBox { color: #808080; } QSpinBox::up-button { border: none; } QSpinBox::down-button { border: none; }")
        else:
            # Normal text color
            self.battery_weight_input.setStyleSheet("")
    
    def on_chart_title_changed(self):
        """Handle chart title input change"""
        title = self.chart_title_input.text()

        # Save to config
        self.config.set_chart_title(title)

    def on_tab_changed(self, index: int):
        """Update display parameters when switching to the Graph tab"""
        if index == 0:  # Graph tab
            self.update_display_parameters()

    def on_graph_current_changed(self):
        """Handle Graph Current checkbox change"""
        show = self.graph_current_checkbox.isChecked()
        self.current_curve.setVisible(show)
        self.chart_widget.getAxis('right').setVisible(show)
        if not show:
            self.current_curve.setData([], [])

    def on_autorange_voltage_changed(self):
        """Handle Auto Range voltage checkbox change — apply immediately in both directions"""
        if self.autorange_voltage_checkbox.isChecked():
            _, voltages = self.test_data.get_voltage_series()
            if voltages:
                v_min, v_max = min(voltages), max(voltages)
                span = v_max - v_min if v_max != v_min else 1.0
                margin = span * (0.10 / 0.80)
                self.chart_widget.setYRange(v_min - margin, v_max + margin, padding=0)
        else:
            battery_type = self.battery_type_combo.currentText()
            max_voltage = self.get_battery_max_voltage(battery_type)
            if self._cell_count_mode_active():
                max_voltage *= int(self.cell_count_combo.currentText())
            self.chart_widget.setYRange(0, max_voltage, padding=0)

    def on_dark_chart_changed(self):
        """Handle dark chart background checkbox change"""
        dark = self.dark_chart_checkbox.isChecked()
        self.config.set_dark_chart(dark)
        self.apply_chart_theme(dark)

    def apply_chart_theme(self, dark: bool):
        """Apply light or dark theme to the chart widget"""
        if not hasattr(self, 'chart_widget'):
            return
        if dark:
            bg, axis_color = 'k', (200, 200, 200)
            label_color = (255, 255, 255)  # White for dark background - maximum visibility
            grid_alpha = 0.5  # Higher alpha for visibility on dark background
        else:
            bg, axis_color = 'w', (0, 0, 0)
            label_color = (0, 150, 0)  # Dark green for white background
            grid_alpha = 0.3  # Lower alpha for subtle grid on white background
        
        self.chart_widget.setBackground(bg)
        self.chart_widget.showGrid(x=True, y=True, alpha=grid_alpha)
        
        for axis in ('left', 'bottom', 'right'):
            ax = self.chart_widget.getAxis(axis)
            ax.setPen(pg.mkPen(color=axis_color))
            ax.setTextPen(pg.mkPen(color=axis_color))
            # Set label color using QColor
            from PyQt6.QtGui import QColor
            ax.label.setDefaultTextColor(QColor(*label_color))
        
        # Set labels with text
        self.chart_widget.setLabel('left', 'Voltage', units='V')
        self.chart_widget.setLabel('bottom', 'Time', units='seconds')
        self.chart_widget.setLabel('right', 'Current', units='mA')
        
        # Force update to ensure axes are visible
        self.chart_widget.update()

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

    def on_voltage_combo_changed(self):
        """Handle voltage combo box change - switch to spinbox for custom entry"""
        if not self._cell_count_mode_active():
            # For non-lithium batteries, check if "Custom voltage..." was selected
            current_data = self.voltage_combo.currentData()
            if current_data == "custom":
                # Switch to spinbox for custom entry
                self.voltage_input_stack.setCurrentIndex(1)
            else:
                # Show combo
                self.voltage_input_stack.setCurrentIndex(0)
        
        self.update_effective_cutoff_display()
    
    def _is_lithium_type(self) -> bool:
        """Return True for lithium battery types that support cell counting"""
        text = self.battery_type_combo.currentText()
        return text.startswith("LiPo") or text.startswith("LiHV") or text.startswith("LiFePO4") or text.startswith("Lithium")

    def on_battery_type_changed(self):
        """Show cell count row and update voltage input for all battery types"""
        # Always show cell count - available for all battery types
        self.cell_count_label.setVisible(True)
        self.cell_count_combo.setVisible(True)
        
        # Update voltage combo for the new battery type
        self.on_cell_count_changed(auto_set_voltage=False)  # Don't auto-set yet
        
        # Auto-set cutoff voltage to recommended value for this battery type
        self.set_cutoff_voltage_to_recommended()
        
        self.update_effective_cutoff_display()

    def on_cell_count_changed(self, auto_set_voltage: bool = True):
        """Switch voltage input between per-cell combo and free-entry spinbox"""
        if self.cell_count_combo.currentText() == "Enter max voltage":
            self.voltage_input_stack.setCurrentIndex(1)  # show spinbox
        else:
            self.voltage_input_stack.setCurrentIndex(0)  # show combo
            self.populate_voltage_combo(per_cell=True)
            # Auto-set to recommended cutoff voltage for this battery type (if requested)
            if auto_set_voltage:
                self.set_cutoff_voltage_to_recommended()
        self.update_effective_cutoff_display()

    def update_effective_cutoff_display(self):
        """Update the live effective cutoff label in the parameters panel and Current Readings panel"""
        in_cell_mode = self._cell_count_mode_active()
        self.effective_cutoff_label.setVisible(in_cell_mode)
        self.effective_cutoff_value.setVisible(in_cell_mode)
        if in_cell_mode:
            if not self.voltage_combo.currentText():
                return
            per_cell = float(self.voltage_combo.currentText())
            cell_count = int(self.cell_count_combo.currentText())
            total = round(per_cell * cell_count, 2)
            self.effective_cutoff_value.setText(f"{total:.2f} V  ({per_cell:.2f} V/cell)")
            self.display_cutoff.setText(f"{total:.2f} ({per_cell:.2f} V/cell)")
        elif self.cell_count_combo.currentText() == "Enter max voltage":
            # Free-entry spinbox mode
            self.display_cutoff.setText(f"{self.voltage_spinbox.value():.2f}")
        elif self.voltage_combo.currentData() == "custom":
            # Custom voltage selected
            self.display_cutoff.setText(f"{self.voltage_spinbox.value():.2f}")
        else:
            # Regular combo selection
            voltage_value = self.get_effective_cutoff_voltage()
            self.display_cutoff.setText(f"{voltage_value:.2f}")

    def update_display_parameters(self):
        """Populate the Current Readings panel with current setup values"""
        if not hasattr(self, 'display_battery_type'):
            return
        params = self.test_data.parameters
        self.display_battery_type.setText(self.battery_type_combo.currentText())
        self.display_current.setText(f"{params.current_ma}")
        self.update_effective_cutoff_display()
        self.display_capacity.setText(f"{params.capacity_mah}")
        max_time = params.calculate_max_time_minutes()
        hours = int(max_time // 60)
        minutes = int(max_time % 60)
        self.display_max_time.setText(f"{hours} hr : {minutes} min")

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
    
    def _cell_count_mode_active(self) -> bool:
        """Return True when cell-count mode is active.

        Cell count mode is active when a numeric cell count is selected
        (not 'Enter max voltage').
        """
        return self.cell_count_combo.currentText() != "Enter max voltage"

    def get_effective_cutoff_voltage(self) -> float:
        """Return the cutoff voltage to send to the hardware.

        Cell-count mode: per-cell combo value × cell count.
        Enter max voltage mode: spinbox value used directly.
        Custom voltage: spinbox value used directly.
        """
        if self._cell_count_mode_active():
            # Per-cell mode: multiply per-cell voltage by cell count
            per_cell = float(self.voltage_combo.currentText())
            return round(per_cell * int(self.cell_count_combo.currentText()), 2)
        if self.cell_count_combo.currentText() == "Enter max voltage":
            # Free-entry spinbox mode
            return self.voltage_spinbox.value()
        # Check if custom voltage is selected
        if self.voltage_combo.currentData() == "custom":
            return self.voltage_spinbox.value()
        # Standard voltage selection from combo data
        voltage_data = self.voltage_combo.currentData()
        return float(voltage_data) if voltage_data and voltage_data != "custom" else 0.0

    def get_battery_max_voltage(self, battery_type: str) -> float:
        """Get maximum voltage for battery type with 10% padding"""
        voltage_map = {
            "LiPo/Li-Ion (3.7v)": 4.2 * 1.1,      # 4.62V
            "LiHV (3.8v)": 4.2 * 1.1,      # 4.62V
            "LiFePO4 (3.2v)": 3.6 * 1.1,      # 3.96V
            "NiMH (1.2v)": 1.5 * 1.1,         # 1.65V
            "NiCd (1.2v)": 1.5 * 1.1,         # 1.65V
            "E-Block NiMH (9v)": 10.0 * 1.1,  # 11.0V
            "Alkaline (1.5v)": 1.6 * 1.1,     # 1.76V
            "Other": 5.0                       # Default 5V
        }
        return voltage_map.get(battery_type, 5.0)
    
    def get_recommended_cutoff_voltage(self, battery_type: str) -> float:
        """Get recommended minimum cutoff voltage per cell for battery type"""
        # Per-cell cutoff voltages (for lithium types) or absolute voltage (for others)
        cutoff_map = {
            "LiPo/Li-Ion (3.7v)": 3.0,    # 3.0V per cell (safe discharge)
            "LiHV (3.8v)": 3.0,           # 3.0V per cell (conservative)
            "LiFePO4 (3.2v)": 2.5,        # 2.5V per cell (recommended minimum)
            "NiMH (1.2v)": 0.90,          # 0.9V per cell
            "NiCd (1.2v)": 0.90,          # 0.9V per cell
            "E-Block NiMH (9v)": 7.50,    # ~0.9V per cell for 9V battery
            "Alkaline (1.5v)": 0.80,      # 0.8V per cell
            "Other": 3.00                  # Safe default
        }
        return cutoff_map.get(battery_type, 3.0)
    
    def set_cutoff_voltage_to_recommended(self):
        """Set cutoff voltage to recommended value based on battery type"""
        battery_type = self.battery_type_combo.currentText()
        recommended = self.get_recommended_cutoff_voltage(battery_type)
        
        # Check if we're in per-cell mode or absolute voltage mode
        in_cell_mode = self._cell_count_mode_active()
        
        if in_cell_mode:
            # For per-cell mode, set per-cell voltage in combo (text-based)
            for i in range(self.voltage_combo.count()):
                voltage_text = self.voltage_combo.itemText(i)
                if voltage_text and abs(float(voltage_text) - recommended) < 0.01:
                    self.voltage_combo.setCurrentIndex(i)
                    break
        else:
            # For absolute voltage mode, find matching value in data
            for i in range(self.voltage_combo.count()):
                voltage_data = self.voltage_combo.itemData(i)
                if voltage_data and voltage_data != "custom":
                    if abs(float(voltage_data) - recommended) < 0.01:
                        self.voltage_combo.setCurrentIndex(i)
                        break
    
    def start_test(self):
        """Start battery discharge test"""
        # Get parameters
        params = TestParameters(
            current_ma=self.current_spin.value(),
            cutoff_voltage=self.get_effective_cutoff_voltage(),
            capacity_mah=self.capacity_spin.value(),
            sample_interval_sec=1,
            beep_enabled=self.beep_enabled_checkbox.isChecked(),
            battery_weight=self.battery_weight_input.value(),
            chart_title=self.chart_title_input.text()
        )
        
        # Validate against maximum current limit
        max_current = self.config.get_max_current()
        if params.current_ma > max_current:
            QMessageBox.warning(
                self,
                "Current Limit Exceeded",
                f"The requested current ({params.current_ma} mA) exceeds the maximum limit ({max_current} mA).\n\n"
                f"Please reduce the current or adjust the limit in File → Setup."
            )
            return
        
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
        
        # Update display panel with test settings
        self.display_battery_type.setText(self.battery_type_combo.currentText())
        self.display_current.setText(f"{params.current_ma}")
        if self._cell_count_mode_active():
            per_cell = float(self.voltage_combo.currentText())
            self.display_cutoff.setText(f"{params.cutoff_voltage:.2f} ({per_cell:.2f} V/cell)")
        else:
            self.display_cutoff.setText(f"{params.cutoff_voltage:.2f}")
        self.display_capacity.setText(f"{params.capacity_mah}")
        max_time = params.calculate_max_time_minutes()
        hours = int(max_time // 60)
        minutes = int(max_time % 60)
        self.display_max_time.setText(f"{hours} hr : {minutes} min")
        
        # Clear chart
        self.voltage_curve.setData([], [])
        self.current_curve.setData([], [])
        
        # Initialize X-axis to show 10 minutes (600 seconds)
        self.chart_widget.setXRange(0, 600, padding=0)
        
        # Set voltage Y-axis to fixed range based on battery type with 10% padding
        battery_type = self.battery_type_combo.currentText()
        max_voltage = self.get_battery_max_voltage(battery_type)
        if self._cell_count_mode_active():
            max_voltage *= int(self.cell_count_combo.currentText())
        self.chart_widget.setYRange(0, max_voltage, padding=0)
        
        # Set current Y-axis to fixed range based on set current value with 20% padding
        max_current = params.current_ma * 1.2
        self.current_viewbox.setYRange(0, max_current, padding=0)
        
        # Send parameters to controller
        max_time = params.calculate_max_time_minutes()
        success = self.protocol.send_test_parameters(
            current_ma=params.current_ma,
            cutoff_voltage=params.cutoff_voltage,
            time_limit_minutes=max_time,
            sample_interval_sec=params.sample_interval_sec,
            beep_enabled=params.beep_enabled
        )
        
        if not success:
            QMessageBox.critical(self, "Error", "Failed to send test parameters")
            self.test_data.state = TestState.ERROR
            return
        
        # Log sent parameters for verification
        if hasattr(self, 'message_text'):
            self.message_text.append(f"Sent parameters: Current={params.current_ma}mA, Cutoff={params.cutoff_voltage}V, Time={max_time}min")
        
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
        
        # Beep to alert user of test completion (if enabled)
        if self.test_data.parameters.beep_enabled:
            QApplication.beep()
        
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
            # Send cancel/termination message to controller
            if self.protocol:
                self.protocol.cancel_test()
            
            # Clear displays
            self.test_data.clear()
            self.elapsed_label.setText("--:--:--")
            self.voltage_label.setText("-------- V")
            self.current_label.setText("-------- mA")
            self.capacity_label.setText("-------- mAh")
            self.display_battery_type.setText("---")
            self.display_current.setText("---")
            self.display_cutoff.setText("---")
            self.display_capacity.setText("---")
            self.display_max_time.setText("---")
            self.voltage_curve.setData([], [])
            self.current_curve.setData([], [])
            self.status_label1.setText("")
            self.update_ui_state()
    
    def on_data_received(self, reading: TestReading):
        """Handle received data from controller"""
        # Debug: Log received data
        if hasattr(self, 'message_text'):
            self.message_text.append(
                f"RX: Time={reading.elapsed_time} V={reading.voltage:.3f}V I={reading.current_ma:.1f}mA Cap={reading.capacity_mah:.2f}mAh"
            )
        
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
        
        # Auto-range voltage axis: keep data between 10% and 90% of window
        if self.autorange_voltage_checkbox.isChecked() and voltages:
            v_min, v_max = min(voltages), max(voltages)
            span = v_max - v_min if v_max != v_min else 1.0
            margin = span * (0.10 / 0.80)  # 80% of window = data span → 10% margins each side
            self.chart_widget.setYRange(v_min - margin, v_max + margin, padding=0)
        
        # Switch X-axis to minutes after 300 seconds
        if times:
            # Get the appropriate label color based on current theme
            from PyQt6.QtGui import QColor
            label_color = (255, 255, 255) if self.config.get_dark_chart() else (0, 150, 0)
            
            if times[-1] > 300:
                plot_times = [t / 60.0 for t in times]
                self.chart_widget.setLabel('bottom', 'Time', units='minutes')
                # Reapply label color after setLabel
                self.chart_widget.getAxis('bottom').label.setDefaultTextColor(QColor(*label_color))
                x_max = plot_times[-1] if plot_times[-1] > 10 else 10
            else:
                plot_times = times
                self.chart_widget.setLabel('bottom', 'Time', units='seconds')
                # Reapply label color after setLabel
                self.chart_widget.getAxis('bottom').label.setDefaultTextColor(QColor(*label_color))
                x_max = 600 if times[-1] <= 600 else times[-1]
            self.voltage_curve.setData(plot_times, voltages)
            if self.graph_current_checkbox.isChecked():
                self.current_curve.setData(plot_times, currents)
            self.chart_widget.setXRange(0, x_max, padding=0.02)
    
    def on_message_received(self, message: str):
        """Handle status message from controller"""
        self.status_label3.setText(message)
        
        # Also append to message text box in Control tab
        if hasattr(self, 'message_text'):
            self.message_text.append(message)
        
        # Check for test completion
        if any(keyword in message for keyword in ['Finished', 'Exceeded', 'Error', 'Cancelled', 'Cutoff']):
            self.stop_test(message)
    
    def on_error_occurred(self, error: str):
        """Handle serial communication error"""
        QMessageBox.critical(self, "Communication Error", f"Serial error: {error}")
        self.stop_test(f"Error: {error}")
    
    # Menu actions
    def save_chart(self):
        """Save chart as image with test results overlay"""
        if not self.test_data.data_points:
            QMessageBox.warning(
                self,
                "No Data",
                "No test data available to save.\n\nPlease run a battery test first."
            )
            return
        
        success, error = ExportManager.export_chart_image(
            self.chart_widget,
            self.test_data,
            self.test_data.parameters,
            parent=self
        )
        
        if error:
            QMessageBox.critical(
                self,
                "Export Failed",
                f"Failed to save chart image:\n\n{error}"
            )
        elif success:
            QMessageBox.information(
                self,
                "Export Successful",
                "Chart image saved successfully!"
            )
    
    def export_csv(self):
        """Export test data as CSV file"""
        if not self.test_data.data_points:
            QMessageBox.warning(
                self,
                "No Data",
                "No test data available to export.\n\nPlease run a battery test first."
            )
            return
        
        # Check minimum data points (like Delphi version requires 6 points)
        if len(self.test_data.data_points) < 6:
            QMessageBox.warning(
                self,
                "Insufficient Data",
                "Not enough data to export.\n\nAt least 6 data points are required."
            )
            return
        
        success, error = ExportManager.export_to_csv(
            self.test_data,
            self.test_data.parameters,
            parent=self
        )
        
        if error:
            QMessageBox.critical(
                self,
                "Export Failed",
                f"Failed to export CSV:\n\n{error}"
            )
        elif success:
            QMessageBox.information(
                self,
                "Export Successful",
                "Test data exported to CSV successfully!"
            )
    
    def print_chart(self):
        """Print chart to printer or PDF"""
        if not self.test_data.data_points:
            QMessageBox.warning(
                self,
                "No Data",
                "No test data available to print.\n\nPlease run a battery test first."
            )
            return
        
        success, error = ExportManager.print_chart(
            self.chart_widget,
            self.test_data,
            self.test_data.parameters,
            parent=self
        )
        
        if error:
            QMessageBox.critical(
                self,
                "Print Failed",
                f"Failed to print chart:\n\n{error}"
            )
        elif success:
            # Print dialog already shows success/completion
            pass
    
    def copy_chart(self):
        """Copy chart to clipboard"""
        if not self.test_data.data_points:
            QMessageBox.warning(
                self,
                "No Data",
                "No test data available to copy.\n\nPlease run a battery test first."
            )
            return
        
        success, error = ExportManager.copy_chart_to_clipboard(
            self.chart_widget,
            parent=self
        )
        
        if error:
            QMessageBox.critical(
                self,
                "Copy Failed",
                f"Failed to copy chart to clipboard:\n\n{error}"
            )
        elif success:
            self.statusbar.showMessage("Chart copied to clipboard", 3000)
    
    def show_help(self):
        """Show help dialog"""
        QMessageBox.information(
            self,
            "Help",
            "Battery Tester Help\n\n" +
            "1. Power on EL and connect USB cable\n" +
            "2. Select COM port and click Connect\n" +
            "3. Use EL encoder to select Battery Test mode\n" +
            "4. Set test parameters (current, voltage, capacity)\n" +
            "5. Click Start Test\n" +
            "6. Check EL display for correct parameters and press encoder to enable current flow\n" +
            "7. Monitor real-time discharge curve\n" +
            "8. Export results when complete"
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
        if self.port_monitor_timer:
            self.port_monitor_timer.stop()
        
        if self.serial_worker:
            self.serial_worker.stop()
            self.serial_worker.wait()
        
        if self.controller.is_connected():
            self.controller.disconnect()
        
        event.accept()
