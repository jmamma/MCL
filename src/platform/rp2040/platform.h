#pragma once

// Standard includes
#include "Arduino.h"
#include "hardware.h"
#include "helpers.h"
#include <stdio.h>

// Platform specific includes
#include "pico.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/structs/pio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
#include "int24.h"
#endif

#define MCL_CACHE_USE_ARRAYS 1

// Debug configuration

#define DEBUG_PIN 2
#define SERIAL_SPEED 9600

// Platform compatibility defines
#ifndef PROGMEM
#define PROGMEM   // Empty macro for non-AVR platforms
#endif

#ifndef pgm_read_byte_near
#define pgm_read_byte_near(x) (*(x))
#endif

#ifndef _BV
#define _BV(bit) (1u << (bit))
#endif

// Interrupt locking mechanisms
extern volatile uint32_t interrupt_lock_count;

#define USE_LOCK()
#define SET_LOCK() uint32_t state = save_and_disable_interrupts()
#define CLEAR_LOCK() restore_interrupts_from_disabled(state)
#define LOCK() USE_LOCK(); SET_LOCK()

#define ATTR_PACKED() __attribute__((packed))

#define ALWAYS_INLINE() __attribute__((always_inline))
#define FORCED_INLINE() __attribute__((always_inline))

extern uint8_t SW_IRQ1;
extern uint8_t SW_IRQ2;
extern uint8_t SW_IRQ3;

#define TRIGGER_SW_IRQ1()  irq_set_pending(SW_IRQ1)
#define TRIGGER_SW_IRQ2()  irq_set_pending(SW_IRQ2)
#define TRIGGER_SW_IRQ3()  irq_set_pending(SW_IRQ3)

#define CLEAR_SW_IRQ1()    irq_clear(SW_IRQ1)
#define CLEAR_SW_IRQ2()    irq_clear(SW_IRQ2)
#define CLEAR_SW_IRQ3()    irq_clear(SW_IRQ3)

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
    #define DEBUG_INIT() do { Serial.begin(SERIAL_SPEED); delay(1000); } while(0)

    // Print line with context awareness
    #define DEBUG_PRINTLN(x) do { \
        if (isInInterrupt()) { \
            debugBuffer.put(x); \
            debugBuffer.put("\n"); \
        } else { \
            debugBuffer.println(x); \
            debugBuffer.transmit(); \
        } \
    } while(0)

    // Print without newline, with context awareness
    #define DEBUG_PRINT(x) do { \
        if (isInInterrupt()) { \
            debugBuffer.put(x); \
        } else { \
            debugBuffer.print(x); \
            debugBuffer.transmit(); \
        } \
    } while(0)

    // Print function name with format string
    #define DEBUG_PRINT_FN(fmt, ...) do { \
        if (isInInterrupt()) { \
            char buf[64]; \
            snprintf(buf, sizeof(buf), "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
            debugBuffer.put(buf); \
        } else { \
            debugBuffer.print(__func__); \
            debugBuffer.print(": "); \
            debugBuffer.println(fmt); \
            debugBuffer.transmit(); \
        } \
    } while(0)
    #define DEBUG_DUMP(x)  { \
    }
#else // DEBUGMODE not defined
    #define DEBUG_INIT()
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINT_FN(fmt)
    #define DEBUG_DUMP(x)
#endif // DEBUGMODE

#else // __cplusplus not defined

// C-compatible empty debug macros
#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT_FN(fmt)

#endif // __cplusplus
