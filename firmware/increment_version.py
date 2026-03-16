#!/usr/bin/env python3
"""
Auto-increment firmware version on every build
Increments the patch number (X.Y.Z -> X.Y.Z+1)
"""
import re
Import("env")

config_path = "include/Config.h"

def increment_version(source, target, env):
    """Increment patch version number before build"""
    print("Incrementing version...")
    
    with open(config_path, 'r') as f:
        content = f.read()
    
    # Find version string pattern: "X.Y.Z"
    version_pattern = r'(const String FW_VERSION = "(\d+)\.(\d+)\.(\d+)";)'
    match = re.search(version_pattern, content)
    
    if match:
        full_match = match.group(1)
        major = int(match.group(2))  # e.g., 7
        minor = int(match.group(3))  # e.g., 1
        patch = int(match.group(4))  # e.g., 1
        
        # Increment patch number
        new_patch = patch + 1
        old_version = f"{major}.{minor}.{patch}"
        new_version = f"{major}.{minor}.{new_patch}"
        
        new_version_line = f'const String FW_VERSION = "{new_version}";'
        new_content = content.replace(full_match, new_version_line)
        
        # Also update @version in file header if present
        header_pattern = r'(@version )(\d+\.\d+\.\d+)'
        def replace_version(match):
            return match.group(1) + new_version
        new_content = re.sub(header_pattern, replace_version, new_content)
        
        with open(config_path, 'w') as f:
            f.write(new_content)
        
        print(f"  Version: {old_version} -> {new_version}")
    else:
        print("  ERROR: Could not find version string in Config.h")

# Register pre-build action
env.AddPreAction("buildprog", increment_version)
