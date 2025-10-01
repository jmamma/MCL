import os
import re
import shutil
import subprocess
import configparser
from SCons.Script import DefaultEnvironment

# --- Script Header ---
# This script customizes the PlatformIO build process.
# 1. Calculates and embeds a 16-bit checksum into the final ELF file.
# 2. Creates/validates a 'manifest.ini' file containing a full build manifest
#    (checksum, size, version string, version code, Git commit ID, and filename).
# 3. For 'avr', regenerates the HEX file to include only specific sections.
# 4. Renames the final firmware artifact based on VERSION_STR and copies it to
#    a clean './build/{env_name}/' directory.

# =================================================================
# Configuration
# =================================================================

env = DefaultEnvironment()

ENV_MAPPING = {
    "rp2040": "rp2040", "rp2350": "rp2040", "tbd": "rp2040",
    "avr": "avr", "megacmd": "avr", "megacommand": "avr", "nano": "avr",
}
FIRMWARE_EXTENSIONS = {
    "rp2040": ".uf2",
    "avr": ".hex"
}
CHECKSUM_MODE = os.environ.get("CHECKSUM_MODE", "validate")

# =================================================================
# Helper Functions
# =================================================================

def get_tool_path(env, tool_name):
    """Finds the full path to a toolchain executable like 'objdump'."""
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
    """Runs a command and captures its output."""
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
# Checksum and Manifest Logic
# =================================================================

def get_git_commit_id(env):
    """Gets the short git commit hash of the current HEAD."""
    try:
        project_dir = env.subst("$PROJECT_DIR")
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, check=True, cwd=project_dir
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "N/A"

def create_checksum_entry(env, env_name, checksum, firmware_size, version_str, version_code, git_commit_id, filename):
    """Creates or updates the checksum entry in build/manifest.ini"""
    project_dir = env.subst("$PROJECT_DIR")
    checksums_file = os.path.join(project_dir, "build", "manifest.ini")
    config = configparser.ConfigParser()

    if os.path.exists(checksums_file):
        config.read(checksums_file)
    
    config[env_name] = {
        'checksum': f'0x{checksum:04x}',
        'size': str(firmware_size),
        'version_string': version_str,
        'version_code': version_code,
        'commit': git_commit_id,
        'filename': filename
    }

    os.makedirs(os.path.dirname(checksums_file), exist_ok=True)
    with open(checksums_file, 'w') as f:
        config.write(f)

    print("\n" + "=" * 60)
    print("✓ CHECKSUM MANIFEST SAVED")
    print("=" * 60)
    print(f"Environment:    {env_name}")
    print(f"Version String: {version_str}")
    print(f"Version Code:   {version_code}")
    print(f"Commit:         {git_commit_id}")
    print(f"Checksum:       0x{checksum:04x}")
    print(f"Size:           {firmware_size} bytes")
    print(f"Filename:       {filename}")
    print(f"Manifest File:  {os.path.relpath(checksums_file, project_dir)}")
    print("=" * 60 + "\n")

def validate_checksum(env, env_name, checksum, firmware_size):
    """Validates the checksum against values stored in build/manifest.ini"""
    project_dir = env.subst("$PROJECT_DIR")
    checksums_file = os.path.join(project_dir, "build", "manifest.ini")
    config = configparser.ConfigParser()

    if not os.path.exists(checksums_file):
        print("\n" + "!" * 60)
        print("! ERROR: manifest.ini file not found!")
        print(f"! Expected at: {os.path.relpath(checksums_file, project_dir)}")
        print("! Run with CHECKSUM_MODE=create to generate it.")
        print("!" * 60 + "\n")
        return

    config.read(checksums_file)

    if env_name not in config:
        print("\n" + "!" * 60)
        print(f"! ERROR: No checksum entry found for '{env_name}' in manifest.ini")
        print("! Run with CHECKSUM_MODE=create to add this entry.")
        print("!" * 60 + "\n")
        return

    official_checksum = int(config[env_name].get('checksum', '0'), 0)
    official_size = int(config[env_name].get('size', '0'))
    official_version_str = config[env_name].get('version_string', 'N/A')
    official_version_code = config[env_name].get('version_code', 'N/A')
    official_commit = config[env_name].get('commit', 'N/A')

    checksum_matches = (checksum == official_checksum)
    size_matches = (firmware_size == official_size)

    if checksum_matches and size_matches:
        print("\n" + "=" * 60)
        print("✓ CHECKSUM VALIDATION PASSED")
        print("=" * 60)
        print(f"Environment:    {env_name}")
        print(f"Version String: {official_version_str} (official)")
        print(f"Version Code:   {official_version_code} (official)")
        print(f"Commit:         {official_commit} (official)")
        print(f"Checksum:       0x{checksum:04x} (matches)")
        print(f"Size:           {firmware_size} bytes (matches)")
        print("=" * 60 + "\n")
    else:
        print("\n" + "!" * 60 + "\n" + "!" * 60)
        print("!!!  CHECKSUM VALIDATION FAILED  !!!")
        print("!" * 60 + "\n" + "!" * 60)
        print(f"\nEnvironment: {env_name}")
        print(f"Official build is based on Version '{official_version_str}' (Code: {official_version_code}), Commit '{official_commit}'\n")

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
        print("!" * 60 + "\n" + "!" * 60 + "\n")
        # env.Exit(1)

# =================================================================
# Checksum Calculation and Embedding (Platform-Agnostic)
# =================================================================

def calculate_and_embed_checksum(elf_file, env, platform_family):
    """Calculates and embeds the checksum into the ELF file."""
    print("--- Running Checksum Calculation ---")
    checksum_section_name = ".firmware_checksum"
    objdump = get_tool_path(env, "objdump")
    cmd = [objdump, "-h", elf_file]
    result = run_command_for_output(cmd, env)

    section_offset = -1
    section_size = -1
    sections = []
    checksum_regex = re.compile(r"^\s*\d+\s+" + re.escape(checksum_section_name) + r"\s+([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)")
    section_regex = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)")

    for line in result.stdout.splitlines():
        match = checksum_regex.search(line)
        if match:
            section_size = int(match.group(1), 16)
            section_offset = int(match.group(2), 16)
        section_match = section_regex.search(line)
        if section_match:
            sections.append({ 'name': section_match.group(1), 'size': int(section_match.group(2), 16), 'vma': int(section_match.group(3), 16), 'lma': int(section_match.group(4), 16), 'file_offset': int(section_match.group(5), 16) })

    if section_offset == -1:
        print(f"✗ Error: Section '{checksum_section_name}' not found in the ELF file.")
        env.Exit(1)
    if section_size != 2:
        print(f"✗ Error: The size of '{checksum_section_name}' must be 2 bytes, but it is {section_size} bytes.")
        env.Exit(1)

    sections_to_checksum = []
    PROG_SECTIONS = ['.text', '.data', '.rodata']
    for section in sections:
        if any(section['name'].startswith(s) for s in PROG_SECTIONS) and section['size'] > 0:
            sections_to_checksum.append(section)

    if not sections_to_checksum:
        print(f"✗ Error: No program sections ({', '.join(PROG_SECTIONS)}) found.")
        env.Exit(1)
    
    sort_key = 'lma' if platform_family == "avr" else 'vma'
    sections_to_checksum.sort(key=lambda s: s[sort_key])

    firmware_size = 0
    if sections_to_checksum:
        last_section = sections_to_checksum[-1]
        if platform_family == "avr":
            firmware_size = last_section['lma'] + last_section['size']
        else:
            first_section = sections_to_checksum[0]
            firmware_size = (last_section['vma'] + last_section['size']) - first_section['vma']

    objcopy = get_tool_path(env, "objcopy")
    all_data = bytearray()
    for section in sections_to_checksum:
        section_bin_file = elf_file + f".{section['name']}.bin"
        cmd = [objcopy, "-O", "binary", "-j", section['name'], elf_file, section_bin_file]
        run_command_for_output(cmd, env)
        with open(section_bin_file, "rb") as f:
            all_data.extend(f.read())
        os.remove(section_bin_file)

    checksum = 0
    for i in range(0, len(all_data), 2):
        word = (all_data[i+1] << 8) | all_data[i] if i + 1 < len(all_data) else all_data[i]
        checksum = (checksum + word) & 0xFFFF

    print("\n┌" + "─" * 48 + "┐")
    print(f"│ Firmware checksum: 0x{checksum:04x}                     │")
    print("└" + "─" * 48 + "┘")
    
    with open(elf_file, "r+b") as f:
        f.seek(section_offset)
        f.write(checksum.to_bytes(2, byteorder='little'))
    
    print(f"✓ Successfully embedded checksum 0x{checksum:04x} into {os.path.basename(elf_file)}")
    print("--------------------------------------")
    return checksum, firmware_size

# =================================================================
# Firmware Parsing and Generation
# =================================================================

def get_version_str(env):
    """Parses CPPDEFINES to find and return the VERSION_STR value."""
    for define in env.get("CPPDEFINES", []):
        if isinstance(define, tuple) and define[0] == "VERSION_STR":
            value = define[1]
            return value.strip('"\\') if isinstance(value, str) else str(value)
    return None

def get_version_code(env):
    """Parses CPPDEFINES to find and return the VERSION value."""
    for define in env.get("CPPDEFINES", []):
        if isinstance(define, tuple) and define[0] == "VERSION":
            return str(define[1])
    return "N/A"

def regenerate_hex_for_avr(elf_file, env, new_hex_filename):
    """(AVR-Specific) Generates a custom .hex file."""
    print("--- Regenerating HEX File for Upload (AVR Specific) ---")
    objcopy = env.subst("$OBJCOPY")
    build_dir = env.subst("$BUILD_DIR")
    hex_file_path = os.path.join(build_dir, new_hex_filename)
    
    cmd = f'"{objcopy}" -O ihex -j .text -j .firmware_checksum -j .data "{elf_file}" "{hex_file_path}"'
    if env.Execute(cmd) == 0:
        print(f"✓ Custom HEX file '{new_hex_filename}' generated successfully!")
        print("--------------------------------------")
        return hex_file_path
    else:
        print(f"✗ Failed to generate custom HEX file.")
        env.Exit(1)

def copy_and_rename_firmware(env, env_name, source_firmware_path, new_firmware_name):
    """Copies the final firmware artifact to the ./build/{env_name}/ directory."""
    print("--- Copying and Renaming Final Firmware ---")
    if not os.path.exists(source_firmware_path):
        print(f"✗ Error: Source firmware file not found at '{source_firmware_path}'")
        return

    project_dir = env.subst("$PROJECT_DIR")
    dest_dir = os.path.join(project_dir, "build", env_name)
    os.makedirs(dest_dir, exist_ok=True)
    
    dest_file_path = os.path.join(dest_dir, new_firmware_name)
    shutil.copy2(source_firmware_path, dest_file_path)
    
    print(f"✓ Firmware copied successfully!")
    print(f"  From: {os.path.relpath(source_firmware_path, project_dir)}")
    print(f"  To:   {os.path.relpath(dest_file_path, project_dir)}")
    print("--------------------------------------")

# =================================================================
# Main Callback Function and Action Registration
# =================================================================

def combined_post_build_actions(source, target, env):
    """The main callback function that orchestrates all post-build steps."""
    print("\n--- Starting Custom Post-Build Actions ---")
    print(f"CHECKSUM_MODE: {CHECKSUM_MODE}")
    
    env_name = env.subst("$PIOENV")
    platform_family = ENV_MAPPING.get(env_name)

    if not platform_family:
        print(f"✗ Warning: Env '{env_name}' not in ENV_MAPPING. Skipping custom actions.")
        return

    elf_file = env.subst("$PROGPATH")
    print(f"Detected environment '{env_name}', mapped to platform '{platform_family}'.")

    # Step 1: Calculate and embed the checksum.
    checksum, firmware_size = calculate_and_embed_checksum(elf_file, env, platform_family)

    # Step 2: Get versioning info for the manifest.
    version_str = get_version_str(env) or "N/A"
    version_code = get_version_code(env)
    git_commit_id = get_git_commit_id(env)
    
    # Step 3: Determine the final firmware name *before* creating the manifest.
    if version_str == "N/A":
        print("⚠ Warning: -DVERSION_STR not found. Skipping firmware rename & copy.")
        new_firmware_name = "firmware" + FIRMWARE_EXTENSIONS.get(platform_family, ".bin")
    else:
        transformed_version = '_'.join(filter(str.isalnum, version_str)).lower()
        firmware_ext = FIRMWARE_EXTENSIONS.get(platform_family, ".bin")
        new_firmware_name = f"mcl_{transformed_version}{firmware_ext}"
    
    print(f"✓ Target filename will be '{new_firmware_name}'")

    # Step 4: Create or validate checksum manifest.
    if CHECKSUM_MODE.lower() == "create":
        create_checksum_entry(env, env_name, checksum, firmware_size, version_str, version_code, git_commit_id, new_firmware_name)
    else:
        validate_checksum(env, env_name, checksum, firmware_size)

    # Step 5: Generate, rename, and copy the final firmware artifact.
    if version_str == "N/A":
        print("--- Finished Custom Post-Build Actions (skipped copy due to missing version) ---")
        return

    source_firmware_path = os.path.splitext(elf_file)[0] + firmware_ext
    if platform_family == "avr":
        source_firmware_path = regenerate_hex_for_avr(elf_file, env, elf_file)

    if source_firmware_path:
        copy_and_rename_firmware(env, env_name, source_firmware_path, new_firmware_name)

    print("--- Finished Custom Post-Build Actions ---")

# Register the post-build action after the final program is linked.
env.AddPostAction("$PROGPATH", combined_post_build_actions)

print(f"✓ Registered multi-platform post-build action (Mode: {CHECKSUM_MODE}).")
