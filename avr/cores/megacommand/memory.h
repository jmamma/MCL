#ifndef MEMORY_H__
#define MEMORY_H__

#define NUM_DEVS 2

#define NUM_CLOCK_CALLBACKS 4

#define RX_BUF_SIZE 0x80UL
#define TX_BUF_SIZE 0x0C00UL
#define TX_SEQBUF_SIZE 0x200UL

#define BANK1_UART1_RX_BUFFER_START 0x2200UL
#define UART1_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART1_TX_BUFFER_START (BANK1_UART1_RX_BUFFER_START + UART1_RX_BUFFER_LEN)
#define UART1_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_UART2_RX_BUFFER_START (BANK1_UART1_TX_BUFFER_START + UART1_TX_BUFFER_LEN)
#define UART2_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART2_TX_BUFFER_START (BANK1_UART2_RX_BUFFER_START + UART2_RX_BUFFER_LEN)
#define UART2_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_UARTSEQ_TX1_BUFFER_START (BANK1_UART2_TX_BUFFER_START + UART2_TX_BUFFER_LEN)
#define UART1_UARTSEQ_TX1_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_UARTSEQ_TX2_BUFFER_START (BANK1_UARTSEQ_TX1_BUFFER_START + TX_SEQBUF_SIZE)
#define UART1_UARTSEQ_TX2_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_UARTSEQ_TX3_BUFFER_START (BANK1_UARTSEQ_TX2_BUFFER_START + TX_SEQBUF_SIZE)
#define UART1_UARTSEQ_TX3_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_UARTSEQ_TX4_BUFFER_START (BANK1_UARTSEQ_TX3_BUFFER_START + TX_SEQBUF_SIZE)
#define UART1_UARTSEQ_TX4_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_SYSEX1_DATA_START (BANK1_UARTSEQ_TX4_BUFFER_START + TX_SEQBUF_SIZE)
#define SYSEX1_DATA_LEN 0x1830UL //6KB

#define BANK1_SYSEX2_DATA_START (BANK1_SYSEX1_DATA_START + SYSEX1_DATA_LEN)
#define SYSEX2_DATA_LEN 0x1830UL //6KB

/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

/*
#pragma message (VAR_NAME_VALUE(RX_BUF_SIZE))
#pragma message (VAR_NAME_VALUE(TX_BUF_SIZE))
#pragma message (VAR_NAME_VALUE(BANK1_UART1_RX_BUFFER_START))
#pragma message (VAR_NAME_VALUE(UART1_RX_BUFFER_LEN))
#pragma message (VAR_NAME_VALUE(BANK1_UART1_TX_BUFFER_START))
#pragma message (VAR_NAME_VALUE(UART1_TX_BUFFER_LEN))
#pragma message (VAR_NAME_VALUE(BANK1_UART2_RX_BUFFER_START))
#pragma message (VAR_NAME_VALUE(UART2_RX_BUFFER_LEN))
#pragma message (VAR_NAME_VALUE(BANK1_UART2_TX_BUFFER_START))
#pragma message (VAR_NAME_VALUE(UART2_TX_BUFFER_LEN))
#pragma message (VAR_NAME_VALUE(BANK1_SYSEX1_DATA_START))
#pragma message (VAR_NAME_VALUE(SYSEX1_DATA_LEN))
#pragma message (VAR_NAME_VALUE(BANK1_SYSEX2_DATA_START))
#pragma message (VAR_NAME_VALUE(SYSEX2_DATA_LEN))
*/

#include "memorybank.h"

#ifdef __cplusplus

class RamBankSelector {
  private:
  uint8_t m_oldbank;
  public:
  FORCED_INLINE() RamBankSelector(uint8_t bank) { m_oldbank = switch_ram_bank(bank); }
  FORCED_INLINE() ~RamBankSelector() { switch_ram_bank(m_oldbank); }
};

#define select_bank(x) RamBankSelector __bank_selector(x)

template<typename T>
FORCED_INLINE() extern inline T get_bank1(volatile T *dst) {
  select_bank(1);
  T c = *dst;
  return c;
}

template<typename T>
FORCED_INLINE() extern inline void put_bank1(volatile T *dst, T data) {
  select_bank(1);
  *dst = data;
}

FORCED_INLINE() extern inline void memcpy_bank1(volatile void *dst, volatile const void *src, uint16_t len) {
  select_bank(1);
  memcpy((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void put_byte_bank1(volatile uint8_t *dst, uint8_t byte) {
  select_bank(1);
  *dst = byte;
}

FORCED_INLINE() extern inline uint8_t get_byte_bank1(volatile uint8_t *dst) {
  select_bank(1);
  uint8_t c = *dst;
  return c;
}

extern volatile uint8_t *rand_ptr;

FORCED_INLINE() extern inline uint8_t get_random_byte() {
    return (pgm_read_byte(rand_ptr++) ^ get_byte_bank1(rand_ptr) ^ slowclock) & 0x7F;
}

#endif// __cplusplus

#endif /* MEMORY_H__ */
