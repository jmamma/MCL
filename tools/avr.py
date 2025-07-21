Import("env")

# Add custom linker script
env.Append(LINKFLAGS=["-T", "custom.ld"])

# Or replace the default linker script
env.Replace(LDSCRIPT_PATH="custom.ld")

import os

# Method 1: Try to override the builder action directly
def custom_objcopy_action(source, target, env):
    cmd = f"avr-objcopy -O ihex -j .text -j .data {source[0]} {target[0]}"
    print(f"Custom objcopy: {cmd}")
    return env.Execute(cmd)

# Method 2: Post-build hook as backup
def regenerate_hex_postbuild(source, target, env):
    hex_file = str(target[0])
    elf_file = hex_file.replace('.hex', '.elf')

    print(f"Post-build: Regenerating {hex_file}")

    if os.path.exists(hex_file):
        os.remove(hex_file)

    cmd = f"avr-objcopy -O ihex -j .text -j .data {elf_file} {hex_file}"
    print(f"Running: {cmd}")

    result = env.Execute(cmd)
    if result == 0:
        print("✓ Custom hex file generated successfully!")
    return result

# Try to replace the ElfToHex builder
try:
    from SCons.Script import Builder
    env["BUILDERS"]["ElfToHex"] = Builder(
        action=env.VerboseAction(custom_objcopy_action, "Building $TARGET (custom)"),
        suffix=".hex",
    )
    print("✓ Replaced ElfToHex builder")
except Exception as e:
    print(f"Builder replacement failed: {e}")

# Add post-build hook as fallback
env.AddPostAction("$BUILD_DIR/firmware.hex", regenerate_hex_postbuild)
print("✓ Added post-build hex regeneration fallback")

