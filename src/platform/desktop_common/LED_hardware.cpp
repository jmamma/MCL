#include "LED_hardware.h"
#include "MidiClock.h"
#include "helpers.h"
#include "platform.h"

#include <string.h>

LEDHardware::LEDHardware() : LED() {
  memset(led_flash_start_time_, 0, sizeof(led_flash_start_time_));
  memset(trig_colors_, 0, sizeof(trig_colors_));
  memset(rendered_colors_, 0, sizeof(rendered_colors_));
}

void LEDHardware::show() {
  const uint16_t current_time = read_clock_ms();
  uint32_t final_render_state = led_base_state_;

  if (clock_diff(blink_last_trigger_time_, current_time) > LED_BLINK_PERIOD) {
    blink_last_trigger_time_ = current_time;
    uint32_t blink_mask = led_blink_mask_;
    uint32_t flash_mask = led_flash_mask_;
    for (uint8_t i = 0; i < 32; ++i) {
      if ((blink_mask & 1u) && !(flash_mask & 1u)) {
        SET_BIT32(led_flash_mask_, i);
        led_flash_start_time_[i] = current_time;
      }
      blink_mask >>= 1;
      flash_mask >>= 1;
    }
  }

  uint32_t flash_mask = led_flash_mask_;
  for (uint8_t i = 0; i < 32; ++i) {
    if (flash_mask & 1u) {
      if (clock_diff(led_flash_start_time_[i], current_time) <
          LED_FLASH_DURATION) {
        TOGGLE_BIT32(final_render_state, i);
      } else {
        CLEAR_BIT32(led_flash_mask_, i);
      }
    }
    flash_mask >>= 1;
  }

  if (rec_active) {
    const bool hint = MidiClock.getBlinkHint(true);
    if (hint != last_blink_hint_) {
      last_blink_hint_ = hint;
      update_leds_ = true;
    }
  }

  if (final_render_state != last_render_state_)
    update_leds_ = true;
  if (!update_leds_)
    return;

  last_render_state_ = final_render_state;
  for (uint8_t i = 0; i < 16; ++i) {
    const bool is_on = (final_render_state & (1u << i)) != 0;
    if (trig_color_override_) {
      rendered_colors_[i] = ((led_blink_mask_ >> i) & 1u)
          ? (is_on ? trig_colors_[i] : STRIP_BLACK)
          : trig_colors_[i];
    } else {
      rendered_colors_[i] = is_on ? STRIP_RED : STRIP_BLACK;
    }
  }

  rendered_colors_[STRIP_LED1] =
      (final_render_state & (1u << STRIP_LED1)) ? STRIP_RED : STRIP_BLACK;
  rendered_colors_[STRIP_LED2] =
      (final_render_state & (1u << STRIP_LED2)) ? STRIP_RED : STRIP_BLACK;
  const bool rec_led_on = rec_active
      ? last_blink_hint_
      : ((led_base_state_ & (1u << STRIP_LED3)) != 0);
  rendered_colors_[STRIP_LED3] = rec_led_on ? STRIP_RED : STRIP_BLACK;
  update_leds_ = false;
}

void LEDHardware::set_trigleds(uint16_t bitmask, TrigLEDMode mode,
                               bool blink, bool update) {
  if (current_led_mode_ == TRIGLED_EXCLUSIVENDYNAMIC &&
      mode == TRIGLED_STEPEDIT) {
    return;
  }
  current_led_mode_ = mode;
  if (trig_color_override_) {
    trig_color_override_ = false;
    memset(trig_colors_, 0, sizeof(trig_colors_));
  }
  if (mode == TRIGLED_STEPEDIT)
    SET_BIT32(led_base_state_, STRIP_LED3);
  else
    CLEAR_BIT32(led_base_state_, STRIP_LED3);

  if (blink)
    led_blink_mask_ = (led_blink_mask_ & 0xffff0000u) | bitmask;
  else
    led_base_state_ = (led_base_state_ & 0xffff0000u) | bitmask;
  if (update)
    update_leds_ = true;
}

void LEDHardware::set_trigleds_color(uint16_t bitmask, uint32_t rgb) {
  trig_color_override_ = true;
  for (uint8_t i = 0; i < 16; ++i) {
    if (bitmask & (1u << i)) {
      trig_colors_[i] = rgb;
      CLEAR_BIT32(led_blink_mask_, i);
    }
  }
  update_leds_ = true;
  last_render_state_ = ~last_render_state_;
}

void LEDHardware::set_trigleds_blink_color(uint16_t bitmask, uint32_t rgb) {
  trig_color_override_ = true;
  for (uint8_t i = 0; i < 16; ++i) {
    if (bitmask & (1u << i)) {
      trig_colors_[i] = rgb;
      SET_BIT32(led_blink_mask_, i);
    }
  }
  update_leds_ = true;
  last_render_state_ = ~last_render_state_;
}

void LEDHardware::clear_trigleds() {
  set_trigleds(0, current_led_mode_, false, false);
  set_trigleds(0, current_led_mode_, true, true);
}

void LEDHardware::reset_trigleds() {
  clear_trigleds();
  current_led_mode_ = TRIGLED_OVERLAY;
  CLEAR_BIT32(led_base_state_, STRIP_LED3);
  update_leds_ = true;
}

void LEDHardware::setPixelColor(uint32_t n, uint32_t c, bool update) {
  if (n < PANEL_LED_COUNT)
    rendered_colors_[n] = c & 0x00ffffffu;
  if (update)
    update_leds_ = true;
}

void LEDHardware::set_flashled(uint8_t n) {
  if (n < 16 && (current_led_mode_ == TRIGLED_STEPEDIT ||
                 current_led_mode_ == TRIGLED_MUTE ||
                 current_led_mode_ == TRIGLED_EXCLUSIVE)) {
    return;
  }
  if (n < 32) {
    SET_BIT32(led_flash_mask_, n);
    led_flash_start_time_[n] = read_clock_ms();
    update_leds_ = true;
  }
}

void LEDHardware::set_flashleds(uint32_t bitmask) {
  const uint16_t current_time = read_clock_ms();
  for (uint8_t i = 0; i < 32; ++i) {
    if (i < 16 && (current_led_mode_ == TRIGLED_STEPEDIT ||
                   current_led_mode_ == TRIGLED_MUTE ||
                   current_led_mode_ == TRIGLED_EXCLUSIVE)) {
      continue;
    }
    if ((bitmask >> i) & 1u) {
      SET_BIT32(led_flash_mask_, i);
      led_flash_start_time_[i] = current_time;
    }
  }
  update_leds_ = true;
}

void LEDHardware::set_led(uint8_t n) {
  if (n < 32) {
    SET_BIT32(led_base_state_, n);
    update_leds_ = true;
  }
}

void LEDHardware::clear_led(uint8_t n) {
  if (n < 32) {
    CLEAR_BIT32(led_base_state_, n);
    update_leds_ = true;
  }
}

void LEDHardware::toggle_trigled(uint8_t n) {
  if (n < 16) {
    TOGGLE_BIT32(led_base_state_, n);
    update_leds_ = true;
  }
}
