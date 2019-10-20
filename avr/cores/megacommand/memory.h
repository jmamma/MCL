#ifndef MEMORY_H__
#define MEMORY_H__

#define RX_BUF_SIZE 0x80UL
#define TX_BUF_SIZE 0x0C00UL

#define BANK1_UART1_RX_BUFFER_START 0x2200UL
#define UART1_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART1_TX_BUFFER_START (BANK1_UART1_RX_BUFFER_START + UART1_RX_BUFFER_LEN)
#define UART1_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_UART2_RX_BUFFER_START (BANK1_UART1_TX_BUFFER_START + UART1_TX_BUFFER_LEN)
#define UART2_RX_BUFFER_LEN (RX_BUF_SIZE)

#define BANK1_UART2_TX_BUFFER_START (BANK1_UART2_RX_BUFFER_START + UART2_RX_BUFFER_LEN)
#define UART2_TX_BUFFER_LEN (TX_BUF_SIZE)

#define BANK1_SYSEX1_DATA_START (BANK1_UART2_TX_BUFFER_START + UART2_TX_BUFFER_LEN)
#define SYSEX1_DATA_LEN 0x1830UL //6KB

#define BANK1_SYSEX2_DATA_START (BANK1_SYSEX1_DATA_START + SYSEX1_DATA_LEN)
#define SYSEX2_DATA_LEN 0x4000UL //16KB

/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

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

#ifdef MEGACOMMAND

#define PL6_MASK (1 << PL6)

extern inline uint8_t switch_ram_bank(uint8_t x) {
  uint8_t old_bank = (uint8_t) (PORTL & PL6_MASK);

  if (x != old_bank) {
    PORTL ^= PL6_MASK;
    return old_bank;
  }
  return x;
}

#else

#define PB0_MASK (1 << PB0)

extern inline uint8_t switch_ram_bank(uint8_t x) {
  uint8_t old_bank = (uint8_t) (PORTB & PB0_MASK);

  if (x != old_bank) {
    PORTB ^= PB0_MASK;
    return old_bank;
  }
  return x;
}

#endif


#ifdef __cplusplus

class RamBankSelector {
  private:
  uint8_t m_oldbank;
  public:
  RamBankSelector(uint8_t bank) { m_oldbank = switch_ram_bank(bank); }
  ~RamBankSelector() { switch_ram_bank(m_oldbank); }
};

#define select_bank(x) RamBankSelector __bank_selector(x)

template<typename T>
extern inline T get_bank1(volatile T *dst) {
  select_bank(1);
  T c = *dst;
  return c;
}

template<typename T>
extern inline void put_bank1(volatile T *dst, T data) {
  select_bank(1);
  *dst = data;
}

#endif// __cplusplus

extern inline void memcpy_bank1(volatile void *dst, volatile void *src, uint32_t len) {
  select_bank(1);
  memcpy(dst, src, len);
}

extern inline void put_byte_bank1(volatile uint8_t *dst, uint8_t byte) {
  select_bank(1);
  *dst = byte;
}

extern inline uint8_t get_byte_bank1(volatile uint8_t *dst) {
  select_bank(1);
  uint8_t c = *dst;
  return c;
}

extern volatile uint8_t *rand_ptr;

ALWAYS_INLINE() extern inline uint8_t get_random_byte() {
    return (pgm_read_byte(rand_ptr++) ^ get_byte_bank1(rand_ptr) ^ slowclock) & 0x7F;
}


#endif /* MEMORY_H__ */
