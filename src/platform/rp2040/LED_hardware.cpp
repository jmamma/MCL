#include "LED.h"
#include "global.h"
#include "helpers.h"
#include "platform.h"

#ifdef PLATFORM_TBD
#include "Ui.h"

void LEDHardware::show() {
  uint16_t current_time = read_clock_ms();
  uint32_t final_render_state = led_base_state;
  if (clock_diff(blink_last_trigger_time, current_time) > LED_BLINK_PERIOD) {
    blink_last_trigger_time = current_time;
    uint32_t blink_mask = led_blink_mask;
    uint32_t flash_mask = led_flash_mask;
    for (uint8_t i = 0; i < 32; i++) {
      if (blink_mask & 1 && !(flash_mask & 1)) {
        SET_BIT32(led_flash_mask, i);
        led_flash_start_time[i] = current_time;
      }
      flash_mask >>= 1;
      blink_mask >>= 1;
    }
  }


  uint32_t flash_mask = led_flash_mask;
  for (uint8_t i = 0; i < 32; i++) {
    if (flash_mask & 1) {
      if (clock_diff(led_flash_start_time[i], current_time) <
          LED_FLASH_DURATION) {
        TOGGLE_BIT32(final_render_state, i);
      } else {
        CLEAR_BIT32(led_flash_mask, i);
      }
    }
    flash_mask >>= 1;
  }

  if (final_render_state != last_render_state) {
    updateLeds = true;
  }
  if (updateLeds) {
    last_render_state = final_render_state;
    for (uint8_t i = 0; i < 32; i++) {
      bool is_on = final_render_state & 1;
      uint8_t id = 255;
      if (i < 16) { //Trigs
        id = tbd_ui.rgb_led_btn_map[i];
      }
      if (i == STRIP_LED1) {
        id = tbd_ui.rgb_led_fbtn_map[2];
      }
      if (i == STRIP_LED2) {
        id = tbd_ui.rgb_led_fbtn_map[1];
      }

      if (id < 255) {
        setPixelColor(id, is_on ? STRIP_RED : STRIP_BLACK, false);
      }
      final_render_state >>= 1;
    }

    tbd_ui.strip.show();
    updateLeds = false;
  }
}

void LEDHardware::set_trigleds(uint16_t bitmask, TrigLEDMode mode, bool blink,
                        bool update) {
  current_led_mode = mode;
  if (blink) {
    led_blink_mask = (led_blink_mask & 0xFFFF0000) | bitmask;
  } else {
    led_base_state = (led_base_state & 0xFFFF0000) | bitmask;  // Preserve upper 16 bits, set lower 16 bits
  }
  if (update) {
    updateLeds = true;
  }
}
void LEDHardware::set_flashled(uint8_t n) {
  if ((n < 16) && (current_led_mode == TRIGLED_STEPEDIT || current_led_mode == TRIGLED_MUTE || current_led_mode == TRIGLED_EXCLUSIVE)) { 
        return; }
  if (n < 32) {
    SET_BIT32(led_flash_mask, n);
    led_flash_start_time[n] = read_clock_ms();
  }
}

void LEDHardware::set_flashleds(uint32_t bitmask) {
 uint16_t current_time = read_clock_ms();
  for (uint8_t i = 0; i < 32; i++) {
   if ((i < 16) && (current_led_mode == TRIGLED_STEPEDIT || current_led_mode == TRIGLED_MUTE || current_led_mode == TRIGLED_EXCLUSIVE)) { 
        return; }
          if ((bitmask >> i) & 1) {
      SET_BIT32(led_flash_mask, i);
      led_flash_start_time[i] = current_time;
    }
  }
}

void LEDHardware::setPixelColor(uint32_t n, uint32_t c, bool update) {
  tbd_ui.strip.setPixelColor(n, c);
  if (update) {
    updateLeds = true;
  }
}
#endif
