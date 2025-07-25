import os
from SCons.Script import Import, DefaultEnvironment

# --- Script Header ---
# This script customizes the PlatformIO build process.
# It ensures the final HEX file for uploading contains only the .text and .data sections
# by running a custom objcopy command right before the upload task begins.

env = DefaultEnvironment()

# --- Core Function ---
def regenerate_hex(elf_file, hex_file, env):
    """
    Generates a .hex file from a .elf file, including only .text and .data sections.
    """
    print("--- Regenerating HEX File for Upload ---")
    print(f"Source ELF: {elf_file}")
    print(f"Target HEX: {hex_file}")

    if not os.path.exists(elf_file):
        print(f"✗ Error: ELF file not found at '{elf_file}'. Skipping HEX regeneration.")
        # Exit with an error code to stop the build/upload process
        env.Exit(1) 

    # Use the objcopy from the current toolchain for portability
    objcopy = env.subst("$OBJCOPY")
    if not objcopy:
        print("✗ Error: $OBJCOPY path not found in environment.")
        env.Exit(1)

    # The command to create the final HEX file
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

def post_build_action(source, target, env):
    """
    Callback function to regenerate the HEX file right after the default build process completes.
    This is triggered by `pio run` or if `pio run -t upload` needs to recompile.
    """
    print("\nPost-build hook: Triggered after firmware build.")

    # The 'target' of this action is the default-generated .hex file.
    # Its path is provided as an SCons Node object in a list.
    hex_file = str(target[0])

    # Derive the corresponding .elf file path from the .hex file path.
    elf_file = os.path.splitext(hex_file)[0] + ".elf"

    # Call the main function to do the work.
    regenerate_hex(elf_file, hex_file, env)

def pre_upload_action(source, target, env):
    """
    Callback function to regenerate the HEX file just before the 'upload' task.
    """
    print("\nPre-upload hook: Triggered before starting upload.")
    # Get the final ELF file path directly from the build environment.
    # The dump confirms $PROGPATH is the correct variable.
    elf_file = env.subst("$PROGPATH")
    # Derive the HEX file path from the ELF file path.
    # The dump shows $HEXFILE does not exist, so this is the reliable way.
    hex_file = os.path.splitext(elf_file)[0] + ".hex"
    # Check that the environment variable was successfully expanded
    if not elf_file:
        print("✗ Error: Could not find $PROGPATH in the environment.")
        env.Exit(1)

    # Call the main function to do the work
    regenerate_hex(elf_file, hex_file, env)

# --- Registering the Action with PlatformIO ---
env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", post_build_action)
print("✓ Added post-build HEX regeneration action.")
# Register the pre-upload hook. This is the primary and most reliable mechanism.
#env.AddPreAction("upload", pre_upload_action)
#print("✓ Added pre-upload HEX regeneration action.")
