#pragma once
#ifdef MEGACOMMAND

#define PL6_MASK (1 << PL6)

ALWAYS_INLINE() inline uint8_t switch_ram_bank(uint8_t x) {
  uint8_t old_bank = (uint8_t) (PORTL & PL6_MASK);

  if (x != old_bank) {
    PORTL ^= PL6_MASK;
    return old_bank;
  }
  return x;
}

#else

#define PB0_MASK (1 << PB0)

ALWAYS_INLINE() inline uint8_t switch_ram_bank(uint8_t x) {
  uint8_t old_bank = (uint8_t) (PORTB & PB0_MASK);

  if (x != old_bank) {
    PORTB ^= PB0_MASK;
    return old_bank;
  }
  return x;
}

#endif

