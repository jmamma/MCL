/* Copyright (c) 2009 - http://ruinwesen.com/ */

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Collection of C helper functions and macros.
 **/

#ifndef HELPERS_H__
#define HELPERS_H__

#include <inttypes.h>

#include <stdarg.h>

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

/** 32 bit bit accessing macro. **/
#define _BV32(i) (_bvmasks32[i]) 

/** Bit-level access and test macros. **/
#define SET_BIT(target, bit)                (target |=  _bvmasks[(uint8_t)bit])
#define CLEAR_BIT(target, bit)              (target &= _ibvmasks[(uint8_t)bit])
#define TOGGLE_BIT(target, bit)             (target ^=  _bvmasks[(uint8_t)bit])
#define IS_BIT_SET(target, bit)      ((bool)(target &   _bvmasks[(uint8_t)bit]))
#define IS_BIT_CLEAR(target, bit)   (!(bool)(target &   _bvmasks[(uint8_t)bit]))

/** 8-bit macros. **/
#define SET_BIT8(target, bit) (SET_BIT(target,bit))
#define CLEAR_BIT8(target, bit) (CLEAR_BIT(target,bit))
#define TOGGLE_BIT8(target, bit) (TOGGLE_BIT(taget, bit))
#define IS_BIT_SET8(target, bit) (IS_BIT_SET(target, bit))
#define IS_BIT_CLEAR8(target, bit) (IS_BIT_CLEAR(target, bit))

/** 16-bit macros. **/
#define SET_BIT16(target, bit)              (((uint8_t*)(&target))[(uint8_t)bit / 8U] |=  _bvmasks[(uint8_t)bit & '\07'])
#define CLEAR_BIT16(target, bit)            (((uint8_t*)(&target))[(uint8_t)bit / 8U] &= _ibvmasks[(uint8_t)bit & '\07'])
#define TOGGLE_BIT16(target, bit)           (((uint8_t*)(&target))[(uint8_t)bit / 8U] ^=  _bvmasks[(uint8_t)bit & '\07'])
#define IS_BIT_SET16(target, bit)    ((bool)(((uint8_t*)(&target))[(uint8_t)bit / 8U] &   _bvmasks[(uint8_t)bit & '\07']))
#define IS_BIT_CLEAR16(target, bit) (!(bool)(((uint8_t*)(&target))[(uint8_t)bit / 8U] &   _bvmasks[(uint8_t)bit & '\07']))

/** 32-bit macros. **/
#define SET_BIT32(target, bit)              (((uint8_t*)(&target))[(uint8_t)bit / 8U] |=  _bvmasks[(uint8_t)bit & '\07'])
#define CLEAR_BIT32(target, bit)            (((uint8_t*)(&target))[(uint8_t)bit / 8U] &= _ibvmasks[(uint8_t)bit & '\07'])
#define TOGGLE_BIT32(target, bit)           (((uint8_t*)(&target))[(uint8_t)bit / 8U] ^=  _bvmasks[(uint8_t)bit & '\07'])
#define IS_BIT_SET32(target, bit)    ((bool)(((uint8_t*)(&target))[(uint8_t)bit / 8U] &   _bvmasks[(uint8_t)bit & '\07']))
#define IS_BIT_CLEAR32(target, bit) (!(bool)(((uint8_t*)(&target))[(uint8_t)bit / 8U] &   _bvmasks[(uint8_t)bit & '\07']))

/** 64-bit macros. **/
#define SET_BIT64(target, bit)            do{((uint8_t*)(&target))[(uint8_t)bit / 8U] |=  _bvmasks[(uint8_t)bit & '\07'];}while(false)
#define CLEAR_BIT64(target, bit)          do{((uint8_t*)(&target))[(uint8_t)bit / 8U] &= _ibvmasks[(uint8_t)bit & '\07'];}while(false)
#define TOGGLE_BIT64(target, bit)         do{((uint8_t*)(&target))[(uint8_t)bit / 8U] ^=  _bvmasks[(uint8_t)bit & '\07'];}while(false)
#define IS_BIT_SET64(target, bit)     (bool)(((uint8_t*)(&target))[(uint8_t)bit / 8U] &   _bvmasks[(uint8_t)bit & '\07'])
#define IS_BIT_CLEAR64(target, bit) (!(bool)(((uint8_t*)(&target))[(uint8_t)bit / 8U] &   _bvmasks[(uint8_t)bit & '\07']))

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
#define MIN(x, y) ((x) > (y) ? (y) : (x))
/** Get the maximum of x and y. **/
#define MAX(x, y) ((x) > (y) ? (x) : (y))
/** Get the absolute value of x. **/
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#ifndef HOST_MIDIDUINO
//#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#endif
//long map(long x, long in_min, long in_max, long out_min, long out_max);

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

	/** On the host, use empty routines to store interrupt status. **/
#define USE_LOCK()   
#define SET_LOCK()   
#define CLEAR_LOCK()
	
#endif

#ifdef AVR
#include <avr/pgmspace.h>
#else
	/** Host-side empty definitions for access to program space. **/
#define PGM_P const char *
#define pgm_read_byte(a) (uint8_t)((*a))
#endif

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
	
uint16_t m_snprintf(char *dst, uint16_t len, const char *fmt, ...);
uint16_t m_vsnprintf(char *dst, uint16_t len, const char *fmt, va_list lp);
	
void m_memcpy(void *dst, const void *src, uint16_t cnt);
void m_memset(void *dst, uint16_t cnt, uint8_t elt);
void m_strncpy(void *dst, const char *src, uint16_t cnt);
void m_strncpy_fill(void *dst, const char *src, uint16_t cnt);
void m_memcpy_p(void *dst, PGM_P src, uint16_t cnt);
void m_strncpy_p(void *dst, PGM_P src, uint16_t cnt);
void m_strncpy_p_fill(void *dst, PGM_P src, uint16_t cnt);
void m_memclr(void *dst, uint16_t cnt);
void m_str16cpy_fill(void *dst, const char *src);
void m_str16cpy_p_fill(void *dst, PGM_P src);
void m_str16cpy_p(void *dst, PGM_P src);
void m_strnappend(void *dst, const char *src, int len);
uint16_t m_strlen(const char *str);

/** @} */

/**
 * \addtogroup helpers_clock Timing functions
 * @{
 **/
			
extern uint16_t read_clock(void);
extern uint16_t read_slowclock(void);
uint16_t clock_diff(uint16_t old_clock, uint16_t new_clock);

#ifdef HOST_MIDIDUINO
#else
extern volatile uint16_t clock;
extern volatile uint16_t slowclock;
extern volatile uint16_t clock_minutes;
#endif

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

#include "BitArray.hh"
#include "Task.hh"
#endif

#endif /* HELPERS_H__ */
