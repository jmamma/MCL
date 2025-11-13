#define BANK0 0

#include "platform.h"

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

// Fast ISR-specific bank switch: always switches to BANK0 if not already there
FORCED_INLINE() inline uint8_t switch_ram_bank_fast_isr() {
  uint8_t old_bank = BANK_PORT & BANK_MASK;
  if (old_bank) {  // If not zero (i.e., if BANK1), switch to BANK0
    BANK_PORT &= ~BANK_MASK;
  }
  return old_bank;
}

// Fast ISR-specific bank restore: only writes if bank needs to change
FORCED_INLINE() inline void switch_ram_bank_noret_fast(uint8_t x) {
  if (x) {  // If restoring to BANK1
    BANK_PORT |= BANK_MASK;
  }
  // If x is 0 (BANK0), we're already there, do nothing
}

#define switch_ram_bank_noret(x) ((void)switch_ram_bank(x))
