import os
import re
import shutil
import subprocess
from SCons.Script import DefaultEnvironment

# --- Script Header ---
# This script customizes the PlatformIO build process.
# 1. After a build, it calculates a 16-bit checksum of the ELF file and embeds it back.
# 2. It then regenerates the final HEX file to include only .text and .data sections,
#    ensuring the updated checksum is included.

env = DefaultEnvironment()

# =================================================================
# Helper Functions for Tool Paths and Command Execution
# =================================================================

def get_tool_path(env, tool_name):
    """
    Finds the full path to a toolchain executable like 'objdump'.
    """
    cc_path = env.subst("$CC")
    toolchain_bin_dir = os.path.dirname(cc_path)
    cc_base = os.path.basename(cc_path)
    prefix = cc_base.replace("gcc", "").replace("clang", "")
    prefixed_tool_path = os.path.join(toolchain_bin_dir, f"{prefix}{tool_name}")
    if os.path.exists(prefixed_tool_path):
        return prefixed_tool_path

    fallback_path = shutil.which(f"{prefix}{tool_name}")
    if fallback_path:
        return fallback_path
    print(f"✗ Error: Could not find tool '{prefix}{tool_name}' in the toolchain or system PATH.")
    env.Exit(1)

def run_command_for_output(cmd, env):
    """
    Helper to run a command (like objdump) and capture its output for parsing.
    Uses subprocess directly for better output handling.
    """
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore', check=True
        )
        return result
    except subprocess.CalledProcessError as e:
        print(f"✗ Error running command: {' '.join(cmd)}")
        print("STDOUT:", e.stdout, sep='\n')
        print("STDERR:", e.stderr, sep='\n')
        env.Exit(1)

# =================================================================
# Checksum Calculation and Embedding Logic
# =================================================================

def calculate_and_embed_checksum(elf_file, env):
    """
    Calculates the checksum of the ELF file and embeds it into the '.firmware_checksum' section.
    """
    print("--- Running Checksum Calculation ---")
    checksum_section_name = ".firmware_checksum"

    # 1. Get path to objdump tool
    objdump = get_tool_path(env, "objdump")

    # 2. Use objdump to get section headers and find our checksum section's offset
    print(f"Reading section info from: {os.path.basename(elf_file)}")
    cmd = [objdump, "-h", elf_file]
    result = run_command_for_output(cmd, env)
    
    section_offset = -1
    section_size = -1
    regex = re.compile(r"^\s*\d+\s+" + re.escape(checksum_section_name) + r"\s+([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)")
    
    for line in result.stdout.splitlines():
        print(line)
        match = regex.search(line)
        if match:
            section_size = int(match.group(1), 16)
            section_offset = int(match.group(2), 16)
            break
    
    if section_offset == -1:
        print(f"✗ Error: Section '{checksum_section_name}' not found in the ELF file.")
        env.Exit(1)
        
    print(f"Found section '{checksum_section_name}': size={section_size} bytes, file_offset={hex(section_offset)}")

    if section_size != 2:
        print(f"✗ Error: The size of '{checksum_section_name}' must be 2 bytes (uint16_t), but it is {section_size} bytes.")
        env.Exit(1)

    # 3. Read ELF into memory
    with open(elf_file, "rb") as f:
        firmware_data = bytearray(f.read())

    # 4. *** NEW: Verify the placeholder value before proceeding ***
    placeholder_value = 0xDADA
    # Read the current 16-bit value from the file (little-endian)
    current_value = (firmware_data[section_offset + 1] << 8) | firmware_data[section_offset]
    
    print(f"Verifying placeholder at offset {hex(section_offset)}...")
    if current_value != placeholder_value:
        print(f"✗ Error: Expected placeholder value {hex(placeholder_value)} at checksum location, but found {hex(current_value)}.")
        print("  This might happen if the firmware was already patched or the C++ code is incorrect.")
        print("  Please ensure the source code contains: ... firmware_checksum = 0xDEAD;")
        env.Exit(1)
    
    print(f"✓ Placeholder {hex(placeholder_value)} verified successfully.")

    # 5. Zero out the checksum section for accurate calculation
    firmware_data[section_offset] = 0
    firmware_data[section_offset + 1] = 0

    # 6. Calculate the 16-bit checksum
    checksum = 0
    for i in range(0, len(firmware_data), 2):
        if i + 1 < len(firmware_data):
            word = (firmware_data[i+1] << 8) | firmware_data[i] # Little-endian
        else:
            word = firmware_data[i] # Handle odd-length firmware
        checksum = (checksum + word) & 0xFFFF # Ensure it remains 16-bit
    print(f"Calculated 16-bit checksum: {hex(checksum)}")

    # 7. Embed the new checksum back into the ELF file
    with open(elf_file, "r+b") as f:
        f.seek(section_offset)
        f.write(checksum.to_bytes(2, byteorder='little'))
    print(f"✓ Successfully embedded checksum into {os.path.basename(elf_file)}")
    print("--------------------------------------")

# =================================================================
# Custom HEX File Generation (Your Original Function)
# =================================================================

def regenerate_hex(elf_file, hex_file, env):
    """
    Generates a .hex file from a .elf file, including only .text and .data sections.
    """
    print("--- Regenerating HEX File for Upload ---")
    print(f"Source ELF: {elf_file}")
    print(f"Target HEX: {hex_file}")

    if not os.path.exists(elf_file):
        print(f"✗ Error: ELF file not found at '{elf_file}'. Skipping HEX regeneration.")
        env.Exit(1) 

    objcopy = env.subst("$OBJCOPY")
    if not objcopy:
        print("✗ Error: $OBJCOPY path not found in environment.")
        env.Exit(1)

    cmd = f'"{objcopy}" -O ihex -j .text -j .data "{elf_file}" "{hex_file}"'
    print(f"Running command: {cmd}")

    result = env.Execute(cmd)
    if result == 0:
        print(f"✓ Custom HEX file '{hex_file}' was generated successfully!")
    else:
        print(f"✗ Failed to generate custom HEX file. Exit code: {result}")
        env.Exit(result)
    print("--------------------------------------")
    return result

# =================================================================
# Main Callback Function and Action Registration
# =================================================================

def combined_post_build_actions(source, target, env):
    """
    The main callback function that orchestrates all post-build steps.
    """
    print("\n--- Starting Custom Post-Build Actions ---")

    # The 'target' of this SCons action is the default-generated .hex file.
    hex_file = str(target[0])
    # Derive the corresponding .elf file path from the .hex file path.
    elf_file = os.path.splitext(hex_file)[0] + ".elf"

    # Step 1: Calculate and embed the checksum into the ELF file.
    calculate_and_embed_checksum(elf_file, env)

    # Step 2: Regenerate the HEX file from the newly-patched ELF file.
    regenerate_hex(elf_file, hex_file, env)

# Register the combined action to be called after the default HEX file is built.
env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", combined_post_build_actions)
print("✓ Registered combined checksum and HEX regeneration post-build action.")
