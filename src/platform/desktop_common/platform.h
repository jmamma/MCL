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

// Platform poll hook. Wasm uses this as a cooperative hardware-service point
// while modal page loops are running; native desktop and hardware
// implementations may no-op or use their real platform services.
void platform_poll();
void platform_wait_poll();

// Hosted platforms can request non-interactive boot behavior. Native desktop
// keeps the normal firmware UI flow; wasm delegates the choice to its host so
// embedded products can auto-load while interactive products show startup UI.
#define MCL_HAS_PLATFORM_HEADLESS_BOOT 1
bool mcl_platform_headless_boot();

// Platform-owned panel input source for the shared desktop GUI hardware shim.
// The native desktop path uses local mock state; wasm forwards to host imports.
uint64_t mcl_platform_button_mask();
int      mcl_platform_encoder_delta(uint8_t encoder_id);
uint32_t mcl_platform_encoder_button_mask();

#if defined(MCL_HAS_DESKTOP_MOUSE)
enum mcl_mouse_event_type_t : uint8_t {
    MCL_MOUSE_MOVE = 0,
    MCL_MOUSE_DOWN = 1,
    MCL_MOUSE_UP = 2,
    MCL_MOUSE_DRAG = 3,
    MCL_MOUSE_DOUBLE_CLICK = 4,
    MCL_MOUSE_WHEEL = 5,
    MCL_MOUSE_EXIT = 6
};

enum mcl_mouse_button_t : uint8_t {
    MCL_MOUSE_BUTTON_LEFT = 1u << 0,
    MCL_MOUSE_BUTTON_RIGHT = 1u << 1,
    MCL_MOUSE_BUTTON_MIDDLE = 1u << 2
};

enum mcl_mouse_modifier_t : uint8_t {
    MCL_MOUSE_MODIFIER_SHIFT = 1u << 0,
    MCL_MOUSE_MODIFIER_CTRL = 1u << 1,
    MCL_MOUSE_MODIFIER_ALT = 1u << 2,
    MCL_MOUSE_MODIFIER_COMMAND = 1u << 3
};

typedef struct mcl_mouse_event_s {
    uint8_t type;
    uint8_t buttons;
    uint8_t modifiers;
    uint8_t reserved;
    int16_t x;
    int16_t y;
    int16_t deltaX;
    int16_t deltaY;
} mcl_mouse_event_t;

bool mcl_platform_mouse_pop(mcl_mouse_event_t* out);
#endif

// MCL's per-platform DEBUG_INIT/DEBUG_PRINT* macros. When DEBUGMODE is set,
// route the existing callsites through DebugBuffer and flush from platform
// poll points so wasm/desktop debug spam is bounded.
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
