#pragma once

#include "platform.h"

enum TrigLEDMode {
  TRIGLED_OVERLAY = 0,
  TRIGLED_STEPEDIT = 1,
  TRIGLED_EXCLUSIVE = 2,
  TRIGLED_EXCLUSIVENDYNAMIC = 3,
  TRIGLED_MUTE = 4
};

class LED {
private:

public:
  LED() {}

  void set_trigleds(uint16_t bitmask, TrigLEDMode mode, bool blink = false, bool update = true) {}
  // Per-trig RGB override. Bits set in bitmask take colour `rgb`; bits not set
  // are unchanged. The first call enables override mode; the next plain
  // set_trigleds() / reset_trigleds() returns to monochrome rendering.
  // No-op on platforms without addressable LEDs.
  void set_trigleds_color(uint16_t bitmask, uint32_t rgb) {}
  void clear_trigleds() {}
  void reset_trigleds() {}
  void setPixelColor(uint32_t n, uint32_t c, bool update = true) {}
  void show() {}
  void set_flashled(uint8_t n) {}
  void set_flashleds(uint32_t bitmask) {}
  inline void set_led(uint8_t n) {}
  inline void clear_led(uint8_t n) {}
  inline void toggle_trigled(uint8_t n) {}

};
