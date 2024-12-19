#pragma once
#include "core.h"
#include "string.h"
#include "platform.h"
#include "helpers.h"

#define NUM_DEVS 2

#define RX_BUF_SIZE 0x80UL
#define TX_BUF_SIZE 0x0C00UL
#define TX_SEQBUF_SIZE 0x200UL

#define UART1_RX_BUFFER_LEN RX_BUF_SIZE
#define UART1_TX_BUFFER_LEN TX_BUF_SIZE

#define UART2_RX_BUFFER_LEN RX_BUF_SIZE
#define UART2_TX_BUFFER_LEN TX_BUF_SIZE

#define UARTUSB_RX_BUFFER_LEN RX_BUF_SIZE
#define UARTUSB_TX_BUFFER_LEN TX_BUF_SIZE

#define SYSEX1_DATA_LEN 0x1830UL //6KB
#define SYSEX2_DATA_LEN 0x1830UL //6KB
#define SYSEXUSB_DATA_LEN 0x1830UL //6KB

#define NUM_CLOCK_CALLBACKS 4

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

FORCED_INLINE() extern inline uint8_t get_byte_bank1(volatile uint8_t *dst) {
  uint8_t c = *dst;
  return c;
}

//BANK3
template<typename T>
FORCED_INLINE() extern inline T get_bank3(volatile T *dst) {
  return *dst;
}

FORCED_INLINE() extern inline void get_bank3(volatile void *dst, volatile const void *src, uint16_t len) {
  memcpy((void*)dst, (uint8_t*)src, len);
}

FORCED_INLINE() extern inline void put_bank3(volatile void *dst, volatile const void *src, uint16_t len) {
  memcpy((void*)src, (uint8_t*)dst, len);
}

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

extern volatile uint8_t *rand_ptr;

FORCED_INLINE() extern inline uint8_t get_random_byte() {
    // Explicit cast to satisfy pgm_read_byte's const requirement
    const uint8_t* ptr = const_cast<const uint8_t*>(rand_ptr++);
    return (pgm_read_byte(ptr) ^ get_byte_bank1(rand_ptr) ^ g_clock_ms);
}

extern inline uint8_t get_random(uint8_t range) {
    uint8_t randomValue = get_random_byte();
    return (uint8_t)((uint16_t)randomValue * range / 256);
}


