"""
Export utilities for Battery Tester application
Handles CSV export, image export, and print support
"""

import csv
from datetime import datetime
from pathlib import Path
from typing import Optional, Tuple
import locale

from PyQt6.QtWidgets import QFileDialog, QMessageBox, QInputDialog
from PyQt6.QtGui import QPainter, QPixmap, QPdfWriter, QPageLayout, QPageSize, QColor, QFont
from PyQt6.QtCore import QMarginsF, QRectF, Qt
from PyQt6.QtPrintSupport import QPrintDialog, QPrinter

from core.models import TestData, TestParameters


class ExportManager:
    """Manages data export operations"""
    
    @staticmethod
    def export_to_csv(test_data: TestData, test_params: TestParameters, 
                      parent=None) -> Tuple[bool, Optional[str]]:
        """
        Export test data to CSV file
        
        Args:
            test_data: Test data with measurements
            test_params: Test configuration parameters
            parent: Parent widget for file dialog
            
        Returns:
            Tuple of (success, error_message)
        """
        if not test_data.data_points:
            return False, "No data to export"
        
        # File dialog
        file_path, _ = QFileDialog.getSaveFileName(
            parent,
            "Export Test Data",
            f"battery_test_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
            "CSV Files (*.csv);;All Files (*)"
        )
        
        if not file_path:
            return False, None  # User cancelled
        
        try:
            # Determine decimal separator based on locale
            try:
                decimal_sep = locale.localeconv()['decimal_point']
            except:
                decimal_sep = '.'
            
            with open(file_path, 'w', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                
                # Header
                writer.writerow(['Battery Tester Data Export'])
                writer.writerow(['Date:', datetime.now().strftime('%Y-%m-%d %H:%M:%S')])
                writer.writerow([])
                
                # Test parameters
                writer.writerow(['Test Parameters'])
                writer.writerow(['Discharge Current (mA):', test_params.current_ma])
                writer.writerow(['Cutoff Voltage (V):', 
                               str(test_params.cutoff_voltage).replace('.', decimal_sep)])
                writer.writerow(['Max Capacity (mAh):', test_params.max_capacity_mah])
                writer.writerow(['Time Limit (min):', test_params.time_limit_minutes])
                writer.writerow([])
                
                # Column headers
                writer.writerow(['Time (s)', 'Voltage (V)', 'Current (mA)', 'Capacity (mAh)'])
                
                # Data points
                for point in test_data.data_points:
                    voltage_str = f"{point.voltage:.3f}".replace('.', decimal_sep)
                    current_str = f"{point.current_ma:.1f}".replace('.', decimal_sep)
                    capacity_str = f"{point.capacity_mah:.1f}".replace('.', decimal_sep)
                    
                    writer.writerow([
                        point.elapsed_seconds,
                        voltage_str,
                        current_str,
                        capacity_str
                    ])
            
            return True, None
            
        except Exception as e:
            return False, f"Failed to export CSV: {str(e)}"
    
    @staticmethod
    def export_chart_image(chart_widget, test_data: TestData, 
                          test_params: TestParameters, parent=None) -> Tuple[bool, Optional[str]]:
        """
        Export chart as image with metadata overlay (matches Delphi version)
        
        Args:
            chart_widget: The pyqtgraph PlotWidget to export
            test_data: Test data for metadata
            test_params: Test parameters for metadata
            parent: Parent widget for file dialog
            
        Returns:
            Tuple of (success, error_message)
        """
        if not test_data.data_points:
            return False, "No data to export"
        
        # Use battery weight from parameters (set in Setup tab)
        if test_params.battery_weight > 0:
            battery_weight = f"{test_params.battery_weight} grams"
        else:
            battery_weight = "a good quality indicator"
        
        # Use chart title from parameters if provided
        chart_title = test_params.chart_title if test_params.chart_title else "battery_chart"
        
        # File dialog
        default_filename = f"{chart_title}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.png"
        file_path, _ = QFileDialog.getSaveFileName(
            parent,
            "Save Chart Image",
            f"battery_chart_{datetime.now().strftime('%Y%m%d_%H%M%S')}.png",
            "PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;BMP Image (*.bmp);;All Files (*)"
        )
        
        if not file_path:
            return False, None  # User cancelled
        
        try:
            # Grab the chart widget as pixmap
            pixmap = chart_widget.grab()
            
            # Create painter to add metadata overlay
            painter = QPainter(pixmap)
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            
            # Calculate position for text overlay (centered-right area like Delphi)
            chart_width = pixmap.width()
            chart_height = pixmap.height()
            text_x = int(chart_width * 0.45)  # Start at 45% width
            text_y = int(chart_height * 0.35)  # Start at 35% height
            
            # Draw metadata text with Courier New font like Delphi
            painter.setPen(QColor(0, 0, 0))
            
            # Title - Bold and underlined
            font = QFont("Courier New", 9)
            font.setBold(True)
            font.setUnderline(True)
            painter.setFont(font)
            
            # Use custom title if provided, otherwise use default
            title_text = test_params.chart_title if test_params.chart_title else "Battery Discharge Test Results"
            painter.drawText(text_x, text_y, title_text)
            
            # Reset font for data - normal weight, no underline
            font.setBold(False)
            font.setUnderline(False)
            painter.setFont(font)
            
            # Get test data for display
            first_point = test_data.data_points[0]
            last_point = test_data.data_points[-1]
            
            # Format duration as HH:MM:SS
            duration_seconds = last_point.elapsed_seconds
            hours = duration_seconds // 3600
            minutes = (duration_seconds % 3600) // 60
            seconds = duration_seconds % 60
            duration_str = f"{hours:02d}:{minutes:02d}:{seconds:02d}"
            
            # Build metadata lines (matching Delphi format exactly)
            line_height = 15
            y_offset = text_y + 15
            
            metadata_lines = [
                f"Date of test    : {datetime.now().strftime('%a %d %b %Y')}",
                f"Duration of test: {duration_str}",
                f"Rated  capacity : {test_params.capacity_mah} mAh",
                f"Tested capacity : {last_point.capacity_mah:.1f} mAh",
                f"Initial voltage : {first_point.voltage:.2f}V",
                f"Cutoff voltage  : {test_params.cutoff_voltage:.2f}V",
                f"Test current    : {test_params.current_ma} mA",
            ]
            
            # Only include battery weight if it's provided (not 0)
            if test_params.battery_weight > 0:
                metadata_lines.append(f"Battery weight  : {battery_weight}")
            
            for line in metadata_lines:
                painter.drawText(text_x, y_offset, line)
                y_offset += line_height
            
            painter.end()
            
            # Save pixmap
            pixmap.save(file_path)
            
            return True, None
            
        except Exception as e:
            return False, f"Failed to export image: {str(e)}"
    
    @staticmethod
    def print_chart(chart_widget, test_data: TestData, 
                   test_params: TestParameters, parent=None) -> Tuple[bool, Optional[str]]:
        """
        Print chart to printer or PDF
        
        Args:
            chart_widget: The pyqtgraph PlotWidget to print
            test_data: Test data for metadata
            test_params: Test parameters for metadata
            parent: Parent widget for print dialog
            
        Returns:
            Tuple of (success, error_message)
        """
        if not test_data.data_points:
            return False, "No data to print"
        
        try:
            # Create printer
            printer = QPrinter(QPrinter.PrinterMode.HighResolution)
            printer.setPageOrientation(QPageLayout.Orientation.Landscape)
            
            # Show print dialog
            dialog = QPrintDialog(printer, parent)
            dialog.setWindowTitle("Print Chart")
            
            if dialog.exec() != QPrintDialog.DialogCode.Accepted:
                return False, None  # User cancelled
            
            # Grab chart as pixmap
            pixmap = chart_widget.grab()
            
            # Create painter
            painter = QPainter(printer)
            
            # Calculate scaling to fit page
            page_rect = printer.pageLayout().paintRectPixels(printer.resolution())
            pixmap_rect = pixmap.rect()
            
            # Scale to fit width or height
            scale_x = page_rect.width() / pixmap_rect.width()
            scale_y = page_rect.height() / pixmap_rect.height()
            scale = min(scale_x, scale_y) * 0.9  # 90% to leave margins
            
            # Calculate centered position
            scaled_width = pixmap_rect.width() * scale
            scaled_height = pixmap_rect.height() * scale
            x_offset = (page_rect.width() - scaled_width) / 2
            y_offset = (page_rect.height() - scaled_height) / 2
            
            # Draw pixmap
            target_rect = QRectF(x_offset, y_offset, scaled_width, scaled_height)
            source_rect = QRectF(pixmap_rect)
            painter.drawPixmap(target_rect, pixmap, source_rect)
            
            # Add metadata at bottom
            painter.setPen(QColor(0, 0, 0))
            font = QFont("Arial", 10)
            painter.setFont(font)
            
            last_point = test_data.data_points[-1]
            duration_str = f"{last_point.elapsed_seconds // 60}m {last_point.elapsed_seconds % 60}s"
            
            metadata_text = (
                f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M')}  |  "
                f"Duration: {duration_str}  |  "
                f"Current: {test_params.current_ma} mA  |  "
                f"Cutoff: {test_params.cutoff_voltage:.2f} V  |  "
                f"Capacity: {last_point.capacity_mah:.1f} mAh"
            )
            
            painter.drawText(
                QRectF(x_offset, y_offset + scaled_height + 20, scaled_width, 30),
                Qt.AlignmentFlag.AlignCenter,
                metadata_text
            )
            
            painter.end()
            
            return True, None
            
        except Exception as e:
            return False, f"Failed to print: {str(e)}"
    
    @staticmethod
    def copy_chart_to_clipboard(chart_widget, parent=None) -> Tuple[bool, Optional[str]]:
        """
        Copy chart to system clipboard
        
        Args:
            chart_widget: The pyqtgraph PlotWidget to copy
            parent: Parent widget for error dialogs
            
        Returns:
            Tuple of (success, error_message)
        """
        try:
            from PyQt6.QtWidgets import QApplication
            
            # Grab chart as pixmap
            pixmap = chart_widget.grab()
            
            # Copy to clipboard
            clipboard = QApplication.clipboard()
            clipboard.setPixmap(pixmap)
            
            return True, None
            
        except Exception as e:
            return False, f"Failed to copy to clipboard: {str(e)}"
