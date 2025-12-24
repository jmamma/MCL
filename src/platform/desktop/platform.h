#pragma once

// Desktop platform stub for MCL library
// Provides minimal compatibility layer for building on desktop

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Attributes - MUST be defined before any includes that use them
// ============================================================================
#define ATTR_PACKED()
#define ALWAYS_INLINE()
#define FORCED_INLINE()

// ============================================================================
// Interrupt locking (no-op on desktop)
// ============================================================================
#define USE_LOCK()
#define SET_LOCK()
#define CLEAR_LOCK()
#define LOCK()

// ============================================================================
// Timing
// ============================================================================
#define time_us_32() 0
#define sleep_ms(x)

// ============================================================================
// Bit manipulation
// ============================================================================
#ifndef _BV
#define _BV(bit) (1u << (bit))
#endif

// ============================================================================
// Program memory (no-op on desktop - all memory is RAM)
// ============================================================================
#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_byte_near(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#define pgm_read_word_near(addr) (*(const uint16_t*)(addr))
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#define pgm_read_ptr(addr) (*(const void**)(addr))

// ============================================================================
// String functions
// ============================================================================
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strlen_P strlen
#define strcmp_P strcmp

// ============================================================================
// Memory bank operations (embedded-specific, no-op on desktop)
// These MUST be defined before RingBuffer.h is included
// ============================================================================
template<typename T>
inline void put_bank1(volatile T* addr, T val) { *addr = val; }

template<typename T>
inline T get_bank1(volatile T* addr) { return *addr; }

inline void memcpy_bank1(volatile void* dest, const volatile void* src, size_t n) {
    memcpy(const_cast<void*>(dest), const_cast<const void*>(src), n);
}

inline int memcmp_bank1(const volatile void* a, const volatile void* b, size_t n) {
    return memcmp(const_cast<const void*>(a), const_cast<const void*>(b), n);
}

// ============================================================================
// Min/max - use templates to avoid macro issues with std::min/max
// ============================================================================
template<typename T>
inline T min(T a, T b) { return a < b ? a : b; }
template<typename T>
inline T max(T a, T b) { return a > b ? a : b; }

// ============================================================================
// Volatile register dummy
// ============================================================================
#define volatile_write(x, v) ((x) = (v))
#define volatile_read(x) (x)

// ============================================================================
// Now include platform-specific stubs that may use the above definitions
// ============================================================================
#include "MidiUart.h"
#include "GUI_hardware.h"

// ============================================================================
// LED functions (no-op on desktop)
// ============================================================================
inline void setLed() {}
inline void clearLed() {}
inline void setLed2() {}
inline void clearLed2() {}

// ============================================================================
// Device and color constants
// ============================================================================
#define NUM_DEVS 4
#define WHITE 1
#define BLACK 0
#define HEX 16
// Note: DEC is not defined to avoid conflict with machine parameter names

// ============================================================================
// Version string
// ============================================================================
#define VERSION_STR "1.0.0"

// ============================================================================
// GUI class stub - only define if HOST_GUI is not set (host has own GUI)
// ============================================================================
#ifndef HOST_GUI

class LightPage;
class Page;

class GUIClass {
public:
    bool display_mirror = false;

    LightPage* currentPage() { return nullptr; }
    void loop() {}
    void init() {}
    void init(void*) {}
    void display() {}
    void setPage(void*) {}
    void setPage(Page*) {}
    void pushPage(void*) {}
    void pushPage(Page*) {}
    void popPage() {}
    bool hasPage(void*) { return false; }
    bool hasPage(Page*) { return false; }
};
inline GUIClass GUI;

#endif // HOST_GUI

// ============================================================================
// OLED display stub
// ============================================================================
class OLEDDisplayClass {
public:
    void clear() {}
    void display() {}
    void print(const char*) {}
    void print(int) {}
    void print(uint16_t, int) {}
    void println(const char*) {}
    void println(int) {}
    void setCursor(int, int) {}
    void setTextSize(int) {}
    void setTextColor(int) {}
    void textbox(const char*, const char*) {}
};
inline OLEDDisplayClass oled_display;

// Note: MidiClass is defined in MCL/Midi/Midi.h

// ============================================================================
// Callback limits
// ============================================================================
#define NUM_CLOCK_CALLBACKS 8

// ============================================================================
// Memory bank constants (embedded-specific)
// ============================================================================
#define BANK1_SYSEX3_DATA_START 0
#define SYSEX3_DATA_LEN 0x10000
#define BANK3_START 0
#define BANK3_END 0x10000

// ============================================================================
// Debug macros (no-op on desktop)
// ============================================================================
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#define DEBUG_DUMP(x)
#define DEBUG_PRINT_FN()
