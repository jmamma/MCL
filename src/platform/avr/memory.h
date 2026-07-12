#ifndef MEMORY_H__
#define MEMORY_H__

#include "platform.h"

#define NUM_DEVS 2

#define NUM_CLOCK_CALLBACKS 4
#define NUM_MIDI_CALLBACKS 8
#define NUM_SYSEX_SLAVES 5
#define NUM_SYSEX_CALLBACKS 1

#define RX_BUF_SIZE 0x80UL
#define TX_BUF_SIZE 0x0C00UL
#define TX_SEQBUF_SIZE 0x200UL
#define RT_BUF_SIZE 0x08UL

// UART0 (USB) buffers
#define BANK1_UART0_RX_BUFFER_START 0x2200UL
#define UART0_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART0_TX_BUFFER_START (BANK1_UART0_RX_BUFFER_START + UART0_RX_BUFFER_LEN)
#define UART0_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_UART0_RT_BUFFER_START (BANK1_UART0_TX_BUFFER_START + UART0_TX_BUFFER_LEN)
#define UART0_RT_BUFFER_LEN (RT_BUF_SIZE)

// UART1 buffers
#define BANK1_UART1_RX_BUFFER_START (BANK1_UART0_RT_BUFFER_START + UART0_RT_BUFFER_LEN)
#define UART1_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART1_TX_BUFFER_START (BANK1_UART1_RX_BUFFER_START + UART1_RX_BUFFER_LEN)
#define UART1_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_UART1_RT_BUFFER_START (BANK1_UART1_TX_BUFFER_START + UART1_TX_BUFFER_LEN)
#define UART1_RT_BUFFER_LEN (RT_BUF_SIZE)

// UART2 buffers
#define BANK1_UART2_RX_BUFFER_START (BANK1_UART1_RT_BUFFER_START + UART1_RT_BUFFER_LEN)
#define UART2_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART2_TX_BUFFER_START (BANK1_UART2_RX_BUFFER_START + UART2_RX_BUFFER_LEN)
#define UART2_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_UART2_RT_BUFFER_START (BANK1_UART2_TX_BUFFER_START + UART2_TX_BUFFER_LEN)
#define UART2_RT_BUFFER_LEN (RT_BUF_SIZE)

// UART sequencer buffers
#define BANK1_UARTSEQ_TX1_BUFFER_START (BANK1_UART2_RT_BUFFER_START + UART2_RT_BUFFER_LEN)
#define UART1_UARTSEQ_TX1_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_UARTSEQ_TX2_BUFFER_START (BANK1_UARTSEQ_TX1_BUFFER_START + TX_SEQBUF_SIZE)
#define UART1_UARTSEQ_TX2_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_UARTSEQ_TX3_BUFFER_START (BANK1_UARTSEQ_TX2_BUFFER_START + TX_SEQBUF_SIZE)
#define UART1_UARTSEQ_TX3_BUFFER_LEN (TX_SEQBUF_SIZE)

#define BANK1_UARTSEQ_TX4_BUFFER_START (BANK1_UARTSEQ_TX3_BUFFER_START + TX_SEQBUF_SIZE)
#define UART1_UARTSEQ_TX4_BUFFER_LEN (TX_SEQBUF_SIZE)

// Sysex data buffers
#define BANK1_SYSEX1_DATA_START (BANK1_UARTSEQ_TX4_BUFFER_START + TX_SEQBUF_SIZE)
#define SYSEX1_DATA_LEN 0x1800UL //6KB

#define BANK1_SYSEX2_DATA_START (BANK1_SYSEX1_DATA_START + SYSEX1_DATA_LEN)
#define SYSEX2_DATA_LEN 0x1800UL //6KB

#define BANK1_SYSEX3_DATA_START (BANK1_SYSEX2_DATA_START + SYSEX2_DATA_LEN)
#define SYSEX3_DATA_LEN 0x1800UL //6KB

#define BANK3_START 0x0000
#define BANK3_END 0x2000
#include "memorybank.h"

#ifdef __cplusplus

/* MegaCommand hardware has 8KB of stack memory and access to 128KB of external SRAM, split across 4 banks.
   Because stack memory and external memory share the same address range, we lose direct access to the other banks and have to access
   them via special modes */

/*
  BANK 0: 0x2000 -> 0xFFFF accessed natively via .data / global data structures.
  BANK 1: 0x2000 -> 0xFFFF is accessed by switching RAM bank and syphoning RAM between stack memory.
  BANK 2: 0x4000 -> 0x6000 is accessed by enabling fringe RAM access, and syphoning RAM between stack memory.
  BANK 3: 0x4000 -> 0x6000 is accessed by switching RAM bank, enabling fringe RAM access, and syphoning RAM between stack memory.
*/

class RamBankSelector {
  private:
  uint8_t m_oldbank;
  public:
  FORCED_INLINE() RamBankSelector(uint8_t bank) { m_oldbank = switch_ram_bank(bank); }
  FORCED_INLINE() ~RamBankSelector() { switch_ram_bank_noret(m_oldbank); }
};

class RamBankSelectorFast {
  private:
  uint8_t m_oldbank;
  public:
  FORCED_INLINE() RamBankSelectorFast() { m_oldbank = switch_ram_bank_fast(); }
  FORCED_INLINE() ~RamBankSelectorFast() { switch_ram_bank_noret_fast(m_oldbank); }
};

class RamAccessFringe {
  uint8_t irqlock_tmp;
  public:
  FORCED_INLINE() RamAccessFringe() {
    irqlock_tmp = SREG;
    cli();
    DDRC = 0xFF;
    PORTC = 0x00;
    XMCRB = (1<<XMM1);
  }
  FORCED_INLINE() ~RamAccessFringe() {
    XMCRB = 0;
    SREG = irqlock_tmp;
  }
};


#define ram_access_fringe() RamAccessFringe __ram_access_fringe
#define select_bank(x) RamBankSelector __bank_selector(x)
#define select_bank_fast() RamBankSelectorFast __bank_selector_fast

template<typename T>
FORCED_INLINE() extern inline T get_bank1(volatile T *dst) {
  select_bank(BANK1);
  return *dst;
}

template<typename T>
FORCED_INLINE() extern inline void put_bank1(volatile T *dst, T data) {
  select_bank(BANK1);
  *dst = data;
}

FORCED_INLINE() extern inline int memcmp_bank1(volatile void *dst, volatile const void *src, uint16_t len) {
  select_bank(BANK1);
  return memcmp((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void memcpy_bank1(volatile void *dst, volatile const void *src, uint16_t len) {
  select_bank(BANK1);
  memcpy((void*)dst, (void*)src, len);
}

FORCED_INLINE() extern inline void put_byte_bank1(volatile uint8_t *dst, uint8_t byte) {
  select_bank(BANK1);
  *dst = byte;
}

FORCED_INLINE() extern inline uint8_t get_byte_bank1(volatile uint8_t *dst) {
  select_bank(BANK1);
  uint8_t c = *dst;
  return c;
}

// Fast ISR-optimized bank1 byte write (assumes currently in BANK0)
FORCED_INLINE() extern inline void put_byte_bank1_isr(volatile uint8_t *dst, uint8_t byte) {
  BANK_PORT |= BANK_MASK;  // Switch to BANK1
  *dst = byte;
  BANK_PORT &= ~BANK_MASK; // Switch back to BANK0
}

// Fast ISR-optimized bank1 byte read (assumes currently in BANK0)
FORCED_INLINE() extern inline uint8_t get_byte_bank1_isr(volatile uint8_t *src) {
  BANK_PORT |= BANK_MASK;  // Switch to BANK1
  uint8_t c = *src;
  BANK_PORT &= ~BANK_MASK; // Switch back to BANK0
  return c;
}


FORCED_INLINE() extern inline void get_bank2(volatile void *dst, volatile const void *src, uint16_t len) {
  ram_access_fringe();
  memcpy((void*)dst, (uint8_t*)src + 0x4000, len);
}

FORCED_INLINE() extern inline void get_bank3(volatile void *dst, volatile const void *src, uint16_t len) {
  ram_access_fringe();
  select_bank(BANK1);
  memcpy((void*)dst, (uint8_t*)src + 0x4000, len);
}


FORCED_INLINE() extern inline void put_bank2(volatile void *dst, volatile const void *src, uint16_t len) {
  ram_access_fringe();
  memcpy((uint8_t*)dst + 0x4000, (uint8_t*)src, len);
}

FORCED_INLINE() extern inline void put_bank3(volatile void *dst, volatile const void *src, uint16_t len) {
  ram_access_fringe();
  select_bank(BANK1);
  memcpy((uint8_t*)dst + 0x4000, (uint8_t*)src, len);
}



extern volatile uint16_t g_random_state;

FORCED_INLINE() extern inline uint8_t get_random_byte() {
    uint16_t state = g_random_state;
    if (state == 0) {
        state = read_clock_ms() | 1;
    }
    state = (state >> 1) ^ ((state & 1) ? 0xB400U : 0);
    g_random_state = state;
    return (uint8_t)(state >> 8);
}

extern inline uint8_t get_random(uint8_t range) {
    uint8_t randomValue = get_random_byte();
    return (uint8_t)((uint16_t)randomValue * range / 256);
}

#endif// __cplusplus

#endif /* MEMORY_H__ */
