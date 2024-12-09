#pragma once

// Core RP2040 includes
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/timer.h"  // If you need timers
#include "hardware/gpio.h"   // If you need GPIO
                             //
#if defined(MEGACOMMAND) && defined(IS_ISR_ROUTINE)
  #define ALWAYS_INLINE() __attribute__((always_inline))
  #define FORCED_INLINE() __attribute__((always_inline))
#elif defined(MEGACOMMAND)
  #define ALWAYS_INLINE()
  #define FORCED_INLINE() __attribute__((always_inline))
#else
  #define ALWAYS_INLINE()
  #define FORCED_INLINE()
#endif

