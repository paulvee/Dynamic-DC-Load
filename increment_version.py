#!/usr/bin/env python3
"""
Auto-increment firmware version on every build
Increments the letter suffix (a->b->c->...->z)
"""
import re
Import("env")

config_path = "include/Config.h"

def increment_version(source, target, env):
    """Increment version letter suffix before build"""
    print("Incrementing version...")
    
    with open(config_path, 'r') as f:
        content = f.read()
    
    # Find version string pattern: "X.Y.Z" or "X.Y.Za" where a is a letter
    version_pattern = r'(const String FW_VERSION = "(\d+\.\d+\.\d+)([a-z]?)";)'
    match = re.search(version_pattern, content)
    
    if match:
        full_match = match.group(1)
        base_version = match.group(2)  # e.g., "7.0.3"
        current_suffix = match.group(3)  # e.g., "a" or ""
        
        # Increment suffix
        if not current_suffix:
            new_suffix = 'a'
        elif current_suffix == 'z':
            new_suffix = 'a'  # Wrap around
            print("  WARNING: Version wrapped from 'z' back to 'a'")
        else:
            new_suffix = chr(ord(current_suffix) + 1)
        
        new_version_line = f'const String FW_VERSION = "{base_version}{new_suffix}";'
        new_content = content.replace(full_match, new_version_line)
        
        # Also update @version in file header using a callback to avoid backreference issues
        header_pattern = r'(@version )(\d+\.\d+\.\d+[a-z]?)'
        def replace_version(match):
            return match.group(1) + base_version + new_suffix
        new_content = re.sub(header_pattern, replace_version, new_content)
        
        with open(config_path, 'w') as f:
            f.write(new_content)
        
        print(f"  Version: {base_version}{current_suffix} -> {base_version}{new_suffix}")
    else:
        print("  ERROR: Could not find version string in Config.h")

# Register pre-build action
env.AddPreAction("buildprog", increment_version)
