/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include <stdarg.h>

#ifdef AVR
#include "WProgram.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#endif

#include "hardware/irq.h"
#include "platform.h"
#include "helpers.h"
#include <string.h>
/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Collection of C helper functions
 **/

/**
 * \addtogroup helpers_string Various string functions
 *
 * @{
 **/

const uint8_t _bvmasks[] = {0b00000001, 0b00000010, 0b00000100, 0b00001000,
                            0b00010000, 0b00100000, 0b01000000, 0b10000000};
const uint8_t _ibvmasks[] = {0b11111110, 0b11111101, 0b11111011, 0b11110111,
                             0b11101111, 0b11011111, 0b10111111, 0b01111111};
const uint32_t _bvmasks32[] = {
    0x0001,     0x0002,     0x0004,     0x0008,     0x0010,     0x0020,
    0x0040,     0x0080,     0x0100,     0x0200,     0x0400,     0x0800,
    0x1000,     0x2000,     0x4000,     0x8000,     0x00010000, 0x00020000,
    0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000,
    0x40000000, 0x80000000,
};

const uint8_t _popcount_lut[] PROGMEM = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

uint8_t popcount(const uint8_t bits) { return pgm_read_byte_near(&_popcount_lut[bits]); }

uint8_t popcount16(const uint16_t bits) {
    return popcount(((uint8_t*)&(bits))[0]) + popcount(((uint8_t*)&(bits))[1]);
}

uint8_t popcount32(const uint32_t bits) {
    return popcount(((uint8_t*)&(bits))[0]) + popcount(((uint8_t*)&(bits))[1]) + popcount(((uint8_t*)&(bits))[2]) + popcount(((uint8_t*)&(bits))[3]);
}

static char tohex(uint8_t i) {
  if (i < 10) {
    return i + '0';
  } else {
    return i - 10 + 'a';
  }
}

/** Convert the string to UPPERCASE. **/
void m_toupper(char *str) {
  char chr;
  while ((chr = *str)) {
    if (chr >= 'a' && chr <= 'z') {
      *str = chr - 'a' + 'A';
    }
    ++str;
  }
}

/** Trim ending spaces **/
void m_trim_space(char *str) {
  for (int i = strlen(str) - 1; i >= 0; --i) {
    if (str[i] == ' ') {
      str[i] = '\0';
    }
    // break on first visible character
    if (str[i] != '\0') {
      break;
    }
  }
}
/** @} **/

/**
 * \addtogroup helpers_clock
 * Timing functions
 *
 * @{
 **/

#ifdef HOST_MIDIDUINO
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

static double startClock;
static uint8_t clockStarted = 0;

/** Return the current clock counter value, using the POSIX gettimeofday
 * function. **/
uint16_t read_clock(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double clock = (double)(tv.tv_sec + (double)tv.tv_usec / 1000000.0);
  if (!clockStarted) {
    startClock = clock;
    clockStarted = 1;
  }
  clock -= startClock;
  clock *= 61250;
  clock = fmod(clock, 65536);

  return clock;
}

/** Return the current slow clock counter value, using the POSIX gettimeofday
 * function. **/
uint16_t read_slowclock(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double clock = (double)(tv.tv_sec + (double)tv.tv_usec / 1000000.0);
  if (!clockStarted) {
    startClock = clock;
    clockStarted = 1;
  }
  clock -= startClock;
  clock *= 976;
  clock = fmod(clock, 65536);
  return clock;
}

#else

/** Embedded version of read_clock, return the fast clock counter. **/
uint16_t read_clock(void) {
  USE_LOCK();
  SET_LOCK();
  uint16_t ret = g_fast_ticks;
  CLEAR_LOCK();
  return ret;
}

/** Embedded version of read_slowclock, return the slow clock counter. **/
uint16_t read_slowclock(void) {
  USE_LOCK();
  SET_LOCK();
  uint16_t ret = g_ms_ticks;
  CLEAR_LOCK();
  return ret;
}
#endif

/**
 * Return the difference between old_clock and new_clock, taking into
 * account overflow of the clock counter.
 **/
uint16_t clock_diff(uint16_t old_clock, uint16_t new_clock) {
  if (new_clock >= old_clock)
    return new_clock - old_clock;
  else
    return new_clock + (65536 - old_clock);
}

/**
 * \addtogroup helpers_math Math functions
 *
 * @{
 **/

// Determine if co-ordinate x,y is within rectangular area
bool in_area(int x, int y, int x2, int y2, int w, int h) {
  return (x >= x2) && (x <= x2 + w) && (y >= y2) && (y <= y2 + h);
}

/**
 * Map x from the range in_min - in_max to the range out_min - out_max.
 *
 * x doesn't have to be inside in_min - in_max, and will lead to a
 * similar overflow in the destination range.
 **/
#ifdef MIDIDUINO
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#endif

/**
 * Limit unsigned value with the encoder move in encoder inside min and max.
 **/
uint8_t u_limit_value(uint8_t value, int8_t encoder, uint8_t min, uint8_t max) {
  int16_t result = (int16_t)value + encoder;
  if (result < ((int16_t)min)) {
    return min;
  } else if (result > ((int16_t)max)) {
    return max;
  } else {
    return result;
  }
}

/**
 * Limit signed value with the encoder move in encoder inside min and max.
 **/
int limit_value(int value, int encoder, int min, int max) {
  int result = value + encoder;
  if (result < min) {
    return min;
  } else if (result > max) {
    return max;
  } else {
    return result;
  }
}

uint8_t interpolate_8(uint8_t start, uint8_t end, uint8_t amount) {
  int diff = (end - start);
  return start + ((diff * amount) >> 7);
}

/** @} **/

/**
 * \addtogroup helpers_debug Debugging functions
 * @{
 **/

/** Print out a hexdump of len bytes of data. **/
#ifdef HOST_MIDIDUINO
void hexdump(uint8_t *data, uint16_t len) {
  uint8_t cnt = 0;
  uint16_t i;
  for (i = 0; i < len; i++) {
    if (cnt == 0) {
      printf("%.4x: ", i);
    }
    printf("%.2x ", data[i]);
    cnt++;
    if (cnt == 8) {
      printf(" ");
    }
    if (cnt == 16) {
      printf("\n");
      cnt = 0;
    }
  }
  if (cnt != 0) {
    printf("\n");
  }
}
#endif

/** @} **/

/** @} **/
