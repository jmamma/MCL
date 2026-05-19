# MCL — desktop static lib for the SPS JUCE plugin.
#
# Produces libmcl_desktop.a. The PlatformIO targets in platformio.ini
# (avr/megacommand/tbd) are untouched.
#
# Platform layout:
#   src/platform/desktop_common/  — shared between desktop and wasm. Headers
#                                   (Arduino.h, SdFat.h, etc.) plus the .cpp
#                                   bodies that don't reference host-specific
#                                   facilities. Both platforms compile this.
#   src/platform/desktop/         — host-specific .cpps that use std::chrono,
#                                   std::filesystem, std::fstream. Built by
#                                   this CMakeLists.
#   src/platform/wasm/            — wasm-host-specific .cpps that route I/O
#                                   through wasm host imports. Built by
#                                   scripts/build_mcl_wasm.sh (separate
#                                   toolchain — llvm-build clang++ +
#                                   wamrc), not this CMake.

cmake_minimum_required(VERSION 3.22)
# C is required because src/mcl/CommonTools/helpers.c provides clock_diff/
# popcount/limit_value/m_toupper/masks. Without C enabled, CMake silently
# drops .c sources from add_library.
project(mcl_desktop C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-shim sources.
file(GLOB DESKTOP_COMMON_SRC CONFIGURE_DEPENDS
    "src/platform/desktop_common/*.cpp")
file(GLOB DESKTOP_SRC CONFIGURE_DEPENDS
    "src/platform/desktop/*.cpp")

# MCL application code. Mirror the TBD env's build_src_filter:
# +<mcl/*> -<mcl/tools/*>. Drivers/TBD is RP2350-specific — excluded.
# Strip out PlatformIO build artefacts (.pio/, build/) and IDE/asset dirs.
# Glob both .cpp and .c — src/mcl/CommonTools/helpers.c is C.
file(GLOB_RECURSE MCL_CORE_SRC CONFIGURE_DEPENDS
    "src/mcl/*.cpp"
    "src/mcl/*.c")
list(FILTER MCL_CORE_SRC EXCLUDE REGEX ".*/\\.pio/.*")
list(FILTER MCL_CORE_SRC EXCLUDE REGEX ".*/build/.*")
list(FILTER MCL_CORE_SRC EXCLUDE REGEX ".*/tools/.*")
list(FILTER MCL_CORE_SRC EXCLUDE REGEX ".*/Drivers/TBD/.*")
list(FILTER MCL_CORE_SRC EXCLUDE REGEX ".*/Tests/.*")
list(FILTER MCL_CORE_SRC EXCLUDE REGEX ".*/ceph-ansible/.*")

# Bundled Arduino libraries from PlatformIO's libdeps. Paths contain spaces;
# CMake handles them as long as we quote the globs.
set(LIBDEPS "${CMAKE_CURRENT_SOURCE_DIR}/.pio/libdeps/tbd")
file(GLOB ADAFRUIT_GFX_SRC CONFIGURE_DEPENDS
    "${LIBDEPS}/Adafruit GFX Library/Adafruit_GFX.cpp")

# Pre-generated resource C++ files. Reuse the rp2040 set — the rp2040 R.h is
# layout-compatible since both desktop and rp2040 are flat-memory.
file(GLOB RESOURCE_SRC CONFIGURE_DEPENDS
    "src/resources/rp2040/*.cpp")

# Smoke-test main.cpp is host-runnable, not part of the static lib.
list(FILTER DESKTOP_SRC EXCLUDE REGEX ".*linktest_main\\.cpp$")

add_library(mcl_desktop STATIC
    ${DESKTOP_COMMON_SRC}
    ${DESKTOP_SRC}
    ${MCL_CORE_SRC}
    ${ADAFRUIT_GFX_SRC}
    ${RESOURCE_SRC})

# Compile-time switches MCL inspects.
target_compile_definitions(mcl_desktop PUBLIC
    ARDUINO=10800
    PLATFORM_DESKTOP=1
    VERSION=4080
    VERSION_STR="R4.80-desktop"
    MCL_DESKTOP_LINK_MCL_CORE=1)

# IMPORTANT: desktop_common BEFORE desktop BEFORE other includes so MCL's
# `#include "Arduino.h"` resolves to OUR shim first. desktop/ paths win for
# anything overridden per-platform (none currently).
target_include_directories(mcl_desktop BEFORE PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/desktop
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/desktop_common)

target_include_directories(mcl_desktop PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    # MCL's tbd env -I list verbatim — keep in sync with platformio.ini:51-79.
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/ResourceManager
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/A4
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/MCL
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/MD
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/MNM
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/A4
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/A4/GridTracks
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/A4/Sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/Generic
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/Generic/GridTracks
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/Generic/Sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MD
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MD/GridTracks
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MD/Sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MD/UI/Pages
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MD/UI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MNM
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Drivers/MNM/GridTracks
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Midi
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/CommonTools
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Fonts
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Diagnostic
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Elektron
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/MidiTools
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mcl/Tests

    # Bundled Arduino libs from PlatformIO's libdeps.
    "${LIBDEPS}/Adafruit GFX Library"

    # Pre-generated resource header (R.h) and its module headers.
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/rp2040)

# Less-strict warning regime — MCL is firmware-style code that the desktop
# compiler will flag for unused params, narrowing conversions, etc.
target_compile_options(mcl_desktop PRIVATE
    -Wno-deprecated-declarations
    -Wno-unused-parameter
    -Wno-write-strings
    -Wno-narrowing
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-sign-compare
    -fpermissive)

# Smoke-test executable: links libmcl_desktop.a into a tiny main() that calls
# mcl_desktop_setup()/tick() once. Surfaces missing-symbol errors that ar
# wouldn't catch. Build with
# `cmake --build BUILD_DIR --target mcl_desktop_linktest`.
add_executable(mcl_desktop_linktest EXCLUDE_FROM_ALL
    src/platform/desktop/linktest_main.cpp)
target_link_libraries(mcl_desktop_linktest PRIVATE mcl_desktop)
