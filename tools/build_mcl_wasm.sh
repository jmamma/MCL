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

is_wasm_build_input() {
    local path="$1"

    # These trees live below src/mcl but are deliberately excluded from the
    # hosted build's source discovery below.
    case "${path}" in
        src/mcl/Drivers/TBD/* | src/mcl/Tests/* | src/mcl/build/* | \
            src/mcl/tools/* | src/mcl/ceph-ansible/*)
            return 1
            ;;
    esac

    case "${path}" in
        src/mcl/*.c | src/mcl/*.cpp | src/mcl/*.h | \
            src/platform/desktop_common/*.c | \
            src/platform/desktop_common/*.cpp | \
            src/platform/desktop_common/*.h | \
            src/platform/wasm/*.c | src/platform/wasm/*.cpp | \
            src/platform/wasm/*.h | src/platform/wasm/sps/*.def | \
            src/platform/wasm/sps/*.json | \
            src/resources/rp2040/*.c | src/resources/rp2040/*.cpp | \
            src/resources/rp2040/*.h | include/*.h | \
            tools/build_mcl_wasm.sh | tools/wasm_abi_codegen.cpp)
            return 0
            ;;
    esac

    return 1
}

# A release package must name one Git tree that contains every source input.
# Development builds can opt out explicitly, but never do so implicitly: the
# resulting artifact cannot truthfully claim HEAD as its provenance.
if command -v git >/dev/null 2>&1 &&
   git -C "${MCL_ROOT}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    DIRTY_BUILD_INPUTS="$({
        git -C "${MCL_ROOT}" diff --name-only HEAD -- \
            src/mcl src/platform/desktop_common src/platform/wasm \
            src/resources/rp2040 include tools/build_mcl_wasm.sh \
            tools/wasm_abi_codegen.cpp
        git -C "${MCL_ROOT}" ls-files --others --exclude-standard -- \
            src/mcl src/platform/desktop_common src/platform/wasm \
            src/resources/rp2040 include tools/build_mcl_wasm.sh \
            tools/wasm_abi_codegen.cpp
    } | LC_ALL=C sort -u | while IFS= read -r path; do
        if is_wasm_build_input "${path}"; then
            printf '%s\n' "${path}"
        fi
    done)"
    if [ -n "${DIRTY_BUILD_INPUTS}" ] &&
       [ "${MCL_WASM_ALLOW_DIRTY:-0}" = "0" ]; then
        echo "[mcl-wasm] refusing non-reproducible dirty source inputs:" >&2
        while IFS= read -r path; do
            printf '  %s\n' "${path}" >&2
        done <<< "${DIRTY_BUILD_INPUTS}"
        echo "[mcl-wasm] commit them or set MCL_WASM_ALLOW_DIRTY=1 for a development-only build" >&2
        exit 1
    fi
    if [ -n "${DIRTY_BUILD_INPUTS}" ]; then
        echo "[mcl-wasm] WARNING: development build includes dirty source inputs"
    fi
    SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-$(git -C "${MCL_ROOT}" log -1 --format=%ct)}"
    export SOURCE_DATE_EPOCH
fi
export ZERO_AR_DATE=1

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

CLANG_RESOURCE_DIR="$("${CXX}" -print-resource-dir)"
if [ ! -f "${CLANG_RESOURCE_DIR}/include/stddef.h" ]; then
    echo "[mcl-wasm] restoring missing Clang resource headers"
    cmake --build "${LLVM}" --target clang-resource-headers \
        --parallel "${MCL_WASM_JOBS:-8}"
fi
[ -f "${CLANG_RESOURCE_DIR}/include/stddef.h" ] || {
    echo "Clang resource header not found at ${CLANG_RESOURCE_DIR}/include/stddef.h"
    exit 1
}

OUT="${MCL_ROOT}/build_wasm"
mkdir -p "${OUT}"

ABI_SCHEMA="${MCL_ROOT}/src/platform/wasm/sps/abi_exports.def"
ABI_TEMPLATE="${MCL_ROOT}/src/platform/wasm/sps/module.template.json"
ABI_CODEGEN_SOURCE="${MCL_ROOT}/tools/wasm_abi_codegen.cpp"
ABI_CODEGEN="${OUT}/wasm_abi_codegen"
ABI_GENERATED_DIR="${OUT}/generated"
ABI_HEADER="${ABI_GENERATED_DIR}/mcl_wasm_exports.generated.h"
ABI_LINKER_RSP="${ABI_GENERATED_DIR}/wasm_exports.rsp"
SPEC_SRC="${ABI_GENERATED_DIR}/module.json"
mkdir -p "${ABI_GENERATED_DIR}"
# Build the checked-in C++ generator every time. This avoids reusing an
# executable produced from another checkout or compiler invocation.
/usr/bin/c++ -std=c++17 -O2 "${ABI_CODEGEN_SOURCE}" -o "${ABI_CODEGEN}"
"${ABI_CODEGEN}" "${ABI_SCHEMA}" "${ABI_TEMPLATE}" "${SPEC_SRC}" \
    "${ABI_HEADER}" "${ABI_LINKER_RSP}"

WASM="${OUT}/mcl.wasm"
AOT="${OUT}/mcl.aot"
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
    -I"${ABI_GENERATED_DIR}"
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
        ! -path "*/Drivers/TBD/*" ! -path "*/Tests/*" ! -path "*/ceph-ansible/*" \
        | LC_ALL=C sort
)
while IFS= read -r f; do MCL_C+=("$f"); done < <(
    find "${MCL_ROOT}/src/mcl" -name "*.c" \
        ! -path "*/.pio/*" ! -path "*/build/*" ! -path "*/tools/*" \
        ! -path "*/ceph-ansible/*" | LC_ALL=C sort
)

DESKTOP_COMMON_CPP=()
while IFS= read -r f; do DESKTOP_COMMON_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/platform/desktop_common" -name "*.cpp" \
        | LC_ALL=C sort
)

WASM_CPP=()
while IFS= read -r f; do WASM_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/platform/wasm" -name "*.cpp" | LC_ALL=C sort
)
WASM_LIBC_C=("${MCL_ROOT}/src/platform/wasm/libc/libc.c")

RESOURCE_CPP=()
while IFS= read -r f; do RESOURCE_CPP+=("$f"); done < <(
    find "${MCL_ROOT}/src/resources/rp2040" -name "*.cpp" | LC_ALL=C sort
)

ADAFRUIT_ROOT="${MCL_ROOT}/.pio/libdeps/tbd/Adafruit GFX Library"
ADAFRUIT_SOURCE_DIGEST="1ecea977274d5b2b1146f87fb6a8c5bb3f05bec0c616555f85ad14166704536f"
[ -f "${ADAFRUIT_ROOT}/Adafruit_GFX.cpp" ] || {
    echo "Adafruit GFX 1.12.1 source is unavailable at ${ADAFRUIT_ROOT}" >&2
    exit 1
}
ADAFRUIT_ACTUAL_DIGEST="$(
    find "${ADAFRUIT_ROOT}" -type f \( -name '*.cpp' -o -name '*.h' \) \
        -print | LC_ALL=C sort | while IFS= read -r file; do
            shasum -a 256 "${file}" | awk '{print $1}'
        done | shasum -a 256 | awk '{print $1}'
)"
if [ "${ADAFRUIT_ACTUAL_DIGEST}" != "${ADAFRUIT_SOURCE_DIGEST}" ]; then
    echo "Adafruit GFX source checksum mismatch: expected ${ADAFRUIT_SOURCE_DIGEST}, got ${ADAFRUIT_ACTUAL_DIGEST}" >&2
    exit 1
fi
ADAFRUIT_CPP=("${ADAFRUIT_ROOT}/Adafruit_GFX.cpp")

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
    @"${ABI_LINKER_RSP}" \
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
if [ "${MCL_WASM_INSTALL:-1}" != "0" ]; then
    mkdir -p "${INSTALL_DIR}"
    cp "${WASM}" "${INSTALL_DIR}/mcl.wasm"
    cp "${AOT}" "${INSTALL_DIR}/mcl.aot"
    cp "${SPEC_SRC}" "${INSTALL_DIR}/module.json"
    mkdir -p "${INSTALL_DIR}/data"
    INSTALL_SUMMARY=" installed=${INSTALL_DIR}"
else
    INSTALL_SUMMARY=" install=disabled"
fi

WASM_BYTES="$(wc -c < "${WASM}")"
AOT_BYTES="$(wc -c < "${AOT}")"
echo "[mcl-wasm] done. wasm=${WASM_BYTES} aot=${AOT_BYTES} package=${PACKAGE_DIR}${INSTALL_SUMMARY}"
