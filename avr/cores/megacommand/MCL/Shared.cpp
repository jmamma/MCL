#include "Shared.h"

void create_chars_mixer() {
  uint8_t temp_charmap[8] = {0, 0, 0, 0, 0, 0, 0, 31};

  for (uint8_t i = 1; i < 8; i++) {
    for (uint8_t x = 1; x < i; x++) {
      temp_charmap[(8 - x)] = 31;
      LCD.createChar(1 + i, temp_charmap);
    }
  }
}

