/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus

extern "C" {
#endif
#define IN_IRQ()  !IS_BIT_SET(SREG, SREG_I)

extern const uint8_t _bvmasks[];
extern const uint8_t _ibvmasks[];
extern const uint32_t _bvmasks32[];

/**
 * \addtogroup helpers_bitmanipulation Bit-manipulation macros
 * @{
 **/

#define ROTATE_LEFT(target, length) do { target = (target >> 1) | (target << (length - 1)); } while (false)
#define ROTATE_RIGHT(target, length) do { target = (target << 1) | (target >> (length - 1)); } while (false)

/** 32 bit bit accessing macro. **/
#define _BV32(i) (_bvmasks32[i])

/** Bit-level access and test macros. **/
#define SET_BIT(target, bit)                do { (target) |=  _bvmasks[((uint8_t)(bit))]; } while (false)
#define CLEAR_BIT(target, bit)              do { (target) &= _ibvmasks[((uint8_t)(bit))]; } while (false)
#define TOGGLE_BIT(target, bit)             do { (target) ^=  _bvmasks[((uint8_t)(bit))]; } while (false)
#define IS_BIT_SET(target, bit)          ((bool)((target) &   _bvmasks[((uint8_t)(bit))]))
#define IS_BIT_CLEAR(target, bit)       (!(bool)((target) &   _bvmasks[((uint8_t)(bit))]))

/** 8-bit macros. **/
#define SET_BIT8(target, bit)               SET_BIT(target,bit)
#define CLEAR_BIT8(target, bit)             CLEAR_BIT(target,bit)
#define TOGGLE_BIT8(target, bit)            TOGGLE_BIT(taget, bit)
#define IS_BIT_SET8(target, bit)            IS_BIT_SET(target, bit)
#define IS_BIT_CLEAR8(target, bit)          IS_BIT_CLEAR(target, bit)

#define SET_BIT_RADDR(target, bit)          do { ((uint8_t*)&(target))[((uint8_t)(bit)) / 0b00001000] |=  _bvmasks[((uint8_t)(bit)) & 0b00000111]; } while (false)
#define CLEAR_BIT_RADDR(target, bit)        do { ((uint8_t*)&(target))[((uint8_t)(bit)) / 0b00001000] &= _ibvmasks[((uint8_t)(bit)) & 0b00000111]; } while (false)
#define TOGGLE_BIT_RADDR(target, bit)       do { ((uint8_t*)&(target))[((uint8_t)(bit)) / 0b00001000] ^=  _bvmasks[((uint8_t)(bit)) & 0b00000111]; } while (false)
#define IS_BIT_SET_RADDR(target, bit)    ((bool)(((uint8_t*)&(target))[((uint8_t)(bit)) / 0b00001000] &   _bvmasks[((uint8_t)(bit)) & 0b00000111]))
#define IS_BIT_CLEAR_RADDR(target, bit) (!(bool)(((uint8_t*)&(target))[((uint8_t)(bit)) / 0b00001000] &   _bvmasks[((uint8_t)(bit)) & 0b00000111]))

#define SET_BIT_RADDR_P(target, bit)          do { ((uint8_t*)(target))[((uint8_t)(bit)) / 0b00001000] |=  _bvmasks[((uint8_t)(bit)) & 0b00000111]; } while (false)
#define CLEAR_BIT_RADDR_P(target, bit)        do { ((uint8_t*)(target))[((uint8_t)(bit)) / 0b00001000] &= _ibvmasks[((uint8_t)(bit)) & 0b00000111]; } while (false)
#define TOGGLE_BIT_RADDR_P(target, bit)       do { ((uint8_t*)(target))[((uint8_t)(bit)) / 0b00001000] ^=  _bvmasks[((uint8_t)(bit)) & 0b00000111]; } while (false)
#define IS_BIT_SET_RADDR_P(target, bit)    ((bool)(((uint8_t*)(target))[((uint8_t)(bit)) / 0b00001000] &   _bvmasks[((uint8_t)(bit)) & 0b00000111]))
#define IS_BIT_CLEAR_RADDR_P(target, bit) (!(bool)(((uint8_t*)(target))[((uint8_t)(bit)) / 0b00001000] &   _bvmasks[((uint8_t)(bit)) & 0b00000111]))

/** 16-bit macros. **/
#define SET_BIT16(target, bit)              SET_BIT_RADDR(target, bit)
#define CLEAR_BIT16(target, bit)            CLEAR_BIT_RADDR(target, bit)
#define TOGGLE_BIT16(target, bit)           TOGGLE_BIT_RADDR(target, bit)
#define IS_BIT_SET16(target, bit)           IS_BIT_SET_RADDR(target, bit)
#define IS_BIT_CLEAR16(target, bit)         IS_BIT_CLEAR_RADDR(target, bit)

/** 24-bit macros. **/
#define SET_BIT24(target, bit)              SET_BIT_RADDR(target, bit)
#define CLEAR_BIT24(target, bit)            CLEAR_BIT_RADDR(target, bit)
#define TOGGLE_BIT24(target, bit)           TOGGLE_BIT_RADDR(target, bit)
#define IS_BIT_SET24(target, bit)           IS_BIT_SET_RADDR(target, bit)
#define IS_BIT_CLEAR24(target, bit)         IS_BIT_CLEAR_RADDR(target, bit)

/** 32-bit macros. **/
#define SET_BIT32(target, bit)              SET_BIT_RADDR(target, bit)
#define CLEAR_BIT32(target, bit)            CLEAR_BIT_RADDR(target, bit)
#define TOGGLE_BIT32(target, bit)           TOGGLE_BIT_RADDR(target, bit)
#define IS_BIT_SET32(target, bit)           IS_BIT_SET_RADDR(target, bit)
#define IS_BIT_CLEAR32(target, bit)         IS_BIT_CLEAR_RADDR(target, bit)

/** 64-bit macros. **/
#define SET_BIT64(target, bit)              SET_BIT_RADDR(target, bit)
#define CLEAR_BIT64(target, bit)            CLEAR_BIT_RADDR(target, bit)
#define TOGGLE_BIT64(target, bit)           TOGGLE_BIT_RADDR(target, bit)
#define IS_BIT_SET64(target, bit)           IS_BIT_SET_RADDR(target, bit)
#define IS_BIT_CLEAR64(target, bit)         IS_BIT_CLEAR_RADDR(target, bit)

#define SET_BIT64_P(target, bit)              SET_BIT_RADDR_P(target, bit)
#define CLEAR_BIT64_P(target, bit)            CLEAR_BIT_RADDR_P(target, bit)
#define TOGGLE_BIT64_P(target, bit)           TOGGLE_BIT_RADDR_P(target, bit)
#define IS_BIT_SET64_P(target, bit)           IS_BIT_SET_RADDR_P(target, bit)
#define IS_BIT_CLEAR64_P(target, bit)         IS_BIT_CLEAR_RADDR_P(target, bit)

/** 128-bit macros. **/
#define SET_BIT128(target, bit)              SET_BIT_RADDR(target, bit)
#define CLEAR_BIT128(target, bit)            CLEAR_BIT_RADDR(target, bit)
#define TOGGLE_BIT128(target, bit)           TOGGLE_BIT_RADDR(target, bit)
#define IS_BIT_SET128(target, bit)           IS_BIT_SET_RADDR(target, bit)
#define IS_BIT_CLEAR128(target, bit)         IS_BIT_CLEAR_RADDR(target, bit)

#define SET_BIT128_P(target, bit)              SET_BIT_RADDR_P(target, bit)
#define CLEAR_BIT128_P(target, bit)            CLEAR_BIT_RADDR_P(target, bit)
#define TOGGLE_BIT128_P(target, bit)           TOGGLE_BIT_RADDR_P(target, bit)
#define IS_BIT_SET128_P(target, bit)           IS_BIT_SET_RADDR_P(target, bit)
#define IS_BIT_CLEAR128_P(target, bit)         IS_BIT_CLEAR_RADDR_P(target, bit)


uint8_t popcount(const uint8_t bits);
uint8_t popcount16(const uint16_t bits);
uint8_t popcount32(const uint32_t bits);

/** @} */

/**
 * \addtogroup helpers_common_symbols Commonly used symbols and macros
 * @{
 **/

/** Define commonly used C symbols. **/
#ifndef NULL
#define NULL 0
#endif
#define FALSE 0
#define TRUE 1

/** Number of elements in a static array. **/
#define countof(arr) ((sizeof(arr)/sizeof(arr[0])))
  
/** Compile-time assertion in C. **/
#define C_ASSERT(e) extern char __C_ASSERT__[(e)?1:-1]

/** Compress a C structure to byte aligned boundaries. **/
#define PACKED __attribute__ ((packed))

/** @} **/

/**
 * \addtogroup helpers_math Math macros
 * @{
 **/

/** Get the minimum of x and y. **/
#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

/** Get the maximum of x and y. **/
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/** Get the absolute value of x. **/
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#ifndef HOST_MIDIDUINO
//#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#endif
//long map(long x, long in_min, long in_max, long out_min, long out_max);
bool in_area(int x, int y, int x2, int y2, int w, int h);
uint8_t u_limit_value(uint8_t value, int8_t encoder, uint8_t min, uint8_t max);
int limit_value(int value, int encoder, int min, int max);
uint8_t interpolate_8(uint8_t start, uint8_t end, uint8_t amount);


/** @} **/

	/**
	 * \addtogroup helpers_locking "Locking" and AVR-specific functions
	 * @{
	 **/
	
#ifdef AVR
  #include <avr/interrupt.h>

	/**
	 * Declaration of temporary variable to store interrupt status.
	 * Use this before using SET_LOCK() and CLEAR_LOCK().
	 **/
#define USE_LOCK()   uint8_t _irqlock_tmp

	/** Set a "lock" by disabling the interrupts. **/
#define SET_LOCK()   _irqlock_tmp = SREG; cli()
	/** Clear the "lock" by restoring the interrupt status. **/
#define CLEAR_LOCK() SREG = _irqlock_tmp
#else
#endif

#ifdef AVR
#include <avr/pgmspace.h>
#endif
	/** Host-side empty definitions for access to program space. **/

	/** @} **/

	/** \category Delay functions. **/

#ifdef MIDIDUINO
#include <util/delay.h>
	/** Use the arduino convention to delay inside the code. **/
#define delay(ms) _delay_ms(ms)
#define delayMicroseconds(us) _delay_us(us)
#endif

#ifdef HOST_MIDIDUINO
	/** Use empty definitions for now on the host side XXX */
#define delay(ms)
#define delayMicroseconds(us)
#endif

/** \addtogroup helpers_string String manipulation routines
 * @{
 **/
	
void m_toupper(char* str);
void m_trim_space(char* str);

/** @} */

/**
 * \addtogroup helpers_clock Timing functions
 * @{
 **/
extern volatile uint16_t g_clock_fast;
extern volatile uint16_t g_clock_ms;

extern uint16_t read_clock(void);
extern uint16_t read_slowclock(void);
uint16_t clock_diff(uint16_t old_clock, uint16_t new_clock);

/** @} **/

/**
 * \addtogroup helpers_debug Debugging functions
 * @{
 **/
			
#ifdef HOST_MIDIDUINO
	void hexdump(uint8_t *data, uint16_t len);
#endif

/**
 * @}
 */
	
#ifdef __cplusplus
}

#include "Task.h"
#endif

