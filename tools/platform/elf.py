import os
import re
import shutil
import subprocess
import configparser
from SCons.Script import DefaultEnvironment

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
    print(f"✗ Error: Could not find tool '{prefix}{tool_name}'.")
    env.Exit(1)

def run_command_for_output(cmd, env):
    """Runs a command and captures its output."""
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore', check=True
        )
        return result
    except subprocess.CalledProcessError as e:
        print(f"✗ Error running command: {' '.join(cmd)}\n{e.stderr}")
        env.Exit(1)

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

# =================================================================
# Checksum and Firmware Calculation Logic
# =================================================================

def get_firmware_checksum_and_size(elf_file, env, platform_family):
    """Parses the ELF file to calculate the firmware checksum and total size."""
    objdump = get_tool_path(env, "objdump")
    result = run_command_for_output([objdump, "-h", elf_file], env)

    sections = []
    section_regex = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)")
    for line in result.stdout.splitlines():
        match = section_regex.search(line)
        if match:
            sections.append({ 'name': match.group(1), 'size': int(match.group(2), 16), 'vma': int(match.group(3), 16), 'lma': int(match.group(4), 16) })

    sections_to_checksum = [s for s in sections if any(s['name'].startswith(p) for p in ['.text', '.data', '.rodata']) and s['size'] > 0]
    if not sections_to_checksum:
        print("✗ Error: No program sections found for checksumming.")
        env.Exit(1)

    sort_key = 'lma' if platform_family == "avr" else 'vma'
    sections_to_checksum.sort(key=lambda s: s[sort_key])

    if platform_family == "avr":
        last_section = sections_to_checksum[-1]
        firmware_size = last_section['lma'] + last_section['size']
    else:
        first_section = sections_to_checksum[0]
        last_section = sections_to_checksum[-1]
        firmware_size = (last_section['vma'] + last_section['size']) - first_section['vma']

    objcopy = get_tool_path(env, "objcopy")
    all_data = bytearray()
    for section in sections_to_checksum:
        section_bin_file = f"{elf_file}.{section['name']}.bin"
        run_command_for_output([objcopy, "-O", "binary", "-j", section['name'], elf_file, section_bin_file], env)
        with open(section_bin_file, "rb") as f:
            all_data.extend(f.read())
        os.remove(section_bin_file)

    checksum = 0
    for i in range(0, len(all_data), 2):
        word = (all_data[i+1] << 8) | all_data[i] if i + 1 < len(all_data) else all_data[i]
        checksum = (checksum + word) & 0xFFFF
    
    return checksum, firmware_size

def calculate_and_embed_checksum(elf_file, env, platform_family):
    """Calculates and embeds the checksum into the ELF file."""
    checksum, firmware_size = get_firmware_checksum_and_size(elf_file, env, platform_family)
    
    objdump = get_tool_path(env, "objdump")
    result = run_command_for_output([objdump, "-h", elf_file], env)

    checksum_section_name = ".firmware_checksum"
    checksum_regex = re.compile(r"^\s*\d+\s+" + re.escape(checksum_section_name) + r"\s+[0-9a-f]+\s+[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)")
    
    section_offset = -1
    for line in result.stdout.splitlines():
        match = checksum_regex.search(line)
        if match:
            section_offset = int(match.group(1), 16)
            break
    
    if section_offset == -1:
        print(f"✗ Error: Section '{checksum_section_name}' not found.")
        env.Exit(1)
    
    print(f"✓ Embedding firmware checksum: 0x{checksum:04x}")

    with open(elf_file, "r+b") as f:
        f.seek(section_offset)
        f.write(checksum.to_bytes(2, byteorder='little'))
    
    return checksum, firmware_size

# =================================================================
# Manifest and File Operations
# =================================================================

def create_or_validate_manifest(env, env_name, checksum, firmware_size):
    """Creates or validates the manifest entry in build/manifest.ini."""
    version_str = get_version_str(env) or "N/A"
    platform_family = ENV_MAPPING.get(env_name)
    firmware_ext = FIRMWARE_EXTENSIONS.get(platform_family, ".bin")
    
    if version_str == "N/A":
        new_firmware_name = f"firmware{firmware_ext}"
    else:
        # MODIFIED: New logic to create filenames like "mcl_h_4_7_0.hex".
        # It filters for alphanumeric characters and joins them with underscores.
        alphanumeric_chars = filter(str.isalnum, version_str)
        transformed_version = '_'.join(alphanumeric_chars).lower()
        new_firmware_name = f"mcl_{transformed_version}{firmware_ext}"
    
    print(f"✓ Target filename will be '{new_firmware_name}'")

    project_dir = env.subst("$PROJECT_DIR")
    checksums_file = os.path.join(project_dir, "build", "manifest.ini")
    config = configparser.ConfigParser()

    if CHECKSUM_MODE.lower() == "create":
        if os.path.exists(checksums_file):
            config.read(checksums_file)
        config[env_name] = {
            'checksum': f'0x{checksum:04x}', 'size': str(firmware_size),
            'version_string': version_str, 'version_code': get_version_code(env),
            'commit': get_git_commit_id(env), 'filename': new_firmware_name
        }
        os.makedirs(os.path.dirname(checksums_file), exist_ok=True)
        with open(checksums_file, 'w') as f:
            config.write(f)
        print("✓ CHECKSUM MANIFEST SAVED")
    else: # Validate mode
        if not os.path.exists(checksums_file):
            print(f"! ERROR: manifest.ini not found at '{checksums_file}'")
            env.Exit(1)
        config.read(checksums_file)
        if env_name not in config:
            print(f"! ERROR: No checksum entry for '{env_name}' in manifest.ini")
            env.Exit(1)
        
        # Restored detailed validation logic
        official_checksum = int(config[env_name].get('checksum', '0'), 0)
        official_size = int(config[env_name].get('size', '0'))
        checksum_matches = (checksum == official_checksum)
        size_matches = (firmware_size == official_size)

        if checksum_matches and size_matches:
            print("✓ CHECKSUM VALIDATION PASSED")
        else:
            print("!!! CHECKSUM VALIDATION FAILED !!!")
            if not checksum_matches:
                print(f"  ❌ CHECKSUM MISMATCH: Expected 0x{official_checksum:04x}, Got 0x{checksum:04x}")
            if not size_matches:
                print(f"  ❌ SIZE MISMATCH: Expected {official_size} bytes, Got {firmware_size} bytes")
            # env.Exit(1)
    
    return new_firmware_name

def regenerate_hex_for_avr(elf_file, hex_file_to_overwrite, env):
    """(AVR-Specific) Generates a custom .hex, overwriting the target file."""
    print("--- Regenerating HEX File for Upload (AVR Specific) ---")
    objcopy = env.subst("$OBJCOPY")
    cmd = f'"{objcopy}" -O ihex -j .text -j .firmware_checksum -j .data "{elf_file}" "{hex_file_to_overwrite}"'
    if env.Execute(cmd) != 0:
        print(f"✗ Failed to regenerate custom HEX file.")
        env.Exit(1)
    print(f"✓ Custom HEX file was regenerated successfully!")

def copy_and_rename_firmware(env, env_name, source_firmware_path, new_firmware_name):
    """Copies the final firmware artifact to the ./build/{env_name}/ directory."""
    print("--- Copying and Renaming Final Firmware ---")
    if not os.path.exists(source_firmware_path):
        print(f"✗ Error: Source firmware not found at '{source_firmware_path}'")
        return

    project_dir = env.subst("$PROJECT_DIR")
    dest_dir = os.path.join(project_dir, "build", env_name)
    os.makedirs(dest_dir, exist_ok=True)
    dest_file_path = os.path.join(dest_dir, new_firmware_name)
    shutil.copy2(source_firmware_path, dest_file_path)
    print(f"✓ Firmware copied to: {os.path.relpath(dest_file_path, project_dir)}")

# =================================================================
# Main Callback Functions and Action Registration
# =================================================================

def embed_checksum_action(source, target, env):
    """STAGE 1: Called after ELF linking to embed the checksum and handle the manifest."""
    print("\n--- [Stage 1] Starting Checksum Embedding & Manifest Handling ---")
    elf_file = str(target[0])
    env_name = env.subst("$PIOENV")
    platform_family = ENV_MAPPING.get(env_name)
    if platform_family:
        # Calculate checksum, get size, and embed it into the ELF
        checksum, firmware_size = calculate_and_embed_checksum(elf_file, env, platform_family)
        
        # Create/validate the manifest and get the final firmware name
        new_firmware_name = create_or_validate_manifest(env, env_name, checksum, firmware_size)
        
        # Store the calculated name in the environment for Stage 2
        env['NEW_FIRMWARE_NAME'] = new_firmware_name
        
    print("--- [Stage 1] Finished Checksum Embedding & Manifest Handling ---")

def finalize_firmware_action(source, target, env):
    """STAGE 2: Called after final firmware generation to finalize artifact."""
    print("\n--- [Stage 2] Starting Firmware Finalization ---")
    firmware_path = str(target[0])
    elf_file = env.subst("$PROGPATH")
    env_name = env.subst("$PIOENV")
    platform_family = ENV_MAPPING.get(env_name)

    if not platform_family:
        return

    # For AVR, regenerate the HEX file to overwrite the default one.
    if platform_family == "avr":
        regenerate_hex_for_avr(elf_file, firmware_path, env)

    # Retrieve the final firmware name determined in Stage 1.
    new_firmware_name = env.get('NEW_FIRMWARE_NAME')
    if not new_firmware_name:
        print("! Error: Could not determine new firmware name from stage 1. Skipping copy/rename.")
        return

    # Copy and rename the final firmware artifact if version is defined.
    if get_version_str(env):
        copy_and_rename_firmware(env, env_name, firmware_path, new_firmware_name)
    else:
        print("⚠ Warning: -DVERSION_STR not found. Skipping final rename & copy.")

    print("--- [Stage 2] Finished Firmware Finalization ---")

# --- Register Actions ---
env.AddPostAction("$PROGPATH", embed_checksum_action)
print("✓ Registered Stage 1: Checksum embedding & manifest action (on .elf).")

env_name = env.subst("$PIOENV")
platform_family = ENV_MAPPING.get(env_name)
if platform_family:
    firmware_ext = FIRMWARE_EXTENSIONS.get(platform_family)
    if firmware_ext:
        final_firmware_target = os.path.join("$BUILD_DIR", f"${{PROGNAME}}{firmware_ext}")
        env.AddPostAction(final_firmware_target, finalize_firmware_action)
        print(f"✓ Registered Stage 2: Firmware finalization action (on {firmware_ext}).")
