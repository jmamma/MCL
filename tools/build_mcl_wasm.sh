#!/usr/bin/env bash
# build_mcl_wasm.sh — compile MCL → mcl.wasm → mcl.aot using the same
# clang++ + wamrc toolchain that scripts/build_usr_example.sh in the
# parent dsp56k_emu repo uses for USR machines.
#
# Output:
#   build_wasm/mcl.wasm
#   build_wasm/mcl.aot
#   build_wasm/package/mcl/module.json
#   build_wasm/package/mcl/mcl.aot
#
# The package directory is the runtime-installable module bundle.

set -euo pipefail

MCL_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DSP_ROOT="$(cd "${MCL_ROOT}/.." && pwd)"

LLVM="${DSP_ROOT}/llvm-build"
CC="${LLVM}/bin/clang"
CXX="${LLVM}/bin/clang++"
if [ -x "${LLVM}/bin/wamrc" ]; then
    WAMRC="${LLVM}/bin/wamrc"
else
    WAMRC="${DSP_ROOT}/external/wamr/wamr-compiler/build/wamrc"
fi

[ -x "${CC}" ]    || { echo "clang not found at ${CC}"; exit 1; }
[ -x "${CXX}" ]   || { echo "clang++ not found at ${CXX}"; exit 1; }
[ -x "${WAMRC}" ] || { echo "wamrc not found at ${WAMRC}"; exit 1; }

OUT="${MCL_ROOT}/build_wasm"
mkdir -p "${OUT}"

WASM="${OUT}/mcl.wasm"
AOT="${OUT}/mcl.aot"
SPEC_SRC="${MCL_ROOT}/src/platform/wasm/sps/module.json"
PACKAGE_DIR="${OUT}/package/mcl"
PACKAGE_WASM="${PACKAGE_DIR}/mcl.wasm"
PACKAGE_AOT="${PACKAGE_DIR}/mcl.aot"
PACKAGE_SPEC="${PACKAGE_DIR}/module.json"
INSTALL_DIR="${SPS_X_MCL_DIR:-${HOME}/Library/Application Support/SPS-X/modules/mcl}"
mkdir -p "${PACKAGE_DIR}"
WASM_STACK_SIZE="${MCL_WASM_STACK_SIZE:-1048576}"

# Per-platform include paths. Mirror MCL/CMakeLists.txt (desktop_common +
# wasm BEFORE the MCL tree includes), plus MCL's tbd-env -I list.
INCLUDES=(
    # libc stubs first — clang's freestanding mode provides stdint/stddef
    # only; src/platform/wasm/libc/ fills in string.h/stdlib.h/stdio.h/math.h.
    -I"${MCL_ROOT}/src/platform/wasm/libc"
    -I"${MCL_ROOT}/src/platform/wasm"
    -I"${MCL_ROOT}/src/platform/desktop_common"
    -I"${MCL_ROOT}/src"
    -I"${MCL_ROOT}/src/mcl"
    -I"${MCL_ROOT}/include"

    -I"${MCL_ROOT}/src/mcl/ResourceManager"
    -I"${MCL_ROOT}/src/mcl/A4"
    -I"${MCL_ROOT}/src/mcl/MCL"
    -I"${MCL_ROOT}/src/mcl/MD"
    -I"${MCL_ROOT}/src/mcl/MNM"
    -I"${MCL_ROOT}/src/mcl/Drivers"
    -I"${MCL_ROOT}/src/mcl/Drivers/A4"
    -I"${MCL_ROOT}/src/mcl/Drivers/A4/GridTracks"
    -I"${MCL_ROOT}/src/mcl/Drivers/A4/Sequencer"
    -I"${MCL_ROOT}/src/mcl/Drivers/Generic"
    -I"${MCL_ROOT}/src/mcl/Drivers/Generic/GridTracks"
    -I"${MCL_ROOT}/src/mcl/Drivers/Generic/Sequencer"
    -I"${MCL_ROOT}/src/mcl/Drivers/MD"
    -I"${MCL_ROOT}/src/mcl/Drivers/MD/GridTracks"
    -I"${MCL_ROOT}/src/mcl/Drivers/MD/Sequencer"
    -I"${MCL_ROOT}/src/mcl/Drivers/MD/UI/Pages"
    -I"${MCL_ROOT}/src/mcl/Drivers/MD/UI"
    -I"${MCL_ROOT}/src/mcl/Drivers/MNM"
    -I"${MCL_ROOT}/src/mcl/Drivers/MNM/GridTracks"
    -I"${MCL_ROOT}/src/mcl/GUI"
    -I"${MCL_ROOT}/src/mcl/Midi"
    -I"${MCL_ROOT}/src/mcl/CommonTools"
    -I"${MCL_ROOT}/src/mcl/Fonts"
    -I"${MCL_ROOT}/src/mcl/Diagnostic"
    -I"${MCL_ROOT}/src/mcl/Elektron"
    -I"${MCL_ROOT}/src/mcl/MidiTools"
    -I"${MCL_ROOT}/src/mcl/Tests"

    -I"${MCL_ROOT}/.pio/libdeps/tbd/Adafruit GFX Library"
    -I"${MCL_ROOT}/src/resources/rp2040"
)

DEFINES=(
    -DARDUINO=10800
    -DPLATFORM_DESKTOP=1
    -DPLATFORM_WASM=1
    -DVERSION=5000
    -DVERSION_STR='"T5.00"'
    -DMCL_DESKTOP_LINK_MCL_CORE=1
    -DMCL_HAS_DESKTOP_MOUSE=1
)

if [ "${MCL_WASM_DEBUG:-0}" != "0" ]; then
    DEFINES+=(-DDEBUGMODE=1)
    echo "[mcl-wasm] DEBUGMODE enabled; DEBUG_PRINT* routes through bounded DebugBuffer"
fi

if [ "${MCL_WASM_GUI_TRACE:-0}" != "0" ]; then
    DEFINES+=(-DMCL_WASM_GUI_TRACE=1)
    echo "[mcl-wasm] GUI tick tracing enabled"
fi

if [ "${MCL_WASM_DISABLE_SOFTWARE_IRQ:-0}" != "0" ]; then
    DEFINES+=(-DMCL_WASM_DISABLE_SOFTWARE_IRQ=1)
    echo "[mcl-wasm] software IRQ/audio catch-up disabled"
fi

if [ "${MCL_WASM_DEBUG_SYMBOLS:-0}" != "0" ] || [ "${MCL_WASM_DUMP_CALL_STACK:-0}" != "0" ]; then
    echo "[mcl-wasm] debug symbols/custom names enabled"
fi

# Common flags for both C and C++. USR example uses -nostdlib -ffreestanding;
# MCL is bigger and uses libc++ templates / std::string-style code via the
# WString shim, so for the first cut we leave stdlib alone and see what
# resolves.
COMMON_FLAGS=(
    --target=wasm32-unknown-unknown
    -nostdlib
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -O2
    -Wno-deprecated-declarations
    -Wno-unused-parameter
    -Wno-write-strings
    -Wno-narrowing
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-sign-compare
    -fpermissive
)

if [ "${MCL_WASM_DEBUG_SYMBOLS:-0}" != "0" ] || [ "${MCL_WASM_DUMP_CALL_STACK:-0}" != "0" ]; then
    COMMON_FLAGS+=(-g)
fi

CXX_FLAGS=(
    -std=c++17
    # Don't emit __cxa_atexit calls for static-storage dtors. We never
    # tear the wasm instance down via exit(), so global dtors don't
    # matter; and clang otherwise inserts __cxa_atexit as an unresolved
    # import even when a definition is present in the same link.
    -fno-use-cxa-atexit
    # No threadsafe-static guards (already in COMMON_FLAGS, but be loud).
    -fno-threadsafe-statics
)
C_FLAGS=()

# Collect MCL source files (mirror CMakeLists.txt filter).
MCL_CPP=()
MCL_C=()
while IFS= read -r f; do MCL_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/mcl" -name "*.cpp" \
        ! -path "*/.pio/*" ! -path "*/build/*" ! -path "*/tools/*" \
        ! -path "*/Drivers/TBD/*" ! -path "*/Tests/*" ! -path "*/ceph-ansible/*"
)
while IFS= read -r f; do MCL_C+=("$f"); done < <(
    find "${MCL_ROOT}/src/mcl" -name "*.c" \
        ! -path "*/.pio/*" ! -path "*/build/*" ! -path "*/tools/*" \
        ! -path "*/ceph-ansible/*"
)

DESKTOP_COMMON_CPP=()
while IFS= read -r f; do DESKTOP_COMMON_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/platform/desktop_common" -name "*.cpp"
)

WASM_CPP=()
while IFS= read -r f; do WASM_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/platform/wasm" -name "*.cpp"
)
WASM_LIBC_C=("${MCL_ROOT}/src/platform/wasm/libc/libc.c")

RESOURCE_CPP=()
while IFS= read -r f; do RESOURCE_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/resources/rp2040" -name "*.cpp"
)

ADAFRUIT_CPP=(
    "${MCL_ROOT}/.pio/libdeps/tbd/Adafruit GFX Library/Adafruit_GFX.cpp"
)

echo "[mcl-wasm] sources: ${#MCL_CPP[@]} cpp, ${#MCL_C[@]} c, ${#DESKTOP_COMMON_CPP[@]} desktop_common, ${#WASM_CPP[@]} wasm, ${#RESOURCE_CPP[@]} resources"
echo "[mcl-wasm] compiling → ${WASM}"
echo "[mcl-wasm] wasm aux stack: ${WASM_STACK_SIZE} bytes"

"${CXX}" \
    "${COMMON_FLAGS[@]}" \
    "${CXX_FLAGS[@]}" \
    "${DEFINES[@]}" \
    "${INCLUDES[@]}" \
    -Wl,--no-entry \
    -Wl,--export-dynamic \
    -Wl,-z,stack-size="${WASM_STACK_SIZE}" \
    -Wl,--export=__wasm_call_ctors \
    -Wl,--export=mcl_setup \
    -Wl,--export=mcl_tick_audio \
    -Wl,--export=mcl_tick_gui \
    -Wl,--export=mcl_framebuffer_offset \
    -Wl,--export=mcl_framebuffer_stride \
    -Wl,--export=mcl_framebuffer_width \
    -Wl,--export=mcl_framebuffer_height \
    -Wl,--export=mcl_midi_in_push \
    -Wl,--export=mcl_midi_out_pop \
    -Wl,--export=mcl_set_transport_position \
    -Wl,--export=mcl_input_set_button_mask \
    -Wl,--export=mcl_input_set_button_mask64 \
    -Wl,--export=mcl_input_add_encoder_delta \
    -Wl,--export=mcl_input_set_encoder_button \
    -Wl,--export=mcl_abi_version \
    -Wl,--export=mcl_debug_tempo_x100 \
    -Wl,--export=mcl_debug_state \
    -Wl,--export=mcl_debug_value \
    -Wl,--allow-undefined \
    -o "${WASM}" \
    -x c++ \
    "${MCL_CPP[@]}" \
    "${MCL_C[@]}" \
    "${DESKTOP_COMMON_CPP[@]}" \
    "${WASM_CPP[@]}" \
    "${WASM_LIBC_C[@]}" \
    "${RESOURCE_CPP[@]}" \
    "${ADAFRUIT_CPP[@]}"

echo "[mcl-wasm] AOT compile → ${AOT}"
WAMRC_FLAGS=(
    --disable-simd
    --disable-ref-types
    --bounds-checks=1
    --stack-bounds-checks=1
)

# Darwin/arm64 reserves x18 as a platform register. WAMR's default AOT target
# triple on macOS is still ELF/Linux-shaped, so LLVM may otherwise allocate x18
# as a general register and the hosted plugin can crash inside AOT code.
if [ "$(uname -s)" = "Darwin" ] && { [ "$(uname -m)" = "arm64" ] || [ "$(uname -m)" = "aarch64" ]; }; then
    WAMRC_FLAGS+=(
        --target=aarch64v8
        --cpu=apple-m1
        --cpu-features=+reserve-x18
    )
fi

if [ "${MCL_WASM_DUMP_CALL_STACK:-0}" != "0" ]; then
    WAMRC_FLAGS+=(
        --enable-dump-call-stack
        --call-stack-features=bounds-checks,ip,func-idx,trap-ip,values
        --emit-custom-sections=name
    )
fi
"${WAMRC}" \
    "${WAMRC_FLAGS[@]}" \
    -o "${AOT}" "${WASM}"

cp "${WASM}" "${PACKAGE_WASM}"
cp "${AOT}" "${PACKAGE_AOT}"
cp "${SPEC_SRC}" "${PACKAGE_SPEC}"
mkdir -p "${INSTALL_DIR}"
cp "${WASM}" "${INSTALL_DIR}/mcl.wasm"
cp "${AOT}" "${INSTALL_DIR}/mcl.aot"
cp "${SPEC_SRC}" "${INSTALL_DIR}/module.json"
mkdir -p "${INSTALL_DIR}/data"

echo "[mcl-wasm] done. wasm=$(wc -c < "${WASM}") aot=$(wc -c < "${AOT}") package=${PACKAGE_DIR} installed=${INSTALL_DIR}"
