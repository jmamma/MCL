#!/usr/bin/env bash
# build_mcl_wasm.sh — compile MCL → mcl.wasm → mcl.aot using the same
# clang++ + wamrc toolchain that scripts/build_usr_example.sh in the
# parent dsp56k_emu repo uses for USR machines.
#
# Output: build_wasm/mcl.wasm and build_wasm/mcl.aot. The .aot is what
# the SPS plugin would load via WAMR.

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
    -DVERSION=4080
    -DVERSION_STR='"R4.80-wasm"'
    -DMCL_DESKTOP_LINK_MCL_CORE=1
)

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

CXX_FLAGS=(-std=c++17)
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

"${CXX}" \
    "${COMMON_FLAGS[@]}" \
    "${CXX_FLAGS[@]}" \
    "${DEFINES[@]}" \
    "${INCLUDES[@]}" \
    -Wl,--no-entry \
    -Wl,--export-dynamic \
    -Wl,--allow-undefined \
    -o "${WASM}" \
    "${MCL_CPP[@]}" \
    "${MCL_C[@]}" \
    "${DESKTOP_COMMON_CPP[@]}" \
    "${WASM_CPP[@]}" \
    "${WASM_LIBC_C[@]}" \
    "${RESOURCE_CPP[@]}" \
    "${ADAFRUIT_CPP[@]}"

echo "[mcl-wasm] AOT compile → ${AOT}"
"${WAMRC}" \
    --disable-simd \
    --disable-ref-types \
    -o "${AOT}" "${WASM}"

echo "[mcl-wasm] done. wasm=$(wc -c < "${WASM}") aot=$(wc -c < "${AOT}")"
