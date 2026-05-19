// memory.h — desktop platform memory map.
//
// MCL has two memory schemes: AVR uses fixed addresses ("banks") and pulls
// in src/platform/avr/memory.h with BANK1_*/BANK3_START macros; RP2040 uses
// statically-allocated arrays via MCL_MEMORY_USE_ARRAYS. Desktop follows the
// RP2040 scheme — flat heap allocations, link-time addresses for the cache
// arrays, and inline bank helpers that just `memcpy`. Borrowed verbatim from
// src/platform/rp2040/memory.h (minus the rp2040-specific #ifdef PLATFORM_TBD
// branches).
#pragma once

#include "platform.h"
#include "helpers.h"

#include <string.h>

#define NUM_DEVS 2

#define RX_BUF_SIZE 0x80UL
#define TX_BUF_SIZE 0x0C00UL
#define TX_SEQBUF_SIZE 0x200UL
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

#define SYSEX1_DATA_LEN     0x1800UL
#define SYSEX2_DATA_LEN     0x1800UL
#define SYSEXUSB_DATA_LEN   0x1800UL

#define MCL_MEMORY_USE_ARRAYS 1

#define NUM_CLOCK_CALLBACKS 4
#define NUM_MIDI_CALLBACKS  16
#define NUM_SYSEX_SLAVES    16
#define NUM_SYSEX_CALLBACKS 4

#define MEMORY_ALIGN(size) (((size) + 3) & ~3)

// Flat-memory bank helpers. RP2040 had no banking either; copy-paste the same
// trivial inlines so MCL's `get_bank1(...)` etc. resolve.
template <typename T>
FORCED_INLINE() inline T get_bank1(volatile T* dst) { return *dst; }

template <typename T>
FORCED_INLINE() inline void put_bank1(volatile T* dst, T data) { *dst = data; }

FORCED_INLINE() inline int memcmp_bank1(volatile void* dst, volatile const void* src, uint16_t len) {
    return memcmp((void*)dst, (void*)src, len);
}
FORCED_INLINE() inline void memcpy_bank1(volatile void* dst, volatile const void* src, uint16_t len) {
    memcpy((void*)dst, (void*)src, len);
}
FORCED_INLINE() inline void put_byte_bank1(volatile uint8_t* dst, uint8_t b)      { *dst = b; }
FORCED_INLINE() inline void put_byte_bank1_isr(volatile uint8_t* dst, uint8_t b)  { *dst = b; }
FORCED_INLINE() inline uint8_t get_byte_bank1_isr(volatile uint8_t* src)          { return *src; }
FORCED_INLINE() inline uint8_t get_byte_bank1(volatile uint8_t* dst)              { return *dst; }

template <typename T>
FORCED_INLINE() inline T get_bank3(volatile T* src) { return *src; }
FORCED_INLINE() inline void get_bank3(void* dst, volatile const void* src, uint16_t len) {
    memcpy(dst, (const void*)src, len);
}
FORCED_INLINE() inline void put_bank3(volatile void* dst, const void* src, uint16_t len) {
    memcpy((void*)dst, src, len);
}
template <typename T>
FORCED_INLINE() inline void put_bank3(volatile T* dst, T data) { *dst = data; }
FORCED_INLINE() inline int memcmp_bank3(volatile void* dst, volatile const void* src, uint16_t len) {
    return memcmp((void*)dst, (void*)src, len);
}
FORCED_INLINE() inline void memcpy_bank3(volatile void* dst, volatile const void* src, uint16_t len) {
    memcpy((void*)dst, (void*)src, len);
}
FORCED_INLINE() inline void put_byte_bank3(volatile uint8_t* dst, uint8_t b)  { *dst = b; }
FORCED_INLINE() inline uint8_t get_byte_bank3(volatile uint8_t* dst)          { return *dst; }

extern volatile uint16_t g_random_state;

FORCED_INLINE() inline uint8_t get_random_byte() {
    uint16_t state = g_random_state;
    if (state == 0) {
        state = read_clock_ms() | 1;
    }
    state = (state >> 1) ^ ((state & 1) ? 0xB400U : 0);
    g_random_state = state;
    return (uint8_t)(state >> 8);
}

inline uint8_t get_random(uint8_t range) {
    uint8_t randomValue = get_random_byte();
    return (uint8_t)((uint16_t)randomValue * range / 256);
}
