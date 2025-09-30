import os
import re
import shutil
import subprocess
import configparser
from SCons.Script import DefaultEnvironment

# --- Script Header ---
# This script customizes the PlatformIO build process for multiple platforms.
# 1. After any build, it calculates and embeds a 16-bit checksum into the final ELF file.
# 2. It uses an environment mapping to identify the platform family (e.g., "avr", "rp2040").
# 3. If the platform family is "avr", it then regenerates the final HEX file to include
#    only specific sections. For all other platforms, this step is skipped.
# 4. Based on CHECKSUM_MODE flag, either creates or validates checksums.

# =================================================================
# Configuration Flag
# =================================================================
# Set this to "create" to generate/update checksums.ini
# Set to "validate" to verify builds against checksums.ini

env = DefaultEnvironment()
env_mapping = {
        "rp2040": "rp2040", "rp2350": "rp2040", "tbd": "rp2040",
        "avr": "avr", "megacmd": "avr", "megacommand": "avr", "nano": "avr",
}

CHECKSUM_MODE = os.environ.get("CHECKSUM_MODE", "validate")  # Default to validate

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
# Checksum Creation Logic
# =================================================================

def create_checksum_entry(env_name, checksum, firmware_size):
    """
    Creates or updates the checksum entry in checksums.ini
    """
    checksums_file = "checksums.ini"
    config = configparser.ConfigParser()

    # Read existing file if it exists
    if os.path.exists(checksums_file):
        config.read(checksums_file)
        print(f"\n✓ Found existing checksums.ini")
    else:
        print(f"\n✓ Creating new checksums.ini")

    # Check if entry exists
    if env_name in config:
        old_checksum = config[env_name].get('checksum', '0x0000')
        old_size = config[env_name].get('size', '0')
        print(f"\n⚠ Updating existing entry for '{env_name}':")
        print(f"  Old checksum: {old_checksum} → New: 0x{checksum:04x}")
        print(f"  Old size:     {old_size} → New: {firmware_size} bytes")
    else:
        print(f"\n✓ Creating new entry for '{env_name}'")

    # Update or create entry
    config[env_name] = {
        'checksum': f'0x{checksum:04x}',
        'size': str(firmware_size)
    }

    # Write to file
    with open(checksums_file, 'w') as f:
        config.write(f)

    print(f"\n" + "=" * 60)
    print(f"✓ CHECKSUM ENTRY SAVED")
    print("=" * 60)
    print(f"Environment: {env_name}")
    print(f"Checksum:    0x{checksum:04x}")
    print(f"Size:        {firmware_size} bytes")
    print(f"File:        {checksums_file}")
    print("=" * 60 + "\n")

# =================================================================
# Checksum Validation Logic
# =================================================================

def validate_checksum(env_name, checksum, firmware_size):
    """
    Validates the calculated checksum against official values stored in checksums.ini
    """
    checksums_file = "checksums.ini"
    config = configparser.ConfigParser()

    # Check if checksums file exists
    if not os.path.exists(checksums_file):
        print("\n" + "!" * 60)
        print("! ERROR: checksums.ini file not found!")
        print("! Cannot validate without reference checksums.")
        print("! Run with CHECKSUM_MODE=create to generate checksums.ini")
        print("!" * 60 + "\n")
        return

    # Read existing checksums file
    config.read(checksums_file)

    # Check if this environment has an entry
    if env_name not in config:
        print("\n" + "!" * 60)
        print(f"! ERROR: No checksum entry found for '{env_name}'")
        print("! Cannot validate without reference checksum.")
        print("! Run with CHECKSUM_MODE=create to add this environment")
        print("!" * 60 + "\n")
        return

    # Get official values
    official_checksum_str = config[env_name].get('checksum', '0x0000')
    official_size_str = config[env_name].get('size', '0')

    # Parse official checksum (handles both 0x1234 and 1234 formats)
    if official_checksum_str.startswith('0x'):
        official_checksum = int(official_checksum_str, 16)
    else:
        official_checksum = int(official_checksum_str)

    official_size = int(official_size_str)

    # Compare values
    checksum_matches = (checksum == official_checksum)
    size_matches = (firmware_size == official_size)

    if checksum_matches and size_matches:
        print("\n" + "=" * 60)
        print("✓ CHECKSUM VALIDATION PASSED")
        print("=" * 60)
        print(f"Environment: {env_name}")
        print(f"Checksum:    0x{checksum:04x} (matches official)")
        print(f"Size:        {firmware_size} bytes (matches official)")
        print("=" * 60 + "\n")
    else:
        print("\n" + "!" * 60)
        print("!" * 60)
        print("!!!  CHECKSUM VALIDATION FAILED  !!!")
        print("!" * 60)
        print("!" * 60)
        print(f"\nEnvironment: {env_name}\n")

        if not checksum_matches:
            print(f"❌ CHECKSUM MISMATCH:")
            print(f"   Expected (official): 0x{official_checksum:04x}")
            print(f"   Got (compiled):      0x{checksum:04x}")
        else:
            print(f"✓  Checksum matches:     0x{checksum:04x}")

        if not size_matches:
            print(f"\n❌ SIZE MISMATCH:")
            print(f"   Expected (official): {official_size} bytes")
            print(f"   Got (compiled):      {firmware_size} bytes")
            print(f"   Difference:          {firmware_size - official_size:+d} bytes")
        else:
            print(f"\n✓  Size matches:         {firmware_size} bytes")

        print("\n" + "!" * 60)
        print("! This build does NOT match the official firmware!")
        print("! Possible causes:")
        print("!   - Code modifications")
        print("!   - Different compiler version")
        print("!   - Different optimization settings")
        print("!   - Different library versions")
        print("!" * 60)
        print("!" * 60 + "\n")

        # Optional: Uncomment to make build fail on mismatch
        # env.Exit(1)

# =================================================================
# Checksum Calculation and Embedding Logic (Platform-Agnostic)
# =================================================================

def calculate_and_embed_checksum(elf_file, env, platform_family):
    """
    Calculates the checksum of the ELF file and embeds it into the '.firmware_checksum' section.
    Checksum is calculated across all code/data sections (e.g. .text, .data, .rodata).
    Returns the checksum and firmware size for validation/creation.
    """
    print("--- Running Checksum Calculation ---")
    checksum_section_name = ".firmware_checksum"
    objdump = get_tool_path(env, "objdump")
    print(f"Reading section info from: {os.path.basename(elf_file)}")
    cmd = [objdump, "-h", elf_file]
    result = run_command_for_output(cmd, env)

    section_offset = -1
    section_size = -1
    firmware_checksum_vma = -1

    sections = []
    checksum_regex = re.compile(r"^\s*\d+\s+" + re.escape(checksum_section_name) + r"\s+([0-9a-f]+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)")
    section_regex = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)")

    for line in result.stdout.splitlines():
        match = checksum_regex.search(line)
        if match:
            section_size = int(match.group(1), 16)
            firmware_checksum_vma = int(match.group(2), 16)
            section_offset = int(match.group(3), 16)
            print(f"Found .firmware_checksum: VMA=0x{firmware_checksum_vma:08x}, size={section_size}, offset=0x{section_offset:x}")

        section_match = section_regex.search(line)
        if section_match:
            sections.append({
                'name': section_match.group(1),
                'size': int(section_match.group(2), 16),
                'vma': int(section_match.group(3), 16),
                'lma': int(section_match.group(4), 16),
                'file_offset': int(section_match.group(5), 16)
            })

    if section_offset == -1:
        print(f"✗ Error: Section '{checksum_section_name}' not found in the ELF file.")
        env.Exit(1)
    if section_size != 2:
        print(f"✗ Error: The size of '{checksum_section_name}' must be 2 bytes (uint16_t), but it is {section_size} bytes.")
        env.Exit(1)

    # --- MODIFIED LOGIC START ---
    # Identify sections that are part of the program image stored in flash.
    sections_to_checksum = []
    PROG_SECTIONS = ['.text', '.data', '.rodata']

    print("\n--- Identifying sections for checksum ---")
    for section in sections:
        if section['name'] in PROG_SECTIONS and section['size'] > 0:
            sections_to_checksum.append(section)

    if not sections_to_checksum:
        print(f"✗ Error: No program sections ({', '.join(PROG_SECTIONS)}) with content found.")
        env.Exit(1)

    # Sort sections by their load address to ensure a consistent order for checksumming.
    # For AVR, LMA (Load Memory Address) is the location in Flash.
    # For ARM/others, VMA (Virtual Memory Address) is typically the Flash address.
    if platform_family == "avr":
        sections_to_checksum.sort(key=lambda s: s['lma'])
        print("\n--- Sections to be included in checksum (sorted by LMA for AVR) ---")
    else:
        sections_to_checksum.sort(key=lambda s: s['vma'])
        print("\n--- Sections to be included in checksum (sorted by VMA) ---")

    for section in sections_to_checksum:
        print(f"  ✓ {section['name']:<20} (LMA=0x{section['lma']:08x}, VMA=0x{section['vma']:08x}, Size={section['size']})")

    # Calculate firmware size for validation. This should represent the
    # total space occupied by the included sections in flash.
    firmware_size = 0
    if platform_family == "avr":
        if sections_to_checksum:
            # The size is determined by the end address of the last section in flash (LMA).
            last_section = sections_to_checksum[-1]
            firmware_size = last_section['lma'] + last_section['size']
            print(f"\nPlatform: AVR - Firmware size determined by end of last section (LMA): {firmware_size} bytes")
    else:
        if sections_to_checksum:
            # For other platforms, it's the span from the start of the first section
            # to the end of the last one. Assumes they are contiguous.
            first_section = sections_to_checksum[0]
            last_section = sections_to_checksum[-1]
            firmware_size = (last_section['vma'] + last_section['size']) - first_section['vma']
            print(f"\nPlatform: {platform_family} - Firmware size determined by VMA span: {firmware_size} bytes")
    # --- MODIFIED LOGIC END ---

    # Extract binary data from each section and concatenate
    objcopy = get_tool_path(env, "objcopy")
    all_data = bytearray()

    for section in sections_to_checksum:
        section_bin_file = elf_file + f".{section['name']}.bin"
        cmd = [objcopy, "-O", "binary", "-j", section['name'], elf_file, section_bin_file]
        print(f"Extracting {section['name']} section...")
        run_command_for_output(cmd, env)

        with open(section_bin_file, "rb") as f:
            all_data.extend(f.read())
        os.remove(section_bin_file)

    print(f"\nTotal data for checksum: {len(all_data)} bytes")

    # Calculate checksum over all extracted data
    checksum = 0
    for i in range(0, len(all_data), 2):
        if i + 1 < len(all_data):
            word = (all_data[i+1] << 8) | all_data[i]
        else:
            word = all_data[i]
        checksum = (checksum + word) & 0xFFFF

    print("┌" + "─" * 48 + "┐")
    print(f"│ Firmware checksum: 0x{checksum:04x}                     │")
    print("└" + "─" * 48 + "┘")

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

    print(f"✓ Successfully embedded checksum 0x{checksum:04x} into {os.path.basename(elf_file)}")
    print("--------------------------------------")

    return checksum, firmware_size


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
    print(f"CHECKSUM_MODE: {CHECKSUM_MODE}")
    
    env_name = env.subst("$PIOENV")
    platform_family = env_mapping.get(env_name)

    if platform_family == "avr":
        elf_file = str(source[0])
    else:
        elf_file = str(target[0])

    print(f"Detected environment '{env_name}', mapped to platform family '{platform_family}'.")

    # Step 1: Calculate and embed the checksum. This is done for ALL platforms.
    # --- MODIFIED: Pass platform_family to the function ---
    checksum, firmware_size = calculate_and_embed_checksum(elf_file, env, platform_family)

    # Step 2: Create or validate checksum based on mode
    if CHECKSUM_MODE.lower() == "create":
        create_checksum_entry(env_name, checksum, firmware_size)
    else:
        validate_checksum(env_name, checksum, firmware_size)

    # Step 3: Regenerate the HEX file ONLY for the 'avr' platform family.
    if platform_family == "avr":
        print(f"Platform family is '{platform_family}', proceeding with HEX file regeneration.")
        hex_file = os.path.splitext(elf_file)[0] + ".hex"
        regenerate_hex(elf_file, hex_file, env)

    print("--- Finished Custom Post-Build Actions ---")

env_name = env.subst("$PIOENV")
platform_family = env_mapping.get(env_name)

if platform_family == "avr":
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", combined_post_build_actions)
else:
    env.AddPostAction("$PROGPATH", combined_post_build_actions)

print(f"✓ Registered multi-platform checksum and post-build action (Mode: {CHECKSUM_MODE}).")
