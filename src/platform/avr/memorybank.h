#define BANK0 0

#ifdef MEGACOMMAND
  #define BANK_PORT PORTL
  #define BANK_MASK (1 << PL6)
#else
  #define BANK_PORT PORTB
  #define BANK_MASK (1 << PB0)
#endif

#define BANK1 BANK_MASK

FORCED_INLINE() inline uint8_t switch_ram_bank(uint8_t x) {
  uint8_t old_bank = BANK_PORT & BANK_MASK;
  BANK_PORT = (BANK_PORT & ~BANK_MASK) | x;
  return old_bank;
}

#define switch_ram_bank_noret(x) ((void)switch_ram_bank(x))
