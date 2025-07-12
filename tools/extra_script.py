#!/usr/bin/env python3

# extra_script.py
import os
import subprocess
import shutil
import re
Import("env")

def find_include_dirs(base_dir):
    """Find all directories that contain header files"""
    include_dirs = []

    if not os.path.exists(base_dir):
        return include_dirs

    for root, dirs, files in os.walk(base_dir):
        # Check if directory contains header files
        has_headers = any(f.endswith(('.h', '.hpp', '.hxx', '.H')) for f in files)
        if has_headers:
            include_dirs.append("-I" + root)

    return include_dirs


# This is the PlatformIO entry point
#env = DefaultEnvironment()
def print_all_env_vars(env):
    """
    A helper function to print all available keys and their
    expanded values from an SCons build environment.
    """
    print("====================== SCons Environment Variables ======================")
    keys = sorted(env.Dictionary().keys())
    max_len = max(len(key) for key in keys)
    for key in keys:
        value = env.subst(f'${key}')
        print(f"{key.ljust(max_len)} : {value}")
    print("======")
print_all_env_vars(env)

def get_tool_path(env, tool_name):
    """Derives a tool path like objdump from the CXX compiler path."""
    # e.g., /path/to/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-g++
    cxx_path = env.subst("$CXX")
    # -> /path/to/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-objdump
    bin_path = os.path.dirname(cxx_path)
    tool_prefix = os.path.basename(cxx_path).replace("g++", "")
    tool = os.path.join(bin_path, f"{tool_prefix}{tool_name}")
    return tool

def run_command(cmd, **kwargs):
    """Helper to run a command and check for errors."""
    print(f"Executing: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    if result.returncode != 0:
        print(f"Error running command: {' '.join(cmd)}")
        print("STDOUT:")
        print(result.stdout)
        print("STDERR:")
        print(result.stderr)
        env.Exit(1)
    return result

def build_assets(env):
    """
    This function replicates the 'assets' target from the Makefile.
    It compiles, extracts, compresses, and generates source files from resources.
    """
    print("--- Running Custom Asset Build Script ---")

    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")
    resource_dir = os.path.join(project_dir, "resource")
    tools_dir = os.path.join(project_dir, "tools")

    if not os.path.isdir(resource_dir):
        print("No 'resource' directory found. Skipping asset build.")
        return

    # Paths for generated files
    resource_build_dir = os.path.join(build_dir, "resource_assets")
    generated_src_dir = os.path.join(project_dir, "src", "resources")
    os.makedirs(resource_build_dir, exist_ok=True)
    os.makedirs(generated_src_dir, exist_ok=True)

    # Tool paths
    cxx = env.subst("$CXX")
    objcopy = get_tool_path(env, "objcopy")
    objdump = get_tool_path(env, "objdump")

    # Get CXX flags from the environment, and add the specific ones from the makefile
    cxx_flags = env.subst("$CCFLAGS $CXXFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS").split()

    resource_cpps = [f for f in os.listdir(resource_dir) if f.endswith(".cpp")]
    if not resource_cpps:
        print("No .cpp files in 'resource' directory. Skipping asset build.")
        return

    all_obj_files = []

    for cpp_file in resource_cpps:
        base_name = os.path.splitext(cpp_file)[0]
        source_path = os.path.join(resource_dir, cpp_file)
        obj_path = os.path.join(resource_build_dir, f"{base_name}.o")
        bin_path = os.path.join(resource_build_dir, f"{base_name}.bin")
        ez_path = os.path.join(resource_build_dir, f"{base_name}.ez")
        gen_cpp_path = os.path.join(resource_build_dir, f"R_{base_name}.cpp")
        
        all_obj_files.append(obj_path)
        # Find all directories under src/ that contain headers
        src_includes = find_include_dirs("src")
        # 1. Compile resource cpp to object file
        print(f"\n--- Compiling resource: {cpp_file} ---")
        compile_cmd = [cxx] + cxx_flags + src_includes + ["-c", source_path, "-o", obj_path]
        run_command(compile_cmd)

        # 2. Extract .data sections into a .bin file
        print(f"--- Extracting data from {base_name}.o ---")
        # Get list of .data sections
        dump_cmd = [objdump, "-h", obj_path]
        result = run_command(dump_cmd)
        data_sections = re.findall(r'^\s*\d+\s+(\.data\.\S+)', result.stdout, re.MULTILINE)
        data_sections.reverse() # Match 'tail -r' from makefile

        with open(bin_path, "wb") as f_bin:
            for section in data_sections:
                tmp_path = os.path.join(resource_build_dir, f"{section}.tmp")
                extract_cmd = [objcopy, "-O", "binary", "--only-section=" + section, obj_path, tmp_path]
                run_command(extract_cmd)
                with open(tmp_path, "rb") as f_tmp:
                    data = f_tmp.read()
                    # 4-byte padding
                    padding_needed = (4 - len(data) % 4) % 4
                    data += b'\0' * padding_needed
                    f_bin.write(data)
                os.remove(tmp_path)

        compress_script_path = os.path.join(tools_dir, "compress.py")
        compress_cmd = ["python3", compress_script_path, os.path.abspath(bin_path), os.path.abspath(ez_path)]
        print(f"\n--- Compression")
        print(compress_cmd)
        run_command(compress_cmd)

        # 4. Generate R_*.cpp from .ez file
        print(f"--- Generating R_{base_name}.cpp ---")
        with open(gen_cpp_path, "w") as f_cpp:
            f_cpp.write('#include "R.h"\n\n')
            f_cpp.write(f'const unsigned char __R_{base_name}[] PROGMEM = {{\n')
            result = run_command(["hexdump", "-v", "-e", '1/1 "%3u,\\n"', ez_path])
            f_cpp.write(result.stdout)
            f_cpp.write('};\n')
        # Copy to src/resources so the main build can find it
        shutil.copy(gen_cpp_path, os.path.join(generated_src_dir, os.path.basename(gen_cpp_path)))

    # 5. Generate R.h header
    print("\n--- Generating R.h ---")
    r_header_path = os.path.join(generated_src_dir, "R.h")
    with open(r_header_path, "w") as f_h:
        f_h.write("#pragma once\n\n")
        # Add includes from resource files
        includes = set()
        for cpp_file in resource_cpps:
            with open(os.path.join(resource_dir, cpp_file), "r") as f_cpp_in:
                for line in f_cpp_in:
                    if line.strip().startswith("#include"):
                        includes.add(line.strip())
        for inc in sorted(list(includes)):
            f_h.write(f"{inc}\n")
        f_h.write("\n")

        for obj_path in all_obj_files:
            base_name = os.path.splitext(os.path.basename(obj_path))[0]
            f_h.write(f"extern const unsigned char __R_{base_name}[] PROGMEM;\n")
            f_h.write(f"struct __T_{base_name} {{\n")
            
            dump_cmd = [objdump, "-t", obj_path]
            result = run_command(dump_cmd)
            # Find global data symbols
            symbols_raw = re.findall(r'^[0-9a-f]+\s+g\s+O\s+\.data\.\S+\s+([0-9a-f]+)\s+(\S+)', result.stdout, re.MULTILINE)
            symbols_raw.reverse()

            total_size = 0
            for size_hex, name in symbols_raw:
                size_dec = int(size_hex, 16)
                size_aligned = (size_dec + 3) // 4 * 4
                total_size += size_aligned
                
                # Try to find the original type definition from the source .cpp
                cpp_path = os.path.join(resource_dir, f"{base_name}.cpp")
                var_type = "unsigned char" # Default
                with open(cpp_path, "r") as f_cpp_in:
                    match = re.search(r'^\s*((?:unsigned\s+)?\S+(?:\s*<[^>]*>)?)\s+' + re.escape(name), f_cpp_in.read(), re.MULTILINE)
                    if match:
                        var_type = match.group(1).strip()
                
                f_h.write(f"  union {{\n")
                f_h.write(f"    {var_type} {name}[0];\n")
                f_h.write(f"    char zz__{name}[{size_aligned}];\n")
                f_h.write(f"  }};\n")
                f_h.write(f"  static constexpr size_t countof_{name} = {size_aligned} / sizeof({var_type});\n")
                f_h.write(f"  static constexpr size_t sizeofof_{name} = {size_aligned};\n\n")
            
            f_h.write(f"  static constexpr size_t __total_size = {total_size};\n")
            f_h.write(f"}};\n\n")
    
    # 6. Generate ResMan.h header
    print("--- Generating ResMan.h ---")
    resman_header_path = os.path.join(generated_src_dir, "ResMan.h")
    with open(resman_header_path, "w") as f_rm:
        f_rm.write("#pragma once\n\n")
        for obj_path in all_obj_files:
            base_name = os.path.splitext(os.path.basename(obj_path))[0]
            f_rm.write(f"__T_{base_name} *{base_name};\n")
            f_rm.write(f"void use_{base_name}() {{ {base_name} = (__T_{base_name}*) __use_resource(__R_{base_name}); }}\n\n")

    print("--- Custom Asset Build Script Finished ---\n")

def sign_firmware(source, target, env):
    """
    This function replicates the signing step from the Makefile.
    It runs after the .bin file has been created.
    """
    private_key = env.subst("$PROJECT_DIR/$CUSTOM_PRIVATE_KEY")
    if not os.path.exists(private_key):
        print(f"Warning: Private key not found at {private_key}. Skipping signing.")
        return

    # Path to the python script for signing, assuming it's in a 'tools' dir
    # This path might need to be adjusted based on your project structure.
    signing_script_path = os.path.join(env.subst("$PROJECT_DIR"), "tools", "signing.py")
    if not os.path.exists(signing_script_path):
        print(f"Warning: Signing script not found at {signing_script_path}. Skipping signing.")
        return

    bin_file = str(target[0])
    signed_bin_file = f"{bin_file}.signed"

    print(f"\n--- Signing firmware: {bin_file} ---")

    cmd = [
        "python3",
        signing_script_path,
        "--mode", "sign",
        "--privatekey", private_key,
        "--bin", bin_file,
        "--out", signed_bin_file
    ]
    run_command(cmd)
    print(f"--- Successfully created signed firmware: {signed_bin_file} ---")

print(f"Extra scripts\n");

build_assets(env)
# Register the signing function to run after the .bin is built
# Note: PlatformIO calls the target .elf, but it produces a .bin as well.
# We target the ELF so this runs after linking.
if env.subst("$CUSTOM_PRIVATE_KEY"):
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", sign_firmware)
