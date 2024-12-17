#include "GUI_hardware.h"
#include "Core.h"
#include "helpers.h"
#include "platform.h"
#include "hardware/structs/sio.h"

// Pin definitions - adjust these to match your wiring
#define SR165_OUT    0  // GPIO0
#define SR165_SHLOAD 1  // GPIO1
#define SR165_CLK    2  // GPIO2

// Create masks for faster operations
#define SR165_CLK_MASK    (1u << SR165_CLK)
#define SR165_SHLOAD_MASK (1u << SR165_SHLOAD)
#define SR165_OUT_MASK    (1u << SR165_OUT)

#define SR165_DELAY() { } // asm("nop"); } // asm("nop");  asm("nop");  }

ALWAYS_INLINE() void SR165Class::clk() {
    sio_hw->gpio_clr = SR165_CLK_MASK;
    sio_hw->gpio_set = SR165_CLK_MASK;
}

ALWAYS_INLINE() void SR165Class::rst() {
    sio_hw->gpio_clr = SR165_SHLOAD_MASK;
    sio_hw->gpio_set = SR165_SHLOAD_MASK;
}

SR165Class::SR165Class() {
    pinMode(SR165_OUT, INPUT);
    pinMode(SR165_SHLOAD, OUTPUT);
    pinMode(SR165_CLK, OUTPUT);
    
    // Set initial states using SIO
    sio_hw->gpio_set = SR165_CLK_MASK | SR165_SHLOAD_MASK;
}

ALWAYS_INLINE() uint8_t SR165Class::read() {
    rst();

    uint8_t res = 0;
    for (uint8_t i = 0; i < 8; i++) {
        res <<= 1;
        res |= ((sio_hw->gpio_in & SR165_OUT_MASK) ? 1 : 0);
        clk();
    }

    return res;
}

ALWAYS_INLINE() uint8_t SR165Class::read_norst() {
    uint8_t res = 0;
    for (uint8_t i = 0; i < 8; i++) {
        res <<= 1;
        res |= ((sio_hw->gpio_in & SR165_OUT_MASK) ? 1 : 0);
        clk();
    }

    return res;
}

ALWAYS_INLINE() uint16_t SR165Class::read16() {
    rst();

    uint16_t res = 0;
    for (uint8_t i = 0; i < 16; i++) {
        res <<= 1;
        res |= ((sio_hw->gpio_in & SR165_OUT_MASK) ? 1 : 0);
        clk();
    }

    return res;
}

/**********************************************/

EncodersClass::EncodersClass() {
    clearEncoders();
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
        sr_old2s[i] = 0;
    }
    sr_old = 0;
}

void EncodersClass::clearEncoders() {
    // USE_LOCK();
    // SET_LOCK();
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
        ENCODER_NORMAL(i) = ENCODER_BUTTON(i) = 0;
    }
    // CLEAR_LOCK();
}

void EncodersClass::poll(uint16_t sr) {
    uint16_t sr_tmp = sr;
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
        if ((sr & 3) != (sr_old & 3)) {
            volatile int8_t *val = &(ENCODER_NORMAL(i));
            if (BUTTON_DOWN(i)) {
                val = &(ENCODER_BUTTON(i));
            }

            if (((sr_old2s[i] & 3) == 0 && (sr_old & 3) == 1 && (sr & 3) == 3) ||
                (((sr_old2s[i] & 3) == 3) && (sr_old & 3) == 2 && (sr & 3) == 0)) {
                if (*val < 64)
                    (*val)++;
            } else if (((sr_old2s[i] & 3) == 0 && (sr_old & 3) == 2 && (sr & 3) == 3) ||
                     ((sr_old2s[i] & 3) == 3 && (sr_old & 3) == 1 && (sr & 3) == 0)) {
                if (*val > -64)
                    (*val)--;
            }
            sr_old2s[i] = sr_old & 3;
        }
        sr >>= 2;
        sr_old >>= 2;
    }

    sr_old = sr_tmp;
}

/**********************************************/

ButtonsClass::ButtonsClass() {
    clear();
}

void ButtonsClass::clear() {
    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; i++) {
        CLEAR_B_DOUBLE_CLICK(i);
        CLEAR_B_CLICK(i);
        CLEAR_B_LONG_CLICK(i);
        STORE_B_OLD(i, B_CURRENT(i));
    }
}

void ButtonsClass::poll(uint8_t but) {
    uint8_t but_tmp = but;

    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; i++) {
        STORE_B_CURRENT(i, IS_BIT_SET8(but_tmp, 0));
        but_tmp >>= 1;
    }
}

SR165Class SR165;
EncodersClass Encoders;
ButtonsClass Buttons;
