// platform.h — desktop platform-abstraction header. Mirrors
// MCL/src/platform/rp2040/platform.h but maps everything to no-ops or std::.
// Included by MCL source via #include "platform.h"; the desktop include path
// must come BEFORE rp2040/avr so this version wins.
#pragma once

#include "Arduino.h"
#include "hardware.h"
// MCL's helpers.h supplies read_clock_ms()/clock_diff()/Task.h. rp2040's
// platform.h includes it via the same name; we match that contract so
// MCL source can rely on platform.h pulling helpers in.
#include "helpers.h"
#include <stdio.h>

// time_us_32() / time_us_64() are Pico SDK names that MCL's Diagnostic code
// uses for ISR timing. Map to micros() on desktop. Defined as inline on the
// C++ side; helpers.c doesn't reference them.
#ifdef __cplusplus
inline uint32_t time_us_32() { return (uint32_t)micros(); }
inline uint64_t time_us_64() { return micros(); }
#endif

#ifdef __cplusplus
#include "int24.h"
#endif

#define MCL_CACHE_USE_ARRAYS 1

#ifndef ENCODER_RES_MULTIPLIER
#define ENCODER_RES_MULTIPLIER 9
#endif

// Debug pin / serial speed — not used on desktop but referenced by some
// macros lifted from rp2040.
#define DEBUG_PIN 2
#define SERIAL_SPEED 9600

#define DEFAULT_TURBO_SPEED 5

// Interrupt locking — no-op on desktop (single-threaded MCL).
#define USE_LOCK()
#define SET_LOCK()
#define CLEAR_LOCK()
#define LOCK() USE_LOCK(); SET_LOCK()

#define ATTR_PACKED()    __attribute__((packed))
#define ALWAYS_INLINE()  inline __attribute__((always_inline))
#define FORCED_INLINE()  inline __attribute__((always_inline))
#define NOINLINE()       __attribute__((noinline))

// Software-IRQ trigger macros — empty stubs (no IRQs).
extern uint8_t SW_IRQ1;
extern uint8_t SW_IRQ2;
extern uint8_t SW_IRQ3;

#define TRIGGER_SW_IRQ1() ((void)0)
#define TRIGGER_SW_IRQ2() ((void)0)
#define TRIGGER_SW_IRQ3() ((void)0)
#define CLEAR_SW_IRQ1()   ((void)0)
#define CLEAR_SW_IRQ2()   ((void)0)
#define CLEAR_SW_IRQ3()   ((void)0)

#ifdef __cplusplus

template <typename T>
inline T atomic_read(const volatile T* ptr) { return *ptr; }

// debugBuffer is declared by MCL's own src/mcl/Diagnostic/DebugBuffer.h
// (a Stream-derived class). The desktop platform doesn't need to declare it
// here; the include order in global.cpp picks up MCL's definition before
// global.cpp instantiates the singleton.
class DebugBuffer;
extern DebugBuffer debugBuffer;

inline bool isInInterrupt() { return false; }

// MCL's per-platform DEBUG_INIT/DEBUG_PRINT* macros. When DEBUGMODE is set,
// route the existing callsites to a stdio sink. In wasm, stdio forwards to the
// host_log import, so the host receives the same stream on stderr/log output.
#ifdef DEBUGMODE
    #include "DebugOutput.h"
    #define DEBUG_INIT()             do {} while (0)
    #define DEBUG_PRINTLN(...)       do { mcl_debug::println(__VA_ARGS__); } while (0)
    #define DEBUG_PRINT(...)         do { mcl_debug::print(__VA_ARGS__); } while (0)
    #define DEBUG_PRINT_FN(...)      do { mcl_debug::function(__func__, ##__VA_ARGS__); } while (0)
    #define DEBUG_DUMP(x)
#else
    #define DEBUG_INIT()
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINT_FN(fmt, ...)
    #define DEBUG_DUMP(x)
#endif

#else  // __cplusplus

#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT_FN(fmt, ...)

#endif // __cplusplus
