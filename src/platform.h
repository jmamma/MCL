#pragma once
#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#define DEBUGMODE

#define USE_LOCK()
#define SET_LOCK() uint32_t state = save_and_disable_interrupts()
#define CLEAR_LOCK() restore_interrupts_from_disabled(state)
#define LOCK() USE_LOCK(); SET_LOCK()

#ifndef F
#define F(str) (str)
#endif

#ifndef PROGMEM
#define PROGMEM   // Empty macro for non-AVR platforms
#endif

#ifndef pgm_read_byte_near
#define pgm_read_byte_near(x) (*(x))
#endif

#define SERIAL_SPEED 115200

#ifdef DEBUGMODE
  // For ARM, we can use Serial for debug output
  #define DEBUG_INIT() Serial.begin(SERIAL_SPEED);
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) do { Serial.println(x); } while(0)
  #define DEBUG_FUNC(fmt) do { Serial.print(__func__); Serial.print(": "); Serial.println(fmt); } while(0)
  //#define DEBUG_PRINTLN(x) do { Serial.println(x); Serial.flush(); } while(0)
  //#define DEBUG_FUNC(fmt) do { Serial.print(__func__); Serial.print(": "); Serial.println(fmt); Serial.flush(); } while(0)
#else
  // If debug mode is off, these become no-ops
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_FUNC(...)
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

