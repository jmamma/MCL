import os
import re
import shutil
import subprocess
from SCons.Script import DefaultEnvironment

# --- Script Header ---
# This script customizes the PlatformIO build process for multiple platforms.
# 1. After any build, it calculates and embeds a 16-bit checksum into the final ELF file.
# 2. It uses an environment mapping to identify the platform family (e.g., "avr", "rp2040").
# 3. If the platform family is "avr", it then regenerates the final HEX file to include
#    only specific sections. For all other platforms, this step is skipped.

env = DefaultEnvironment()
env_mapping = {
        "rp2040": "rp2040", "rp2350": "rp2040", "tbd": "rp2040",
        "avr": "avr", "megacmd": "avr", "megacommand": "avr", "nano": "avr",
}

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
# Checksum Calculation and Embedding Logic (Platform-Agnostic)
# =================================================================

def calculate_and_embed_checksum(elf_file, env):
    """
    Calculates the checksum of the ELF file and embeds it into the '.firmware_checksum' section.
    """
    print("--- Running Checksum Calculation ---")
    checksum_section_name = ".firmware_checksum"

    objdump = get_tool_path(env, "objdump")
    print(f"Reading section info from: {os.path.basename(elf_file)}")
    cmd = [objdump, "-h", elf_file]
    result = run_command_for_output(cmd, env)

    section_offset = -1
    section_size = -1

    # ALSO get the .text section end to match C code behavior
    text_section_end = -1

    # Parse both .firmware_checksum and .text sections
    checksum_regex = re.compile(r"^\s*\d+\s+" + re.escape(checksum_section_name) + r"\s+([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)")
    text_regex = re.compile(r"^\s*\d+\s+\.text\s+([0-9a-f]+)\s+([0-9a-f]+)")

    for line in result.stdout.splitlines():
        #print(line)

        # Check for .firmware_checksum section
        match = checksum_regex.search(line)
        if match:
            section_size = int(match.group(1), 16)
            section_offset = int(match.group(2), 16)

        # Check for .text section to get its size
        text_match = text_regex.search(line)
        if text_match:
            text_size = int(text_match.group(1), 16)
            text_vma = int(text_match.group(2), 16)
            # For .text section that starts at 0, the end is just the size
            if text_vma == 0:
                text_section_end = text_size

    if section_offset == -1:
        print(f"✗ Error: Section '{checksum_section_name}' not found in the ELF file.")
        env.Exit(1)

    if text_section_end == -1:
        print(f"✗ Error: .text section not found in the ELF file.")
        env.Exit(1)

    print(f"Found section '{checksum_section_name}': size={section_size} bytes, file_offset={hex(section_offset)}")
    print(f"Found .text section end: {text_section_end} bytes (0x{text_section_end:x})")

    if section_size != 2:
        print(f"✗ Error: The size of '{checksum_section_name}' must be 2 bytes (uint16_t), but it is {section_size} bytes.")
        env.Exit(1)
    # Extract the .text section binary data using objcopy
    objcopy = get_tool_path(env, "objcopy")
    text_bin_file = elf_file + ".text.bin"
    # Extract .text section to binary file
    cmd = [objcopy, "-O", "binary", "-j", ".text", elf_file, text_bin_file]
    print(f"Extracting .text section: {' '.join(cmd)}")
    result = run_command_for_output(cmd, env)
    # Read the extracted .text section binary data
    with open(text_bin_file, "rb") as f:
        text_data = bytearray(f.read())
    # Clean up temporary file
    os.remove(text_bin_file)
    print(f"Extracted .text section: {len(text_data)} bytes")

    checksum_boundary = text_section_end

    checksum = 0
    for i in range(0, checksum_boundary, 2):
        if i + 1 < checksum_boundary:
            word = (text_data[i+1] << 8) | text_data[i]
        else:
            word = text_data[i]
        checksum = (checksum + word) & 0xFFFF

    print("┌" + "─" * 48 + "┐")
    print(f"│ Firmware checksum: | 0x{checksum:04x}")
    print("└" + "─" * 48 + "┘")

    print(f"text_section_end: {text_section_end}")
    print(f"Will checksum from 0 to {checksum_boundary}")
    print(f"Python calculated checksum: {checksum}")

    # Embed the checksum
    with open(elf_file, "r+b") as f:
        f.seek(section_offset)
        f.write(checksum.to_bytes(2, byteorder='little'))

    # Verify it was written correctly
    with open(elf_file, "rb") as f:
        f.seek(section_offset)
        data = f.read(2)
        written = int.from_bytes(data, byteorder="little")

    if written != checksum:
        print("✗ Error: Failed to write checksum correctly!")
        env.Exit(1)

    print(f"✓ Successfully embedded checksum {checksum} into {os.path.basename(elf_file)}")
    print("--------------------------------------")

# =================================================================
# Custom HEX File Generation (AVR-Specific)
# =================================================================

def regenerate_hex(elf_file, hex_file, env):
    """
    Generates a .hex file from a .elf file, including only .text and .data sections.
    """
    print("--- Regenerating HEX File for Upload (AVR Specific) ---")
    objcopy = env.subst("$OBJCOPY")
    cmd = f'"{objcopy}" -O ihex -j .text -j .firmware_checksum -j .data "{elf_file}" "{hex_file}"'
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
    # *** NEW: Use the environment name and mapping for platform detection ***
    env_name = env.subst("$PIOENV")
    # Look up the family, defaulting to None if the env is not in the map
    platform_family = env_mapping.get(env_name)

    # The 'target' of this action is the final ELF file ($PROGPATH).
    if platform_family == "avr":
      elf_file = str(source[0])
    else:
      elf_file = str(target[0])

    print(f"Detected environment '{env_name}', mapped to platform family '{platform_family}'.")

    # Step 1: Calculate and embed the checksum. This is done for ALL platforms.
    calculate_and_embed_checksum(elf_file, env)

    # Step 2: Regenerate the HEX file ONLY for the 'avr' platform family.
    if platform_family == "avr":
        print(f"Platform family is '{platform_family}', proceeding with HEX file regeneration.")
        # Derive the .hex file path from the .elf file path.
        hex_file = os.path.splitext(elf_file)[0] + ".hex"
        regenerate_hex(elf_file, hex_file, env)

    print("--- Finished Custom Post-Build Actions ---")

env_name = env.subst("$PIOENV")
platform_family = env_mapping.get(env_name)
# Register the action to be called after the final program is linked.
# $PROGPATH is the path to the final ELF file, which is platform-agnostic.
if platform_family == "avr":
  env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", combined_post_build_actions)
else:
  env.AddPostAction("$PROGPATH", combined_post_build_actions)

print("✓ Registered multi-platform checksum and post-build action.")
