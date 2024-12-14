#pragma once

#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include <stdio.h>

#define DEBUGMODE
#define DEBUG_PIN 2

extern volatile uint32_t interrupt_lock_count;
/*
#define SET_LOCK() \
    uint32_t _saved_state; \
    if (interrupt_lock_count++ == 0) { \
        _saved_state = save_and_disable_interrupts(); \
    }

#define CLEAR_LOCK() \
    if (--interrupt_lock_count == 0) { \
        restore_interrupts(_saved_state); \
    }

*/
#define USE_LOCK()

#define SET_LOCK() uint32_t state = save_and_disable_interrupts()
#define CLEAR_LOCK() restore_interrupts_from_disabled(state)
#define LOCK() USE_LOCK(); SET_LOCK()

#ifndef PROGMEM
#define PROGMEM   // Empty macro for non-AVR platforms
#endif

#ifndef pgm_read_byte_near
#define pgm_read_byte_near(x) (*(x))
#endif

#define SERIAL_SPEED 115200

#ifdef __cplusplus

#include "DebugBuffer.h"
extern DebugBuffer debugBuffer;

inline bool isInInterrupt() {
   uint32_t ipsr;
    __asm volatile ("mrs %0, IPSR" : "=r" (ipsr));
    return ipsr != 0;  // Non-zero value means we're in an exception/interrupt handler
}

#ifdef DEBUGMODE
    #define DEBUG_INIT() Serial.begin(SERIAL_SPEED);
#define DEBUG_PRINTLN(x) do { \
    if (isInInterrupt()) { \
        debugBuffer.put(x); \
        debugBuffer.put("\n"); \
    } else { \
        Serial.println(x); \
        Serial.flush(); \
    } \
} while(0)
#define DEBUG_PRINT(x) do { \
    if (isInInterrupt()) { \
        debugBuffer.put(x); \
    } else { \
        Serial.print(x); \
        Serial.flush(); \
    } \
} while(0)
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

#else
    #define DEBUG_INIT()
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_FUNC(fmt)
#endif

#else
    #define DEBUG_INIT()
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_FUNC(fmt)
#endif

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

