#pragma once

// Standard includes
#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include <stdio.h>

// Debug configuration
#define DEBUGMODE
#define DEBUG_PIN 2
#define SERIAL_SPEED 115200

// Platform compatibility defines
#ifndef PROGMEM
#define PROGMEM   // Empty macro for non-AVR platforms
#endif

#ifndef pgm_read_byte_near
#define pgm_read_byte_near(x) (*(x))
#endif

#ifndef _BV
#define _BV(bit) (1UL << (bit))
#endif

// Interrupt locking mechanisms
extern volatile uint32_t interrupt_lock_count;

#define USE_LOCK()
#define SET_LOCK() uint32_t state = save_and_disable_interrupts()
#define CLEAR_LOCK() restore_interrupts_from_disabled(state)
#define LOCK() USE_LOCK(); SET_LOCK()

// Function inlining configuration
#if defined(MEGACOMMAND) && defined(IS_ISR_ROUTINE)
    #define ALWAYS_INLINE() __attribute__((always_inline))
    #define FORCED_INLINE() __attribute__((always_inline))
#elif defined(MEGACOMMAND)
    #define ALWAYS_INLINE()
    #define FORCED_INLINE() __attribute__((always_inline))
#else
    #define ALWAYS_INLINE() inline
    #define FORCED_INLINE() __attribute__((always_inline))
#endif

// C++ specific functionality
#ifdef __cplusplus

#include "DebugBuffer.h"
extern DebugBuffer debugBuffer;

// Interrupt detection function
inline bool isInInterrupt() {
    uint32_t ipsr;
    __asm volatile ("mrs %0, IPSR" : "=r" (ipsr));
    return ipsr != 0;  // Non-zero value means we're in an exception/interrupt handler
}

// Debug macros - only active when DEBUGMODE is defined
#ifdef DEBUGMODE
    // Initialize debug serial port
    #define DEBUG_INIT() Serial.begin(SERIAL_SPEED)

    // Print line with context awareness
    #define DEBUG_PRINTLN(x) do { \
        if (isInInterrupt()) { \
            debugBuffer.put(x); \
            debugBuffer.put("\n"); \
        } else { \
            Serial.println(x); \
            Serial.flush(); \
        } \
    } while(0)

    // Print without newline, with context awareness
    #define DEBUG_PRINT(x) do { \
        if (isInInterrupt()) { \
            debugBuffer.put(x); \
        } else { \
            Serial.print(x); \
            Serial.flush(); \
        } \
    } while(0)

    // Print function name with format string
    #define DEBUG_FUNC(fmt, ...) do { \
        if (isInInterrupt()) { \
            char buf[64]; \
            snprintf(buf, sizeof(buf), "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
            debugBuffer.put(buf); \
        } else { \
            Serial.print(__func__); \
            Serial.print(": "); \
            Serial.println(fmt); \
            Serial.flush(); \
        } \
    } while(0)

#else // DEBUGMODE not defined
    #define DEBUG_INIT()
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_FUNC(fmt)
#endif // DEBUGMODE

#else // __cplusplus not defined

// C-compatible empty debug macros
#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_FUNC(fmt)

#endif // __cplusplus
