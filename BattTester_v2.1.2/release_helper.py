#!/usr/bin/env python3
"""
Release Helper Script for Battery Tester

Automates the process of:
1. Building executable
2. Preparing release assets
3. Creating GitHub release (with gh CLI)

Usage:
    python release_helper.py v2.0.2
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
from datetime import datetime


def check_gh_cli():
    """Check if GitHub CLI is installed"""
    try:
        result = subprocess.run(['gh', '--version'], capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✓ GitHub CLI found: {result.stdout.split()[2]}")
            return True
    except FileNotFoundError:
        pass
    
    print("✗ GitHub CLI not found")
    print("\nTo use automated releases, install GitHub CLI:")
    print("  https://cli.github.com/")
    print("\nOr create release manually - see GITHUB_RELEASE_GUIDE.md")
    return False


def check_authenticated():
    """Check if user is authenticated with GitHub CLI"""
    try:
        result = subprocess.run(['gh', 'auth', 'status'], capture_output=True, text=True)
        if result.returncode == 0:
            print("✓ Authenticated with GitHub")
            return True
    except:
        pass
    
    print("✗ Not authenticated with GitHub")
    print("\nRun: gh auth login")
    return False


def build_executable():
    """Build the Windows executable"""
    print("\n" + "="*60)
    print("Building Windows Executable")
    print("="*60)
    
    if not Path('build_executable.py').exists():
        print("✗ build_executable.py not found")
        return False
    
    # Run build script (will prompt for clean)
    result = subprocess.run([sys.executable, 'build_executable.py'])
    
    if result.returncode == 0 and Path('dist/BatteryTester.exe').exists():
        print("\n✓ Build successful")
        return True
    else:
        print("\n✗ Build failed")
        return False


def prepare_release_asset(version):
    """Copy and rename executable for release"""
    print(f"\nPreparing release asset for {version}...")
    
    source = Path('dist/BatteryTester.exe')
    if not source.exists():
        print(f"✗ {source} not found")
        return None
    
    # Create releases folder if it doesn't exist (local only, ignored by git)
    releases_dir = Path('releases')
    releases_dir.mkdir(exist_ok=True)
    
    # Copy with version in filename
    dest = releases_dir / f'BatteryTester-Windows-{version}.exe'
    shutil.copy2(source, dest)
    
    size_mb = dest.stat().st_size / (1024 * 1024)
    print(f"✓ Created: {dest}")
    print(f"  Size: {size_mb:.1f} MB")
    
    return dest


def create_github_release(version, asset_path):
    """Create GitHub release using gh CLI"""
    print(f"\nCreating GitHub release {version}...")
    
    if not Path('RELEASE_NOTES.md').exists():
        print("⚠ RELEASE_NOTES.md not found - using default notes")
        notes = f"Battery Tester {version} - Windows Release"
        notes_arg = ['--notes', notes]
    else:
        notes_arg = ['--notes-file', 'RELEASE_NOTES.md']
    
    cmd = [
        'gh', 'release', 'create', version,
        str(asset_path),
        '--title', f'Battery Tester {version} - Windows Release'
    ] + notes_arg
    
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd)
    
    if result.returncode == 0:
        print(f"\n✓ Release {version} created successfully!")
        print(f"\nView at: https://github.com/paulvee/Battery-tester-Python/releases/tag/{version}")
        print(f"Download link: https://github.com/paulvee/Battery-tester-Python/releases/download/{version}/{asset_path.name}")
        return True
    else:
        print(f"\n✗ Failed to create release")
        return False


def main():
    """Main release workflow"""
    print("="*60)
    print("Battery Tester - Release Helper")
    print("="*60)
    
    # Check version argument
    if len(sys.argv) < 2:
        print("\nUsage: python release_helper.py VERSION")
        print("\nExample:")
        print("  python release_helper.py v2.0.2")
        print("\nVersion must start with 'v' (e.g., v2.0.1, v2.1.0)")
        return 1
    
    version = sys.argv[1]
    
    # Validate version format
    if not version.startswith('v'):
        print(f"\n✗ Version must start with 'v' (got: {version})")
        print("  Example: v2.0.1")
        return 1
    
    print(f"\nPreparing release: {version}")
    print(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Check if GitHub CLI is available
    has_gh = check_gh_cli()
    if has_gh:
        if not check_authenticated():
            has_gh = False
    
    print()
    
    # Step 1: Build executable
    response = input("Build new executable? (y/n): ").strip().lower()
    if response == 'y':
        if not build_executable():
            print("\n✗ Cannot continue without successful build")
            return 1
    else:
        if not Path('dist/BatteryTester.exe').exists():
            print("\n✗ dist/BatteryTester.exe not found")
            print("  Build executable first: python build_executable.py")
            return 1
    
    # Step 2: Prepare release asset
    asset_path = prepare_release_asset(version)
    if not asset_path:
        return 1
    
    # Step 3: Create GitHub release
    if has_gh:
        print()
        response = input(f"Create GitHub release {version}? (y/n): ").strip().lower()
        if response == 'y':
            if create_github_release(version, asset_path):
                print("\n🎉 Release complete!")
                return 0
            else:
                print("\n⚠ Release created locally but failed to upload to GitHub")
                print(f"  Asset ready at: {asset_path}")
                print("  Create release manually - see GITHUB_RELEASE_GUIDE.md")
                return 1
        else:
            print("\n✓ Asset prepared successfully")
            print(f"  Location: {asset_path}")
            print("\nTo create release manually:")
            print("  1. Go to: https://github.com/paulvee/Battery-tester-Python/releases/new")
            print(f"  2. Tag: {version}")
            print(f"  3. Upload file: {asset_path}")
            return 0
    else:
        print("\n✓ Asset prepared successfully")
        print(f"  Location: {asset_path}")
        print("\nCreate GitHub release manually - see GITHUB_RELEASE_GUIDE.md")
        print("Or install GitHub CLI: https://cli.github.com/")
        return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\nAborted by user")
        sys.exit(1)
