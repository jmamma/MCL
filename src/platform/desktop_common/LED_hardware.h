// LED_hardware.h — desktop/WASM virtual LED hardware.
//
// The desktop platform has no physical strip, but it must retain the output
// state produced by MCL so a host can render the selected panel. Logical
// outputs match the TBD-16 firmware surface: 16 trigger LEDs followed by the
// two device/status LEDs and the record/step-edit LED.
#pragma once

#define LED_BLINK_PERIOD   1000
#define LED_FLASH_DURATION 50

#include "LED.h"
#include "platform.h"

#define COLOR(r, g, b) (uint32_t)(((r) << 16) | ((g) << 8) | (b))
#define STRIP_RED      COLOR(255, 0, 0)
#define STRIP_WHITE    COLOR(255, 255, 255)
#define STRIP_BLACK    COLOR(0, 0, 0)
#define STRIP_LED1 16
#define STRIP_LED2 17
#define STRIP_LED3 18

class LEDHardware : public LED {
public:
    static constexpr uint8_t PANEL_LED_COUNT = 19;

    LEDHardware();

    void set_trigleds(uint16_t bitmask, TrigLEDMode mode,
                      bool blink = false, bool update = true);
    void set_trigleds_color(uint16_t bitmask, uint32_t rgb);
    void set_trigleds_blink_color(uint16_t bitmask, uint32_t rgb);
    void clear_trigleds();
    void reset_trigleds();
    void setPixelColor(uint32_t n, uint32_t c, bool update = true);
    void show();
    void set_flashled(uint8_t n);
    void set_flashleds(uint32_t bitmask);
    void set_led(uint8_t n);
    void clear_led(uint8_t n);
    void toggle_trigled(uint8_t n);

    const uint32_t* panelLedColors() const { return rendered_colors_; }
    uint32_t panelLedColor(uint8_t n) const {
        return n < PANEL_LED_COUNT ? rendered_colors_[n] : 0;
    }

    bool rec_active = false;

private:
    uint32_t led_base_state_ = 0;
    uint32_t led_blink_mask_ = 0;
    uint32_t led_flash_mask_ = 0;
    uint32_t led_flash_start_time_[32] = {};
    uint32_t last_render_state_ = 0;
    uint16_t blink_last_trigger_time_ = 0;
    bool last_blink_hint_ = false;
    bool trig_color_override_ = false;
    bool update_leds_ = true;
    TrigLEDMode current_led_mode_ = TRIGLED_OVERLAY;
    uint32_t trig_colors_[16] = {};
    uint32_t rendered_colors_[PANEL_LED_COUNT] = {};
};
