/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include <stdarg.h>

#ifdef AVR
#include "WProgram.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#endif

#include "helpers.h"

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

/** Return the length of a string. **/
uint16_t m_strlen(const char *src) {
  uint16_t result = 0;
  while (src[result++] != '\0')
    ;
  return result;
}

/** Copy cnt bytes from src to dst. **/
void m_memcpy(void *dst, const void *src, uint16_t cnt) {
  while (cnt) {
    *((uint8_t *)dst++) = *((uint8_t *)src++);
    cnt--;
  }
}

/** Copy cnt bytes from the program space src to dst. **/
void m_memcpy_p(void *dst, PGM_P src, uint16_t cnt) {
  while (cnt) {
    *((uint8_t *)dst++) = pgm_read_byte(src);
    src++;
    cnt--;
  }
}

static char tohex(uint8_t i) {
  if (i < 10) {
    return i + '0';
  } else {
    return i - 10 + 'a';
  }
}

/**
 * va_string version of printf.
 *
 * Format arguments are:
 * - %b (byte value)
 * - %B (short value)
 * - %x (hex byte value)
 * - %X (hex short value)
 * - %s (string)
 **/
uint16_t m_vsnprintf(char *dst, uint16_t len, const char *fmt, va_list lp) {

  char *ptr = dst;
  char *end = ptr + len - 1;
  while ((*fmt != 0) && (ptr < end)) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case '\0':
        goto end;
        break;

      case 'b': // byte
      {
        uint8_t i = va_arg(lp, int);
        if ((ptr + 3) < end) {
          *(ptr++) = i / 100 + '0';
          *(ptr++) = (i % 100) / 10 + '0';
          *(ptr++) = (i % 10) + '0';
        } else {
          goto end;
        }
      } break;

      case 'B': // short
      {
        uint16_t i = va_arg(lp, int);
        if ((ptr + 5) < end) {
          *(ptr++) = i / 10000 + '0';
          *(ptr++) = (i % 10000) / 1000 + '0';
          *(ptr++) = (i % 1000) / 100 + '0';
          *(ptr++) = (i % 100) / 10 + '0';
          *(ptr++) = (i % 10) + '0';
        } else {
          goto end;
        }
      } break;

      case 'x': // hex 8
      {
        uint8_t i = va_arg(lp, int);
        if ((ptr + 2) < end) {
          *(ptr++) = tohex(i / 16);
          *(ptr++) = tohex(i % 16);
        } else {
          goto end;
        }
      } break;

      case 'X': // hex 16
      {
        uint16_t i = va_arg(lp, int);
        if ((ptr + 4) < end) {
          *(ptr++) = tohex((i >> 12) & 0xF);
          *(ptr++) = tohex((i >> 8) & 0xF);
          *(ptr++) = tohex((i >> 4) & 0xF);
          *(ptr++) = tohex(i & 0xF);
        } else {
          goto end;
        }
      } break;

      case 's': // string
      {
        const char *ptr2 = va_arg(lp, char *);
        while ((ptr < end) && *ptr2) {
          *ptr++ = *ptr2++;
        }
      } break;
      }
    } else {
      *ptr++ = *fmt;
    }
    fmt++;
  }

  *ptr = '\0';
  ptr++;
end:
  return ptr - dst;
}

/**
 * embedded printf.
 *
 * Format arguments are:
 * - %b (byte value)
 * - %B (short value)
 * - %x (hex byte value)
 * - %X (hex short value)
 * - %s (string)
 **/
uint16_t m_snprintf(char *dst, uint16_t len, const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);
  uint16_t ret = m_vsnprintf(dst, len, fmt, lp);
  va_end(lp);
  return ret;
}

/** Copy cnt bytes from src to dst. **/
void m_strncpy(void *dst, const char *src, uint16_t cnt) {
  while (cnt && *src) {
    *((uint8_t *)dst++) = *((uint8_t *)src++);
    cnt--;
  }
  if (cnt > 0) {
    *((uint8_t *)dst++) = 0;
  }
}

/** Copy cnt bytes from src to dst, and fill up with spaces. **/
void m_strncpy_fill(void *dst, const char *src, uint16_t cnt) {
  while (cnt && *src) {
    *((uint8_t *)dst++) = *((uint8_t *)src++);
    cnt--;
  }
  while (cnt > 1) {
    cnt--;
    *((uint8_t *)dst++) = ' ';
  }
  if (cnt > 0)
    *((uint8_t *)dst++) = 0;
}

/** Copy cnt bytes from program space src to dst. **/
void m_strncpy_p(void *dst, PGM_P src, uint16_t cnt) {
  while (cnt) {
    char byte = pgm_read_byte(src);
    if (byte == 0)
      break;
    *((uint8_t *)dst++) = byte;
    src++;
    cnt--;
  }
  if (cnt > 0) {
    *((uint8_t *)dst++) = 0;
  }
}

/** Copy cnt bytes from program space src to dst, and fill up with spaces. **/
void m_strncpy_p_fill(void *dst, PGM_P src, uint16_t cnt) {
  while (cnt) {
    char byte = pgm_read_byte(src);
    if (byte == 0)
      break;
    *((uint8_t *)dst++) = byte;
    src++;
    cnt--;
  }
  while (cnt > 1) {
    *((uint8_t *)dst++) = ' ';
    cnt--;
  }
  if (cnt > 0)
    *((uint8_t *)dst++) = 0;
}

/** Clear cnt bytes of dst and set them to 0. **/
void m_memclr(void *dst, uint16_t cnt) {
  while (cnt) {
    *((uint8_t *)dst++) = 0;
    cnt--;
  }
}

/** Set cnt bytes at dst with the value elt. **/
void m_memset(void *dst, uint16_t cnt, uint8_t elt) {
  while (cnt) {
    *((uint8_t *)dst++) = elt;
    cnt--;
  }
}

/** Copy string (max 16 characters) from src to dst and fill up with whitespace.
 * **/
void m_str16cpy_fill(void *dst, const char *src) {
  m_strncpy_fill(dst, src, 16);
}

/** Copy string (max 16 characters) from program space src to dst and fill up
 * with whitespace. **/
void m_str16cpy_p_fill(void *dst, PGM_P src) { m_strncpy_p_fill(dst, src, 16); }

/** Copy string (max 16 characters) from program space src to dst. **/
void m_str16cpy_p(void *dst, PGM_P src) { m_strncpy_p(dst, src, 16); }

/** Append the string at src to the string at dst, not exceeding len characters.
 * **/
void m_strnappend(void *dst, const char *src, int len) {
  char *ptr = dst;
  int i;
  for (i = 0; i < len; i++) {
    if (ptr[0] == '\0')
      break;
    ptr++;
  }
  m_strncpy(ptr, src, len - i);
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
  for (int i = m_strlen(str) - 1; i >= 0; --i) {
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
volatile uint16_t slowclock = 0;
volatile uint16_t clock = 0;
volatile uint16_t clock_minutes = 0;
/** Embedded version of read_clock, return the fast clock counter. **/
uint16_t read_clock(void) {
  USE_LOCK();
  SET_LOCK();
  uint16_t ret = clock;
  CLEAR_LOCK();
  return ret;
}

/** Embedded version of read_slowclock, return the slow clock counter. **/
uint16_t read_slowclock(void) {
  USE_LOCK();
  SET_LOCK();
  uint16_t ret = slowclock;
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

/** @} **/

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
