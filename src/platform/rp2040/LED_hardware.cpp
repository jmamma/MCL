#include "LED.h"
#include "global.h"
#include "helpers.h"
#include "platform.h"
#include "MidiClock.h"

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

  // REC button LED blink hint (only when live recording active)
  if (rec_active) {
    bool hint = MidiClock.getBlinkHint(true);
    if (hint != last_blink_hint) {
      last_blink_hint = hint;
      updateLeds = true;
    }
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
        id = tbd_ui.rgb_led_fbtn_map[1]; // restored original mapping
      }

      if (id < 255) {
        uint32_t color;
        if (i < 16 && trig_color_override) {
          if ((led_blink_mask >> i) & 1) {
            // Colored blink: toggle visibility while keeping the colour.
            color = is_on ? trig_colors[i] : STRIP_BLACK;
          } else {
            color = trig_colors[i];
          }
        } else {
          color = is_on ? STRIP_RED : STRIP_BLACK;
        }
        setPixelColor(id, color, false);
      }
      final_render_state >>= 1;
    }

    // REC button LED (fbtn_map[0]): live record blink takes priority over step-edit
    bool rec_led_on = rec_active ? last_blink_hint
                                 : (bool)((led_base_state >> STRIP_LED3) & 1);
    setPixelColor(tbd_ui.rgb_led_fbtn_map[0], rec_led_on ? STRIP_RED : STRIP_BLACK, false);

    // STRIP_LED2 (fbtn_map[1]) doubles as the SPS-mode latch indicator —
    // amber while latched, otherwise the normal led_base_state-driven red.
    if (sps_active) {
      setPixelColor(tbd_ui.rgb_led_fbtn_map[1], COLOR(255, 96, 0), false);
    }

    tbd_ui.strip.show();
    updateLeds = false;
  }
}

void LEDHardware::set_trigleds(uint16_t bitmask, TrigLEDMode mode, bool blink,
                        bool update) {
  // Page-owned LED gate. Mirrors set_flashled's existing convention:
  // when a page has claimed the panel via TRIGLED_EXCLUSIVENDYNAMIC
  // (e.g. BankPopupPage's coloured chain LEDs, GridPage's load popup),
  // seq-driven STEPEDIT writes are dropped so the page's palette
  // survives. The page itself can still write any mode it wants —
  // the gate only blocks STEPEDIT specifically.
  if (current_led_mode == TRIGLED_EXCLUSIVENDYNAMIC &&
      mode == TRIGLED_STEPEDIT) {
    return;
  }
  current_led_mode = mode;
  // Plain set_trigleds returns to monochrome rendering — colour override
  // ends here; the next caller has to re-arm it explicitly.
  if (trig_color_override) {
    trig_color_override = false;
    memset(trig_colors, 0, sizeof(trig_colors));
  }
  if (mode == TRIGLED_STEPEDIT) {
    SET_BIT32(led_base_state, STRIP_LED3);
  } else {
    CLEAR_BIT32(led_base_state, STRIP_LED3);
  }
  if (blink) {
    led_blink_mask = (led_blink_mask & 0xFFFF0000) | bitmask;
  } else {
    led_base_state = (led_base_state & 0xFFFF0000) | bitmask;  // Preserve upper 16 bits, set lower 16 bits
  }
  if (update) {
    updateLeds = true;
  }
}
void LEDHardware::set_trigleds_color(uint16_t bitmask, uint32_t rgb) {
  trig_color_override = true;
  for (uint8_t i = 0; i < 16; i++) {
    if (bitmask & (1u << i)) {
      trig_colors[i] = rgb;
      // Solid colour: ensure blink isn't lingering on these bits.
      CLEAR_BIT32(led_blink_mask, i);
    }
  }
  // Force a render — final_render_state matters less in override mode but
  // show() bails when it equals last_render_state, so nudge updateLeds.
  updateLeds = true;
  last_render_state = ~last_render_state;
}

void LEDHardware::set_trigleds_blink_color(uint16_t bitmask, uint32_t rgb) {
  trig_color_override = true;
  for (uint8_t i = 0; i < 16; i++) {
    if (bitmask & (1u << i)) {
      trig_colors[i] = rgb;
      SET_BIT32(led_blink_mask, i);
    }
  }
  updateLeds = true;
  last_render_state = ~last_render_state;
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
