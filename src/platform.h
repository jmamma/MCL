#pragma once
#include <Arduino.h>

#define USE_LOCK()
#define SET_LOCK() noInterrupts();
#define CLEAR_LOCK() interrupts();

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
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  // If debug mode is off, these become no-ops
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
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

