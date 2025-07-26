#pragma once

// Standard includes
#include "Arduino.h"
#include "hardware.h"
#include "helpers.h"
#include <stdio.h>

#define IS_MEGACMD() IS_BIT_CLEAR(PINK,PK2)
#define SET_USB_MODE(x) { PORTK = ((x)); }

#define LOCAL_SPI_ENABLE() { DDRB = 0xFF; PORTL |= _BV(PL4); }
#define LOCAL_SPI_DISABLE() { DDRB = 0; PORTB = 0; }

#define EXTERNAL_SPI_ENABLE() { PORTL |= _BV(PL3); }
#define EXTERNAL_SPI_DISABLE() { PORTL &= ~_BV(PL3); }

// Debug configuration

#define SERIAL_SPEED 250000

#ifndef _BV
#define _BV(bit) (1u << (bit))
#endif

// Interrupt locking mechanisms
extern volatile uint32_t interrupt_lock_count;

#include <avr/interrupt.h>

#define USE_LOCK()   uint8_t _irqlock_tmp
#define SET_LOCK()   _irqlock_tmp = SREG; cli()
#define CLEAR_LOCK() SREG = _irqlock_tmp
#define LOCK() USE_LOCK(); SET_LOCK()

#include <avr/pgmspace.h>

#define time_us_32() g_clock_fast
#define sleep_ms(x) delay(x)

// Function inlining configuration
#if defined(MEGACOMMAND) && defined(IS_ISR_ROUTINE)
    #define ALWAYS_INLINE() __attribute__((always_inline))
    #define FORCED_INLINE() __attribute__((always_inline))
#elif defined(MEGACOMMAND)
    #define ALWAYS_INLINE()
    #define FORCED_INLINE() __attribute__((always_inline))
#else
    #define ALWAYS_INLINE() __attribute__((always_inline))
    #define FORCED_INLINE() __attribute__((always_inline))
#endif

// C++ specific functionality
#ifdef __cplusplus

#include "new.h"
#include "DebugBuffer.h"
extern DebugBuffer debugBuffer;

// Interrupt detection function
inline bool isInInterrupt() {
    return !(SREG & (1 << SREG_I));  // Return true if Global Interrupt Enable bit is cleared
}

// Debug macros - only active when DEBUGMODE is defined
#ifdef DEBUGMODE

    #define DEBUG_CHECK_STACK() { if ((int) SP < 0x200 || (int)SP > 0x2200) { cli(); setLed2(); setLed(); while (1); } }
    // Initialize debug serial port

    #define DEBUG_INIT() do { change_usb_mode(0x03);  MidiUartUSB.mode = UART_SERIAL;  MidiUartUSB.init(); MidiUartUSB.set_speed(SERIAL_SPEED); } while(0)

    // Print line with context awareness
    #define DEBUG_PRINTLN(x) do { debugBuffer.println(x); } while(0)
    #define DEBUG_PRINT(x) do { debugBuffer.print(x); } while(0)
    /*
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
*/
   #define DEBUG_PRINT_FN(fmt)
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

