"""
Dialog windows for Battery Tester application
"""

from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QPushButton,
    QLabel
)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QPixmap, QFont


class AboutDialog(QDialog):
    """About dialog showing version and credits"""
    
    def __init__(self, version: str, parent=None):
        super().__init__(parent)
        self.setWindowTitle("About Battery Tester")
        self.setFixedSize(400, 250)
        
        layout = QVBoxLayout()
        layout.setSpacing(15)
        
        # Title
        title = QLabel("Battery Tester")
        title_font = QFont()
        title_font.setPointSize(16)
        title_font.setBold(True)
        title.setFont(title_font)
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title)
        
        # Version
        version_label = QLabel(version)
        version_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(version_label)
        
        layout.addSpacing(10)
        
        # Credits
        credits_text = (
            "<b>Python Port</b><br>"
            "© 2026<br><br>"
            "<b>Original Delphi Application</b><br>"
            "© 2019 vwlowen.co.uk<br><br>"
            "<i>In memory of the original developer</i><br><br>"
            "For use with Arduino/ESP32 Dynamic Load Controller"
        )
        credits = QLabel(credits_text)
        credits.setAlignment(Qt.AlignmentFlag.AlignCenter)
        credits.setWordWrap(True)
        layout.addWidget(credits)
        
        layout.addStretch()
        
        # OK button
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        ok_btn = QPushButton("OK")
        ok_btn.clicked.connect(self.accept)
        ok_btn.setDefault(True)
        button_layout.addWidget(ok_btn)
        button_layout.addStretch()
        
        layout.addLayout(button_layout)
        self.setLayout(layout)


class NewTestDialog(QDialog):
    """Dialog for confirming new test"""
    
    def __init__(self, reset_default: bool = False, parent=None):
        super().__init__(parent)
        self.setWindowTitle("New Test")
        self.setModal(True)
        self.setFixedSize(350, 120)
        
        layout = QVBoxLayout()
        
        # Message
        message = QLabel(
            "Controller will be prepared for a new test.\n\n"
            "Current data will be cleared."
        )
        message.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(message)
        
        layout.addStretch()
        
        # Buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        yes_btn = QPushButton("Yes")
        yes_btn.clicked.connect(self.accept)
        yes_btn.setDefault(True)
        button_layout.addWidget(yes_btn)
        
        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(cancel_btn)
        
        button_layout.addStretch()
        
        layout.addLayout(button_layout)
        self.setLayout(layout)
