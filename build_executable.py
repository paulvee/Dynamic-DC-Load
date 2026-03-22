#!/usr/bin/env python3
"""
Build script for creating Battery Tester executables using PyInstaller

Usage:
    python build_executable.py

This will create a standalone executable in the dist/ folder.
You must run this on each target platform (Windows, macOS, Linux).
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path


def clean_build_folders():
    """Remove old build artifacts"""
    folders_to_clean = ['build', 'dist', '__pycache__']
    for folder in folders_to_clean:
        if os.path.exists(folder):
            print(f"Cleaning {folder}/...")
            shutil.rmtree(folder)
    
    # Remove spec file if it exists
    spec_files = list(Path('.').glob('*.spec'))
    for spec_file in spec_files:
        print(f"Removing {spec_file}...")
        spec_file.unlink()


def check_dependencies():
    """Verify PyInstaller is installed"""
    try:
        import PyInstaller
        print(f"✓ PyInstaller {PyInstaller.__version__} found")
        return True
    except ImportError:
        print("✗ PyInstaller not found")
        print("\nInstalling PyInstaller...")
        subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller>=6.0.0"])
        return True


def build_executable():
    """Build the executable using PyInstaller"""
    
    # Determine platform-specific settings
    platform = sys.platform
    
    if platform == 'win32':
        name = 'BatteryTester'
        icon_arg = ['--icon=icon.ico'] if os.path.exists('icon.ico') else []
        windowed = '--windowed'
    elif platform == 'darwin':  # macOS
        name = 'BatteryTester'
        # macOS needs .icns format - can convert from .ico if needed
        icon_arg = ['--icon=icon.icns'] if os.path.exists('icon.icns') else []
        windowed = '--windowed'
    else:  # Linux
        name = 'BatteryTester'
        icon_arg = ['--icon=icon.ico'] if os.path.exists('icon.ico') else []
        windowed = '--windowed'  # Still use windowed for GUI
    
    # PyInstaller command
    cmd = [
        'pyinstaller',
        '--onefile',           # Single executable file
        windowed,              # No console window (GUI app)
        '--name', name,        # Executable name
        '--clean',             # Clean PyInstaller cache
        
        # Add local packages
        '--paths', '.',        # Add current directory to Python path
        
        # Add data files (config.ini.example as a template)
        '--add-data', f'config.ini.example{os.pathsep}.',
        
        # Hidden imports for local modules
        '--hidden-import', 'gui',
        '--hidden-import', 'gui.main_window',
        '--hidden-import', 'gui.dialogs',
        '--hidden-import', 'core',
        '--hidden-import', 'core.models',
        '--hidden-import', 'utils',
        '--hidden-import', 'utils.config_manager',
        '--hidden-import', 'utils.export_utils',
        
        # Hidden imports (PyQt6 modules)
        '--hidden-import', 'PyQt6.QtCore',
        '--hidden-import', 'PyQt6.QtGui',
        '--hidden-import', 'PyQt6.QtWidgets',
        '--hidden-import', 'pyqtgraph',
        '--hidden-import', 'serial',
        '--hidden-import', 'pandas',
        '--hidden-import', 'numpy',
        
        # Entry point
        'main.py'
    ]
    
    # Add icon if provided
    cmd.extend(icon_arg)
    
    print(f"\nBuilding executable for {platform}...")
    print(f"Command: {' '.join(cmd)}\n")
    
    # Run PyInstaller
    result = subprocess.run(cmd)
    
    if result.returncode == 0:
        print(f"\n✓ Build successful!")
        print(f"\nExecutable location: dist/{name}")
        # Automatically copy to release folder with versioned name
        import re
        version = None
        # Try to extract version from gui/main_window.py
        try:
            with open('gui/main_window.py', 'r', encoding='utf-8') as f:
                for line in f:
                    m = re.match(r'\s*VERSION\s*=\s*"([^"]+)"', line)
                    if m:
                        version = m.group(1).split()[0].lstrip('v')
                        break
        except Exception as e:
            print(f"[WARN] Could not extract version: {e}")
        if version:
            release_dir = Path('release')
            release_dir.mkdir(exist_ok=True)
            dest = release_dir / f"BatteryTester_v{version}.exe"
            shutil.copy2(f"dist/{name}.exe", dest)
            print(f"\n✓ Copied to {dest}")
        else:
            print("[WARN] Could not determine version for release copy.")
        print(f"\nTo distribute:")
        print(f"  1. Use the file in the release folder")
        print(f"  2. Ensure config.ini is created (will use config.ini.example as template)")
        print(f"  3. Run the executable")
        return True
    else:
        print(f"\n✗ Build failed with return code {result.returncode}")
        return False


def main():
    """Main build process"""
    print("=" * 60)
    print("Battery Tester - Executable Builder")
    print("=" * 60)
    print()
    
    # Check environment
    if not check_dependencies():
        return 1
    
    # Clean old builds
    response = input("\nClean old build folders? (y/n): ").strip().lower()
    if response == 'y':
        clean_build_folders()
    
    print()
    
    # Build
    if build_executable():
        return 0
    else:
        return 1


if __name__ == '__main__':
    sys.exit(main())
