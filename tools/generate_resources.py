#!/usr/bin/env python3
#
# extra_script.py
#
# Unified, platform-aware script to build and process resource assets.
# This version uses robust parsing logic for both AVR and RP2040 platforms
# to generate correct, self-updating header files.
#

import os
import subprocess
import shutil
import re
Import("env")

# =============================================================================
# Helper Functions
# =============================================================================

def get_platform_subdir(env):
    """
    Determine the platform-specific subdirectory based on PlatformIO environment name.
    """
    env_name = env.subst("$PIOENV")
    env_mapping = {
        "rp2040": "rp2040", "rp2350": "rp2040", "pico": "rp2040",
        "avr": "avr", "mega": "avr", "uno": "avr", "nano": "avr",
    }
    for key, value in env_mapping.items():
        if key in env_name.lower():
            return value
    print(f"Warning: No mapping for environment '{env_name}'. Defaulting to 'generic'.")
    return "generic"

def get_tool_path(env, tool_name):
    """
    Finds the full path to a toolchain executable by searching the system PATH.
    """
    cc_base = os.path.basename(env.subst("$CC"))
    prefix = cc_base.replace("gcc", "").replace("clang", "")
    prefixed_tool = f"{prefix}{tool_name}"
    full_path = shutil.which(prefixed_tool)
    if full_path:
        return full_path
    print(f"Error: Could not find tool '{prefixed_tool}' in your system's PATH.")
    env.Exit(1)

def run_command(cmd, **kwargs):
    """Helper to run a command, print its output, and check for errors."""
    # print(f"Executing: {' '.join(cmd)}") # Uncomment for extreme verbosity
    result = subprocess.run(
        cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore', **kwargs
    )
    if result.returncode != 0:
        print(f"Error running command: {' '.join(cmd)}")
        print("STDOUT:", result.stdout, sep='\n')
        print("STDERR:", result.stderr, sep='\n')
        env.Exit(1)
    return result

# =============================================================================
# AVR Platform Asset Builder
# =============================================================================

def build_assets_avr(env, resource_dir, build_dir, gen_dir):
    """
    Asset pipeline for the AVR platform, using custom flags and robust parsing.
    """
    print("\n--- Running AVR Asset Build Pipeline ---")

    cxx = get_tool_path(env, "g++")
    objcopy = get_tool_path(env, "objcopy")
    objdump = get_tool_path(env, "objdump")

    custom_flags = [
        "-DF_CPU=16000000L", "-DARDUINO=10803", "-DARDUINO_AVR_MEGA2560",
        "-DARDUINO_ARCH_AVR", "-D__AVR_ATmega2560__", "-DAVR", "-std=gnu++1z",
        "-Os", "--short-enums", "-fpermissive", "-fshort-enums",
        "-fdata-sections"
    ]
    include_flags = env.subst("$_CPPINCFLAGS").split()
    compiler_flags = custom_flags + include_flags
    print("\n--- Scanning resource files for includes ---")
    all_includes = set()
    resource_cpp_files = [f for f in os.listdir(resource_dir) if f.endswith(".cpp")]
    for cpp_file in resource_cpp_files:
        with open(os.path.join(resource_dir, cpp_file), 'r', encoding='utf-8') as f_in:
            all_includes.update(line.strip() for line in f_in if line.strip().startswith("#include"))

    proceed = False
    for cpp_file in resource_cpp_files:
        base_name = os.path.splitext(cpp_file)[0]
        fin_path = os.path.join(gen_dir, f"R_{base_name}.cpp")
        if not os.path.isfile(fin_path):
           proceed = True
    if not proceed:
      print("Detected compressed assets, skipping compile/compression of resources.")
      return

    h_content = ['#pragma once', '']
    h_content.extend(sorted(list(all_includes)))
    h_content.append('')
    resman_content = []

    symbol_regex = re.compile(r"^\S+\s+.*\s+(\.data\S*)\s+([0-9a-f]{8})\s+(\S+)")

    for cpp_file in resource_cpp_files:
        base_name = os.path.splitext(cpp_file)[0]
        source_path = os.path.join(resource_dir, cpp_file)
        obj_path = os.path.join(build_dir, f"{base_name}.o")
        bin_path = os.path.join(build_dir, f"{base_name}.bin")
        ez_path = os.path.join(build_dir, f"{base_name}.ez")
        gen_cpp_path = os.path.join(gen_dir, f"R_{base_name}.cpp")

        print(f"\n--- Processing resource: {cpp_file} ---")
        run_command([cxx] + compiler_flags + ["-c", source_path, "-o", obj_path])

        dump_result = run_command([objdump, "-h", obj_path])
        data_sections = re.findall(r'^\s*\d+\s+(\.data\S*)', dump_result.stdout, re.MULTILINE)
        if not data_sections:
            open(bin_path, 'wb').close()
        else:
            with open(bin_path, "wb") as f_bin:
                for section in data_sections:
                    tmp_path = os.path.join(build_dir, f"{base_name}{section}.tmp")
                    run_command([objcopy, "-O", "binary", "-j", section, obj_path, tmp_path])
                    with open(tmp_path, "rb") as f_tmp:
                        f_bin.write(f_tmp.read())
                    os.remove(tmp_path)

        compress_script_path = os.path.join(env.subst("$PROJECT_DIR"), "tools", "compress.py")
        run_command(["python3", compress_script_path, os.path.abspath(bin_path), os.path.abspath(ez_path)])

        with open(gen_cpp_path, "w", encoding="utf-8") as f:
            f.write(f'#include "R.h"\n\nconst unsigned char __R_{base_name}[] PROGMEM = {{\n')
            with open(ez_path, "rb") as f_ez:
                f.write(",\n".join(f"  {b}" for b in f_ez.read()))
            f.write('\n};\n')

        h_content.append(f"extern const unsigned char __R_{base_name}[] PROGMEM;")
        symbols = []
        result = run_command([objdump, "-t", obj_path])
        for line in result.stdout.splitlines():
            match = symbol_regex.search(line)
            if match:
                symbol_name = match.group(3)
                if not symbol_name.startswith('.'):
                    symbols.append({"Size": int(match.group(2), 16), "Name": symbol_name})
        types = {}
        with open(source_path, "r", encoding='utf-8') as f_src:
            for line in f_src:
                if "=" in line and not line.strip().startswith("//"):
                    declaration = line.split('=')[0].strip()
                    type_and_name = declaration.split('[')[0].strip()
                    parts = type_and_name.split()
                    if len(parts) >= 2:
                        var_name = parts[-1]
                        var_type = " ".join(parts[:-1])
                        types[var_name] = var_type

        h_content.append(f"struct __T_{base_name} {{")
        total_sz = 0
        for sym in symbols:
            name, size, type_str = sym["Name"], sym["Size"], types.get(sym["Name"])
            if not type_str:
                print(f"Warning: Could not find type for symbol '{name}' in '{cpp_file}'. Skipping.")
                continue
            h_content.extend([
                f"  union {{", f"    {type_str} {name}[0];", f"    char zz__{name}[{size}];", f"  }};",
                f"  static constexpr size_t countof_{name} = {size} / sizeof({type_str});",
                f"  static constexpr size_t sizeofof_{name} = {size};"
            ])
            total_sz += size
        h_content.extend([f"  static constexpr size_t __total_size = {total_sz};", f"}};\n"])
        resman_content.extend([f"  __T_{base_name} *{base_name};", f"  void use_{base_name}() {{ {base_name} = (__T_{base_name}*) __use_resource(__R_{base_name}); }}"])

    print("\n--- Writing final header files for AVR ---")
    with open(os.path.join(gen_dir, "R.h"), "w") as f: f.write("\n".join(h_content))
    with open(os.path.join(gen_dir, "ResMan.h"), "w") as f: f.write("\n".join(resman_content))


def build_assets_rp2040(env, resource_dir, build_dir, gen_dir):
    """
    Asset pipeline for the RP2040 / RP2350 platform, now with robust parsing.
    """
    print("\n--- Running RP2040 Asset Build Pipeline ---")
    cxx = get_tool_path(env, "g++")
    objcopy = get_tool_path(env, "objcopy")
    objdump = get_tool_path(env, "objdump")
    cxx_flags = env.subst("$CCFLAGS $CXXFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS").split()
    all_obj_files = []
    print("\n--- Scanning resource files for includes ---")
    all_includes = set()
    resource_cpp_files = [f for f in os.listdir(resource_dir) if f.endswith(".cpp")]

    for cpp_file in resource_cpp_files:
        with open(os.path.join(resource_dir, cpp_file), "r") as f_cpp_in:
            all_includes.update(line.strip() for line in f_cpp_in if line.strip().startswith("#include"))

    h_content = ['#pragma once', '']
    h_content.extend(sorted(list(all_includes)))
    h_content.append('')
    resman_content = []

    symbol_regex = re.compile(r"^\S+\s+.*\s+(\.data\S*)\s+([0-9a-f]{8})\s+(\S+)")

    for cpp_file in resource_cpp_files:
        base_name = os.path.splitext(cpp_file)[0]
        source_path = os.path.join(resource_dir, cpp_file)
        obj_path = os.path.join(build_dir, f"{base_name}.o")
        bin_path = os.path.join(build_dir, f"{base_name}.bin")
        ez_path = os.path.join(build_dir, f"{base_name}.ez")
        gen_cpp_path = os.path.join(gen_dir, f"R_{base_name}.cpp")
        all_obj_files.append(obj_path)
        print(f"\n--- Processing resource: {cpp_file} ---")
        run_command([cxx] + cxx_flags + ["-c", source_path, "-o", obj_path])
        result = run_command([objdump, "-h", obj_path])
        data_sections = re.findall(r'^\s*\d+\s+(\.data\S*)', result.stdout, re.MULTILINE)
        data_sections.reverse()

        with open(bin_path, "wb") as f_bin:
            for section in data_sections:
                tmp_path = os.path.join(build_dir, f"{section}.tmp")
                run_command([objcopy, "-O", "binary", "--only-section=" + section, obj_path, tmp_path])
                with open(tmp_path, "rb") as f_tmp:
                    data = f_tmp.read()
                    f_bin.write(data + (b'\0' * ((4 - len(data) % 4) % 4)))
                os.remove(tmp_path)
        compress_script_path = os.path.join(env.subst("$PROJECT_DIR"), "tools", "compress.py")
        run_command(["python3", compress_script_path, os.path.abspath(bin_path), os.path.abspath(ez_path)])

        #with open(gen_cpp_path, "w") as f_cpp:
        #    f_cpp.write('#include "R.h"\n\n')
        #    f_cpp.write(f'const unsigned char __R_{base_name}[] PROGMEM = {{\n')
        #    result = run_command(["hexdump", "-v", "-e", '1/1 "%3u,\\n"', ez_path])
        #    f_cpp.write(result.stdout)
        #    f_cpp.write('};\n')
        with open(gen_cpp_path, "w") as f_cpp:
            f_cpp.write('#include "R.h"\n\n')
            f_cpp.write(f'const unsigned char __R_{base_name}[] PROGMEM = {{\n')
            # Read the binary file and convert bytes to the same format as hexdump
            with open(ez_path, "rb") as f_bin:
                bytes_data = f_bin.read()
            # Write bytes with proper formatting (3-digit width, comma after each except last)
            for i, byte_val in enumerate(bytes_data):
                if i == len(bytes_data) - 1:
                    # Last byte - no trailing comma
                    f_cpp.write(f'{byte_val:3d}\n')
                else:
                    f_cpp.write(f'{byte_val:3d},\n')
            f_cpp.write('};\n')

#        with open(gen_cpp_path, "w") as f_cpp:
#            f_cpp.write('#include "R.h"\n\n')
#            f_cpp.write(f'const unsigned char __R_{base_name}[] PROGMEM = {{\n')
#            # Read the binary file and convert bytes to the same format as hexdump
#            with open(ez_path, "rb") as f_bin:
#                while True:
#                    byte = f_bin.read(1)
#                    if not byte:
#                        break
#                    f_cpp.write(f'{byte[0]:3d},\n')
#            f_cpp.write('};\n')

        h_content.append(f"extern const unsigned char __R_{base_name}[] PROGMEM;")
        symbols = []
        result = run_command([objdump, "-t", obj_path])
        for line in result.stdout.splitlines():
            match = symbol_regex.search(line)
            if match:
                symbol_name = match.group(3)
                if not symbol_name.startswith('.'):
                    symbols.append({"Size": int(match.group(2), 16), "Name": symbol_name})
        types = {}
        with open(source_path, "r", encoding='utf-8') as f_src:
            for line in f_src:
                if "=" in line and not line.strip().startswith("//"):
                    declaration = line.split('=')[0].strip()
                    type_and_name = declaration.split('[')[0].strip()
                    parts = type_and_name.split()
                    if len(parts) >= 2:
                        var_name = parts[-1]
                        var_type = " ".join(parts[:-1])
                        types[var_name] = var_type
        h_content.append(f"struct __T_{base_name} {{\n")
        total_sz = 0
        symbols.reverse()
        for sym in symbols:
            name, size, type_str = sym["Name"], sym["Size"], types.get(sym["Name"])
            if not type_str:
                print(f"Warning: Could not find type for symbol '{name}' in '{cpp_file}'. Skipping.")
                continue
            size_aligned = (size + 3) & ~3 # 4-byte alignment for ARM
            h_content.extend([
                f"  union {{", f"    {type_str} {name}[0];", f"    char zz__{name}[{size_aligned}];", f"  }};",
                f"  static constexpr size_t countof_{name} = {size_aligned} / sizeof({type_str});",
                f"  static constexpr size_t sizeofof_{name} = {size_aligned};"
            ])
            total_sz += size_aligned
        h_content.extend([f"  static constexpr size_t __total_size = {total_sz};", f"}};\n"])
        resman_content.extend([f"  __T_{base_name} *{base_name};", f"  void use_{base_name}() {{ {base_name} = (__T_{base_name}*) __use_resource(__R_{base_name}); }}\n"])

    print("\n--- Writing final header files for RP2040 ---")
    with open(os.path.join(gen_dir, "R.h"), "w") as f_h: f_h.write("\n".join(h_content))
    with open(os.path.join(gen_dir, "ResMan.h"), "w") as f_rm: f_rm.write("\n".join(resman_content))


# =============================================================================
# Main Dispatcher & Entry Point
# =============================================================================

def build_assets(env):
    """Main dispatcher for the asset build process."""
    print("--- Running Custom Asset Build Script ---")
    resource_dir = os.path.join(env.subst("$PROJECT_DIR"), "resource")
    if not os.path.isdir(resource_dir) or not os.listdir(resource_dir):
        return

    platform_subdir = get_platform_subdir(env)
    print(f"Detected environment: {env.subst('$PIOENV')} -> Using platform: {platform_subdir}")

    resource_build_dir = os.path.join(env.subst("$BUILD_DIR"), "resource_assets")
    generated_src_dir = os.path.join(env.subst("$PROJECT_DIR"), "src", "resources", platform_subdir)
    os.makedirs(resource_build_dir, exist_ok=True)
    os.makedirs(generated_src_dir, exist_ok=True)

    if platform_subdir == "avr":
        build_assets_avr(env, resource_dir, resource_build_dir, generated_src_dir)
    elif platform_subdir == "rp2040":
        build_assets_rp2040(env, resource_dir, resource_build_dir, generated_src_dir)
    else:
        print(f"Warning: No asset build pipeline defined for platform '{platform_subdir}'.")

    print("--- Custom Asset Build Script Finished ---\n")

def sign_firmware(source, target, env):
    """Runs a signing script on the final binary."""
    if not env.subst("$CUSTOM_PRIVATE_KEY"):
        return
    private_key = env.subst("$PROJECT_DIR/$CUSTOM_PRIVATE_KEY")
    if not os.path.exists(private_key):
        return

    signing_script_path = os.path.join(env.subst("$PROJECT_DIR"), "tools", "signing.py")
    if not os.path.exists(signing_script_path):
        return

    bin_file = str(target[0])
    signed_bin_file = f"{bin_file}.signed"
    print(f"\n--- Signing firmware: {os.path.basename(bin_file)} ---")
    cmd = ["python3", signing_script_path, "--mode", "sign", "--privatekey", private_key, "--bin", bin_file, "--out", signed_bin_file]
    run_command(cmd)
    print(f"--- Successfully created signed firmware: {os.path.basename(signed_bin_file)} ---")


print("--- extra_script.py loaded ---")
build_assets(env)

if env.subst("$CUSTOM_PRIVATE_KEY"):
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", sign_firmware)
