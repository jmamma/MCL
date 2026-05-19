// GUI_hardware.h — desktop GUI hardware surface.
//
// Mirrors MCL/src/platform/rp2040/GUI_hardware.h verbatim for the non-TBD
// path. The classes are mostly portable C++ logic over mock state; encoder
// readings come from the JUCE side via desktop_gpio_state[] or future input
// shims. For step 1+2 every poll() is empty and reads return zero.
#pragma once

#include "Arduino.h"
#include "hardware.h"
#include "platform.h"
#include "LED_hardware.h"

#include <stdint.h>

class SR165Class {
public:
    SR165Class() = default;
    ALWAYS_INLINE() uint8_t  read()       { return 0; }
    ALWAYS_INLINE() uint16_t read16()     { return 0; }
    ALWAYS_INLINE() uint8_t  read_norst() { return 0; }
};

#define GUI_NUM_ENCODERS 4
#define GUI_NUM_BUTTONS  8

class EncodersClass {
public:
    encoder_t encoders[GUI_NUM_ENCODERS] = {};

    EncodersClass() = default;
    void poll(uint16_t /*sr*/) {}
    void clearEncoders() {
        for (auto& e : encoders) { e.normal = 0; e.button = 0; }
    }

    ALWAYS_INLINE() int8_t getNormal(uint8_t i) { return encoders[i].normal; }
    ALWAYS_INLINE() int8_t limitValue(int8_t v, int8_t lo, int8_t hi) {
        return v > hi ? hi : (v < lo ? lo : v);
    }
};

#define ENCODER_NORMAL(i)         (encoders[(i)].normal)
#define ENCODER_SHIFT(i)          (encoders[(i)].normal)
#define ENCODER_BUTTON(i)         (encoders[(i)].button)
#define ENCODER_BUTTON_SHIFT(i)   (encoders[(i)].button)

#define B_BIT_CURRENT       0
#define B_BIT_OLD           1
#define B_BIT_PRESSED_ONCE  2
#define B_BIT_DOUBLE_CLICK  3
#define B_BIT_CLICK         4
#define B_BIT_LONG_CLICK    5

#define DOUBLE_CLICK_TIME 200
#define LONG_CLICK_TIME   800

// helpers.h provides IS_BIT_SET8/SET_BIT8/CLEAR_BIT8 (MCL convention).
#define B_STATUS(i, bit)           IS_BIT_SET8(Buttons.buttons[i].status, bit)
#define B_SET_STATUS(i, bit)       SET_BIT8(Buttons.buttons[i].status, bit)
#define B_CLEAR_STATUS(i, bit)     CLEAR_BIT8(Buttons.buttons[i].status, bit)
#define B_STORE_STATUS(i, bit, j)  { if (j) B_SET_STATUS(i, bit); else B_CLEAR_STATUS(i, bit); }

#define B_CURRENT(i)               B_STATUS(i, B_BIT_CURRENT)
#define SET_B_CURRENT(i)           B_SET_STATUS(i, B_BIT_CURRENT)
#define CLEAR_B_CURRENT(i)         B_CLEAR_STATUS(i, B_BIT_CURRENT)
#define STORE_B_CURRENT(i, j)      B_STORE_STATUS(i, B_BIT_CURRENT, j)

#define B_OLD(i)                   B_STATUS(i, B_BIT_OLD)
#define SET_B_OLD(i)               B_SET_STATUS(i, B_BIT_OLD)
#define CLEAR_B_OLD(i)             B_CLEAR_STATUS(i, B_BIT_OLD)
#define STORE_B_OLD(i, j)          B_STORE_STATUS(i, B_BIT_OLD, j)

#define B_PRESSED_ONCE(i)          B_STATUS(i, B_BIT_PRESSED_ONCE)
#define SET_B_PRESSED_ONCE(i)      B_SET_STATUS(i, B_BIT_PRESSED_ONCE)
#define CLEAR_B_PRESSED_ONCE(i)    B_CLEAR_STATUS(i, B_BIT_PRESSED_ONCE)

#define B_CLICK(i)                 B_STATUS(i, B_BIT_CLICK)
#define SET_B_CLICK(i)             B_SET_STATUS(i, B_BIT_CLICK)
#define CLEAR_B_CLICK(i)           B_CLEAR_STATUS(i, B_BIT_CLICK)

#define B_DOUBLE_CLICK(i)          B_STATUS(i, B_BIT_DOUBLE_CLICK)
#define SET_B_DOUBLE_CLICK(i)      B_SET_STATUS(i, B_BIT_DOUBLE_CLICK)
#define CLEAR_B_DOUBLE_CLICK(i)    B_CLEAR_STATUS(i, B_BIT_DOUBLE_CLICK)

#define B_LONG_CLICK(i)            B_STATUS(i, B_BIT_LONG_CLICK)
#define SET_B_LONG_CLICK(i)        B_SET_STATUS(i, B_BIT_LONG_CLICK)
#define CLEAR_B_LONG_CLICK(i)      B_CLEAR_STATUS(i, B_BIT_LONG_CLICK)

#define B_PRESS_TIME(i)            (buttons[(i)].press_time)
#define B_LAST_PRESS_TIME(i)       (buttons[(i)].last_press_time)

#define BUTTON_DOWN(b)             (!B_CURRENT(b))
#define BUTTON_UP(b)               (B_CURRENT(b))
#define OLD_BUTTON_DOWN(b)         (!B_OLD(b))
#define OLD_BUTTON_UP(b)           (B_OLD(b))
#define BUTTON_PRESSED(b)          (OLD_BUTTON_UP(b) && BUTTON_DOWN(b))
#define BUTTON_DOUBLE_CLICKED(b)   (B_DOUBLE_CLICK(b))
#define BUTTON_LONG_CLICKED(b)     (B_LONG_CLICK(b))
#define BUTTON_CLICKED(b)          (B_CLICK(b))
#define BUTTON_RELEASED(b)         (OLD_BUTTON_DOWN(b) && BUTTON_UP(b))
#define BUTTON_PRESS_TIME(b)       (clock_diff(B_PRESS_TIME(b), slowclock))

typedef struct button_s {
    uint8_t status;
} button_t;

class ButtonsClass {
public:
    // MCL's button scheme is ACTIVE-LOW: BUTTON_DOWN(b) == !B_CURRENT(b).
    // A button with status=0 reads as pressed, which on boot makes
    // MCL.cpp:178 (BUTTON2 held at startup → boot menu) trigger immediately.
    // Initialise each button's status with the B_BIT_CURRENT bit set so
    // every button reads as released until input plumbing arrives.
    button_t buttons[GUI_NUM_BUTTONS] = {};
    ButtonsClass() {
        for (auto& b : buttons) b.status = (uint8_t)(1u << B_BIT_CURRENT);
    }

    static constexpr uint8_t ENCODER1 = 0;
    static constexpr uint8_t ENCODER2 = 1;
    static constexpr uint8_t ENCODER3 = 2;
    static constexpr uint8_t ENCODER4 = 3;
    static constexpr uint8_t BUTTON1  = 4;
    static constexpr uint8_t BUTTON2  = 5;
    static constexpr uint8_t BUTTON3  = 6;
    static constexpr uint8_t BUTTON4  = 7;

    static constexpr uint16_t ENCODER1_MASK = _BV(ENCODER1);
    static constexpr uint16_t ENCODER2_MASK = _BV(ENCODER2);
    static constexpr uint16_t ENCODER3_MASK = _BV(ENCODER3);
    static constexpr uint16_t ENCODER4_MASK = _BV(ENCODER4);
    static constexpr uint16_t BUTTON1_MASK  = _BV(BUTTON1);
    static constexpr uint16_t BUTTON2_MASK  = _BV(BUTTON2);
    static constexpr uint16_t BUTTON3_MASK  = _BV(BUTTON3);
    static constexpr uint16_t BUTTON4_MASK  = _BV(BUTTON4);

    ALWAYS_INLINE() void clear() {
        // Active-low: B_BIT_CURRENT set == button released.
        for (auto& b : buttons) b.status = (uint8_t)(1u << B_BIT_CURRENT);
    }
    ALWAYS_INLINE() void poll(uint8_t /*sr*/) {}
};

class GUIHardware {
private:
    bool     inGui  = false;
    uint16_t oldsr  = 0;

public:
    ButtonsClass Buttons;
    LEDHardware  led;

    GUIHardware() = default;
    void init()  {}
    void poll()  {}
    void clear() {}

    friend class GuiClass;
};

extern GUIHardware GUI_hardware;
extern SR165Class  SR165;
extern EncodersClass Encoders;
extern ButtonsClass Buttons;
