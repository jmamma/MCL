#pragma once
#include "string.h"
#include "platform.h"
#include "helpers.h"

#define NUM_DEVS 2

#define RX_BUF_SIZE 0x80UL
#define TX_BUF_SIZE 0x0C00UL
#ifdef PLATFORM_TBD
#define TX_SEQBUF_SIZE 0x400UL
#else
#define TX_SEQBUF_SIZE 0x200UL
#endif
#define RT_BUF_SIZE 0x08UL

#define UART1_RX_BUFFER_LEN RX_BUF_SIZE
#define UART1_TX_BUFFER_LEN TX_BUF_SIZE
#define UART1_RT_BUFFER_LEN RT_BUF_SIZE

#define UART2_RX_BUFFER_LEN RX_BUF_SIZE
#define UART2_TX_BUFFER_LEN TX_BUF_SIZE
#define UART2_RT_BUFFER_LEN RT_BUF_SIZE

#define UARTUSB_RX_BUFFER_LEN RX_BUF_SIZE
#define UARTUSB_TX_BUFFER_LEN TX_BUF_SIZE
#define UARTUSB_RT_BUFFER_LEN RT_BUF_SIZE

#define SYSEX1_DATA_LEN 0x1800UL //6KB
#define SYSEX2_DATA_LEN 0x1800UL //6KB
#define SYSEXUSB_DATA_LEN 0x1800UL //6KB

#define MCL_MEMORY_USE_ARRAYS 1

#define NUM_CLOCK_CALLBACKS 4
#define NUM_MIDI_CALLBACKS 16
#define NUM_SYSEX_SLAVES 16
#define NUM_SYSEX_CALLBACKS 4

#define MEMORY_ALIGN(size) (((size) + 3) & ~3)

//BANK1
template<typename T>
FORCED_INLINE() extern inline T get_bank1(volatile T *dst) {
  return *dst;
}

template<typename T>
FORCED_INLINE() extern inline void put_bank1(volatile T *dst, T data) {
  *dst = data;
}

FORCED_INLINE() extern inline int memcmp_bank1(volatile void *dst, volatile const void *src, uint16_t len) {
  return memcmp((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void memcpy_bank1(volatile void *dst, volatile const void *src, uint16_t len) {
  memcpy((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void put_byte_bank1(volatile uint8_t *dst, uint8_t byte) {
  *dst = byte;
}

// ISR-optimized version (on RP2040, same as regular put_byte_bank1 since no memory banks)
FORCED_INLINE() extern inline void put_byte_bank1_isr(volatile uint8_t *dst, uint8_t byte) {
  *dst = byte;
}

FORCED_INLINE() extern inline uint8_t get_byte_bank1_isr(volatile uint8_t *src) {
  return *src;
}

FORCED_INLINE() extern inline uint8_t get_byte_bank1(volatile uint8_t *dst) {
  uint8_t c = *dst;
  return c;
}

//BANK3
// Get single value
template<typename T>
FORCED_INLINE() extern inline T get_bank3(volatile T *src) {
    return *src;
}

// Get memory block
FORCED_INLINE() extern inline void get_bank3(void *dst, volatile const void *src, uint16_t len) {
    memcpy(dst, (const void*)src, len);
}

// Put memory block
FORCED_INLINE() extern inline void put_bank3(volatile void *dst, const void *src, uint16_t len) {
    memcpy((void*)dst, src, len);
}

// Put single value
template<typename T>
FORCED_INLINE() extern inline void put_bank3(volatile T *dst, T data) {
    *dst = data;
}

FORCED_INLINE() extern inline int memcmp_bank3(volatile void *dst, volatile const void *src, uint16_t len) {
  return memcmp((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void memcpy_bank3(volatile void *dst, volatile const void *src, uint16_t len) {
  memcpy((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void put_byte_bank3(volatile uint8_t *dst, uint8_t byte) {
  *dst = byte;
}

FORCED_INLINE() extern inline uint8_t get_byte_bank3(volatile uint8_t *dst) {
  uint8_t c = *dst;
  return c;
}

extern volatile uint16_t g_random_state_a;
extern volatile uint16_t g_random_state_b;

FORCED_INLINE() extern inline uint8_t get_random_byte() {
    uint16_t a = g_random_state_a;
    uint16_t b = g_random_state_b;
    if ((a | b) == 0) {
        a = read_clock_ms() ^ 0xA5A5U;
        b = a ^ 0x5A5AU;
    }
    uint16_t next = a + b;
    g_random_state_a = b;
    g_random_state_b = next;
    return (uint8_t)(next >> 8);
}

extern inline uint8_t get_random(uint8_t range) {
    uint8_t randomValue = get_random_byte();
    return (uint8_t)((uint16_t)randomValue * range / 256);
}
