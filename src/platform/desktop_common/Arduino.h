// Arduino.h — desktop platform shim.
//
// Force-included by virtue of MCL/src/platform/desktop being on the include
// path BEFORE any system Arduino headers (see MCL/CMakeLists.txt). MCL source
// is unmodified; this header provides just enough of the Arduino API surface
// to make MCL's cross-platform code compile and link on macOS/Linux/Windows.
//
// Conventions:
//   - PROGMEM / F() / pgm_read_* are no-ops (flat memory, no Harvard split).
//   - delay/noInterrupts/sei/cli are no-ops (no audio-thread blocking and no
//     real interrupt context on desktop).
//   - GPIO calls read/write a process-global mock array in hardware.cpp.
//   - millis()/micros() use std::chrono::steady_clock.
//   - SPI is a no-op class — display drivers compute the framebuffer in RAM
//     and only flush via SPI.transfer(), which we drop.
//   - Wire (I2C) isn't used by MCL's core paths; provided as an empty header.
#pragma once

// C-and-C++ compatible headers. MCL has helpers.c which includes Arduino.h,
// so we can't use <c*> variants unconditionally.
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
// std::algorithm only needed for callers using std::min/std::max. The
// min/max templates below are at global scope. wasm32 build doesn't have
// libc++ available.
#if defined(__cplusplus) && !defined(PLATFORM_WASM)
#include <algorithm>
#endif
// <new> for placement-new (MCL/src/mcl/MCL/DeviceTrack.cpp:26 etc.).
// Pulled in unconditionally on C++ so source doesn't need to include it.
#ifdef __cplusplus
#include <new>
#endif

// PROGMEM / flash-string family — empty on desktop. ~520 callsites across
// MCL collapse to plain memory access.
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PSTR
#define PSTR(s) (s)
#endif
#ifndef F
#define F(s) (s)
#endif
// __FlashStringHelper is normally an opaque forward-declared class on Arduino
// so overloads like `put(const char*)` and `put(const __FlashStringHelper*)`
// remain distinct. Don't typedef it to char — that would collapse the
// overloads (see MCL/src/mcl/Diagnostic/DebugBuffer.h:27-28). C++ only.
#ifdef __cplusplus
class __FlashStringHelper;
#endif
typedef const char* PGM_P;
typedef const void* PGM_VOID_P;

// C-cast (not reinterpret_cast) so helpers.c can use the same macros.
#ifndef pgm_read_byte
#define pgm_read_byte(p)       (*(const uint8_t*)(p))
#endif
#ifndef pgm_read_byte_near
#define pgm_read_byte_near(p)  (*(const uint8_t*)(p))
#endif
#ifndef pgm_read_word
#define pgm_read_word(p)       (*(const uint16_t*)(p))
#endif
#ifndef pgm_read_word_near
#define pgm_read_word_near(p)  (*(const uint16_t*)(p))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(p)      (*(const uint32_t*)(p))
#endif
#ifndef pgm_read_dword_near
#define pgm_read_dword_near(p) (*(const uint32_t*)(p))
#endif
#ifndef pgm_read_ptr
#define pgm_read_ptr(p)        (*(const void* const*)(p))
#endif

#ifndef memcpy_P
#define memcpy_P(dst, src, n)  memcpy((dst), (src), (n))
#endif
#ifndef strcpy_P
#define strcpy_P(dst, src)     strcpy((dst), (src))
#endif
#ifndef strncpy_P
#define strncpy_P(dst, src, n) strncpy((dst), (src), (n))
#endif
#ifndef strlen_P
#define strlen_P(s)            strlen(s)
#endif
#ifndef strcmp_P
#define strcmp_P(a, b)         strcmp((a), (b))
#endif

// Bit helpers — AVR-flavoured names MCL uses.
#ifndef _BV
#define _BV(bit) (1u << (bit))
#endif

// GPIO mode constants
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef INPUT
#define INPUT 0
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 2
#endif

// Arduino math constants — math.h's M_PI isn't visible in std mode for some
// configurations, and MCL/src/mcl/MCL/Osc.cpp uses bare `PI`.
#ifndef PI
#define PI    3.1415926535897932384626433832795
#endif
#ifndef HALF_PI
#define HALF_PI 1.5707963267948966192313216916398
#endif
#ifndef TWO_PI
#define TWO_PI 6.283185307179586476925286766559
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.295779513082320876798154814105
#endif

// Print bases used by Print::print(int, base)
#ifndef DEC
#define DEC 10
#endif
#ifndef HEX
#define HEX 16
#endif
#ifndef OCT
#define OCT 8
#endif
#ifndef BIN
#define BIN 2
#endif

// boolean / byte typedefs Arduino code expects. C doesn't define `bool`
// without <stdbool.h>; pull it in unconditionally to make `boolean` work
// in both .c and .cpp.
#include <stdbool.h>
typedef uint8_t byte;
typedef bool    boolean;

#ifdef __cplusplus
extern "C" {
#endif

// Time
unsigned long millis(void);
unsigned long micros(void);
void          delay(unsigned long ms);
void          delayMicroseconds(unsigned int us);

// GPIO mock surface — backed by desktop_gpio_state[] in hardware.cpp.
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
inline void yield() {}
inline int  analogRead(uint8_t /*pin*/)            { return 0; }
inline void analogWrite(uint8_t /*pin*/, int /*v*/) {}
inline void analogReference(uint8_t /*mode*/)       {}

// Interrupt control — desktop platform has no ISRs to disable.
inline void sei()           {}
inline void cli()           {}
inline void interrupts()    {}
inline void noInterrupts()  {}
#else
#define yield()              ((void)0)
#define analogRead(pin)      (0)
#define analogWrite(pin, v)  ((void)0)
#define analogReference(m)   ((void)0)
#define sei()                ((void)0)
#define cli()                ((void)0)
#define interrupts()         ((void)0)
#define noInterrupts()       ((void)0)
#endif
#ifndef attachInterrupt
#define attachInterrupt(...) ((void)0)
#endif
#ifndef detachInterrupt
#define detachInterrupt(...) ((void)0)
#endif

// Utility helpers. Arduino's traditional min(a, b) / max(a, b) are macros
// that clash with std::min / std::max. Use proper templates at global scope
// instead — MCL's `min(x, y)` callsites compile, and code that wants
// `std::min` still works.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef __cplusplus
template <typename A, typename B>
inline auto min(const A& a, const B& b) -> decltype(a < b ? a : b) { return a < b ? a : b; }
template <typename A, typename B>
inline auto max(const A& a, const B& b) -> decltype(a > b ? a : b) { return a > b ? a : b; }
#else
// C-side fallback: macros (Arduino's classic shape). C code that uses
// these on side-effecting expressions is rare in MCL's helpers.c.
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (b) : (b))
#endif

#ifndef abs
#define abs(x) ((x) < 0 ? -(x) : (x))
#endif
#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif
#ifndef map
#define map(value, fromLow, fromHigh, toLow, toHigh) \
    ((long)(value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow)
#endif

#ifndef lowByte
#define lowByte(w)  ((uint8_t)((w) & 0xff))
#endif
#ifndef highByte
#define highByte(w) ((uint8_t)(((w) >> 8) & 0xff))
#endif
#ifndef bitRead
#define bitRead(v, b)  (((v) >> (b)) & 0x01)
#endif
#ifndef bitSet
#define bitSet(v, b)   ((v) |= (1UL << (b)))
#endif
#ifndef bitClear
#define bitClear(v, b) ((v) &= ~(1UL << (b)))
#endif
#ifndef bitWrite
#define bitWrite(v, b, x) ((x) ? bitSet(v, b) : bitClear(v, b))
#endif

#ifndef word
#define word(h, l) (((uint16_t)(h) << 8) | (uint8_t)(l))
#endif

#ifdef __cplusplus
// Random — Arduino's random(min, max) is half-open; std::rand wrapper is
// fine for MCL's purposes (UI animation seeds).
inline long random(long howbig) {
    if (howbig <= 0) return 0;
    return rand() % howbig;
}
inline long random(long howsmall, long howbig) {
    if (howsmall >= howbig) return howsmall;
    return howsmall + (rand() % (howbig - howsmall));
}
inline void randomSeed(unsigned long seed) { srand((unsigned int)seed); }

// Print / Stream / Serial / WString — pulled in so user code that includes
// just <Arduino.h> sees the full surface. C++ only.
#include "WString.h"
#include "Print.h"
#include "Stream.h"
#include "HardwareSerial.h"
#endif  // __cplusplus
