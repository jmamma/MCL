#pragma once

#define LED_BLINK_PERIOD 1000
#define LED_FLASH_DURATION 50
#include "LED.h"
#include "platform.h"

#define COLOR(r, g, b) tbd_ui.strip.Color(r, g, b)
#define STRIP_RED COLOR(255, 0, 0)
#define STRIP_BLACK COLOR(0, 0, 0)
#define STRIP_LED1 16
#define STRIP_LED2 17

// #define STRIP_LED1 tbd_ui.rgb_led_fbtn_map[2]
// #define STRIP_LED2 tbd_ui.rgb_led_fbtn_map[1]


class LEDHardware : public LED {
#ifndef PLATFORM_TBD
public:
  LEDHardware() : LED() {}
};
#else
private:
  uint32_t led_base_state;
  uint32_t led_blink_mask;
  uint32_t led_flash_mask;
  uint32_t led_flash_start_time[32];
  uint32_t last_render_state;
  uint32_t blink_last_trigger_time;

public:
  TrigLEDMode current_led_mode;
  bool updateLeds;
  LEDHardware()
      : led_base_state(0), led_blink_mask(0), led_flash_mask(0),
        last_render_state(0), blink_last_trigger_time(0), updateLeds(true),
        current_led_mode(TRIGLED_OVERLAY) {
    memset(led_flash_start_time, 0, sizeof(led_flash_start_time));
  }

  void set_trigleds(uint16_t bitmask, TrigLEDMode mode, bool blink = false,
                    bool update = true);
  void clear_trigleds() {
     set_trigleds(0, current_led_mode, false, false);
     set_trigleds(0, current_led_mode, true, true);
  }
  void reset_trigleds() {
     clear_trigleds();
     current_led_mode = TRIGLED_OVERLAY;
  }
  void setPixelColor(uint32_t n, uint32_t c, bool update = true);
  void show();
  void set_flashled(uint8_t n);
  void set_flashleds(uint32_t bitmask);
  inline void set_led(uint8_t n) {
    if (n < 32) {
      SET_BIT32(led_base_state, n);
      updateLeds = true;
    }
  }
  inline void clear_led(uint8_t n) {
    if (n < 32) {
      CLEAR_BIT32(led_base_state, n);
      updateLeds = true;
    }
  }
  inline void toggle_trigled(uint8_t n) {
    if (n < 16) {
      TOGGLE_BIT32(led_base_state, n);
      updateLeds = true;
    }
  }

};
#endif
