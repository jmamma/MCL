#pragma once

// GUI_hardware.h stub for desktop builds

#include <stdint.h>

// Button codes (these are button identifiers)
#define BUTTON_NONE 0
// Note: We can't use BUTTON_DOWN as a code since it's also a macro below

// Number of buttons and encoders
#define GUI_NUM_BUTTONS 16
#define GUI_NUM_ENCODERS 4

// Encoder type
typedef struct {
    int16_t value;
    int16_t old_value;
} encoder_t;

// Button state functions (stub - always returns false/true)
// These match the macro signatures from avr/rp2040 platforms
#define BUTTON_DOWN(button) (false)
#define OLD_BUTTON_UP(button) (true)
#define OLD_BUTTON_DOWN(button) (false)

// Button press/release detection (same pattern as avr/rp2040 platforms)
#define BUTTON_PRESSED(button) (OLD_BUTTON_UP(button) && BUTTON_DOWN(button))
#define BUTTON_RELEASED(button) (OLD_BUTTON_DOWN(button) && !BUTTON_DOWN(button))

// GUI functions
inline uint8_t gui_read_buttons() { return BUTTON_NONE; }
inline bool gui_button_pressed(uint8_t button) { (void)button; return false; }

// Encoder reading (stub)
inline int16_t encoder_get_value(uint8_t enc) { (void)enc; return 0; }
inline void encoder_set_value(uint8_t enc, int16_t val) { (void)enc; (void)val; }
