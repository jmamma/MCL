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
