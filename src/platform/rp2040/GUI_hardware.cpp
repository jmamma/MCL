#include "GUI_hardware.h"
#include "helpers.h"
#include "platform.h"
#include "global.h"
#include "hardware/gpio.h"
#include "hardware/structs/sio.h"

#ifdef PLATFORM_TBD
#include "Ui.h"
#include "oled.h"
#endif

// Pin definitions - keeping original pins
#define SR165_OUT    12
#define SR165_SHLOAD 6
#define SR165_CLK    7

// Create masks for single-cycle operations
#define SR165_OUT_MASK    (1u << SR165_OUT)
#define SR165_SHLOAD_MASK (1u << SR165_SHLOAD)
#define SR165_CLK_MASK    (1u << SR165_CLK)

#define SR165_DELAY() { } // asm("nop"); } // asm("nop");  asm("nop");  }

ALWAYS_INLINE() void SR165Class::clk() {
    sio_hw->gpio_clr = SR165_CLK_MASK;    // Single cycle clear
    SR165_DELAY();
    sio_hw->gpio_set = SR165_CLK_MASK;    // Single cycle set
}

ALWAYS_INLINE() void SR165Class::rst() {
    sio_hw->gpio_clr = SR165_SHLOAD_MASK;  // Single cycle clear
    SR165_DELAY();
    sio_hw->gpio_set = SR165_SHLOAD_MASK;  // Single cycle set
}

SR165Class::SR165Class() {
}

ALWAYS_INLINE() uint8_t SR165Class::read() {
    rst();
    uint8_t res = 0;
    for (uint8_t i = 0; i < 8; i++) {
        res <<= 1;
        res |= ((sio_hw->gpio_in & SR165_OUT_MASK) ? 1 : 0); // Direct read
        clk();
    }
    return res;
}

ALWAYS_INLINE() uint8_t SR165Class::read_norst() {
    uint8_t res = 0;
    for (uint8_t i = 0; i < 8; i++) {
        res <<= 1;
        res |= ((sio_hw->gpio_in & SR165_OUT_MASK) ? 1 : 0); // Direct read
        clk();
    }
    return res;
}

ALWAYS_INLINE() uint16_t SR165Class::read16() {
    rst();
    uint16_t res = 0;
    for (uint8_t i = 0; i < 16; i++) {
        res <<= 1;
        res |= ((sio_hw->gpio_in & SR165_OUT_MASK) ? 1 : 0); // Direct read
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
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
        ENCODER_NORMAL(i) = ENCODER_BUTTON(i) = 0;
    }
}

void EncodersClass::poll(uint16_t sr) {
    uint16_t sr_tmp = sr;
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
        if ((sr & 3) != (sr_old & 3)) {
            volatile int8_t *val = &(ENCODER_NORMAL(i));
            volatile int8_t *button = &(ENCODER_BUTTON(i));
            *button = BUTTON_DOWN(i);

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

void EncodersClass::pollTBD(const ui_data_t& ui_data) {
   for (uint8_t i = 0; i < GUI_NUM_ENCODERS && i < 4; i++) {
        volatile int8_t *val = &(ENCODER_NORMAL(i));
        volatile int8_t *button = &(ENCODER_BUTTON(i));
        *button = BUTTON_DOWN(i);

        // rev_c: use pot_states bits (BIT0=fwd, BIT1=bwd)
        if (ui_data.pot_states[i] & (1 << 0)) {
          if (*val < 64) (*val)++;
        }
        if (ui_data.pot_states[i] & (1 << 1)) {
          if (*val > -64) (*val)--;
        }
   }
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
void ButtonsClass::pollTBD(const ui_data_t& ui_data) {
  // rev_c button mapping

  // f_btns: 0:F1, 1:F2 -> BUTTON1, BUTTON2
  STORE_B_CURRENT(BUTTON1, !(ui_data.f_btns & (1 << 0)));
  STORE_B_CURRENT(BUTTON2, !(ui_data.mcl_btns & (1 << 5)));

  // f_btns: 2:POT1, 3:POT2, 4:POT3, 5:POT4 -> Encoder buttons
  for (uint8_t i = 0; i < 4; i++) {
    STORE_B_CURRENT(ENCODER1 + i, !(ui_data.f_btns & (1 << (i + 2))));
  }

  // Direction buttons: LEFT=bit0, DOWN=bit1, RIGHT=bit2, UP=bit3
  STORE_B_CURRENT(FUNC_BUTTON7, !(ui_data.mcl_btns & (1 << 0))); // LEFT
  STORE_B_CURRENT(FUNC_BUTTON8, !(ui_data.mcl_btns & (1 << 1))); // DOWN
  STORE_B_CURRENT(FUNC_BUTTON9, !(ui_data.mcl_btns & (1 << 2))); // RIGHT
  STORE_B_CURRENT(FUNC_BUTTON6, !(ui_data.mcl_btns & (1 << 3))); // UP

  // PLAY=bit8, REC=bit9, STOP=bit10
  STORE_B_CURRENT(FUNC_BUTTON2, !(ui_data.mcl_btns & (1 << 8)));  // PLAY
  STORE_B_CURRENT(FUNC_BUTTON1, !(ui_data.mcl_btns & (1 << 9)));  // REC
  STORE_B_CURRENT(FUNC_BUTTON3, !(ui_data.mcl_btns & (1 << 10))); // STOP

  STORE_B_CURRENT(BUTTON3, !(ui_data.mcl_btns & (1 << 4)));
  STORE_B_CURRENT(BUTTON4, !(ui_data.f_btns & (1 << 1)));

  // Sequencer Buttons
  for (int i = 0; i < 16; i++) {
    bool state = ui_data.d_btns & (1 << i);
    STORE_B_CURRENT(i + TRIG_BUTTON1, !state);
  }
}

void GUIHardware::poll() {
    if (inGui) {
        return;
    }

    inGui = true;
#ifndef PLATFORM_TBD
    uint16_t sr = SR165.read16();
    if (sr != oldsr) {
        Buttons.clear();
        Buttons.poll(sr >> 8);
        Encoders.poll(sr);
        oldsr = sr;
        GUI.events.pollEvents();
    }
#else
    if (tbd_ui.UpdateUIInputs()) {
      ui_data_t ui_data_current = tbd_ui.CopyUiData();
      if (ui_data_current.systicks != last_ui_systicks) {
        Buttons.clear();
        Buttons.pollTBD(ui_data_current);
        Encoders.pollTBD(ui_data_current);
        last_ui_systicks = ui_data_current.systicks;
        GUI.events.pollEvents();
      }
    }
#endif
    led.show();
    inGui = false;
}

void GUIHardware::init() {
    // Initialize GPIO pins
#ifndef PLATFORM_TBD
    gpio_init(SR165_OUT);
    gpio_init(SR165_SHLOAD);
    gpio_init(SR165_CLK);
    gpio_set_dir(SR165_OUT, GPIO_IN);
    gpio_set_dir(SR165_SHLOAD, GPIO_OUT);
    gpio_set_dir(SR165_CLK, GPIO_OUT);
    // Set initial states using SIO for fastest operation
    sio_hw->gpio_set = SR165_CLK_MASK | SR165_SHLOAD_MASK;
    delay(10);
    uint16_t sr = SR165.read16();
    Buttons.clear();
    Buttons.poll(sr >> 8);
    Encoders.poll(sr);
    oldsr = sr;
#else
    last_ui_systicks = 0;
    tbd_ui.InitHardware();
    tbd_ui.InitLeds();
    tbd_ui.strip.show();
    tbd_ui.strip.setBrightness(5);
    // Read initial button state to prevent spurious release events on boot
    if (tbd_ui.UpdateUIInputs()) {
      ui_data_t ui_data_current = tbd_ui.CopyUiData();
      Buttons.clear();
      Buttons.pollTBD(ui_data_current);
      Encoders.pollTBD(ui_data_current);
      last_ui_systicks = ui_data_current.systicks;
    }
#endif
}

void GUIHardware::clear() {
   Buttons.clear();
   Encoders.clearEncoders();
}

GUIHardware GUI_hardware;
SR165Class SR165;
EncodersClass Encoders;
ButtonsClass Buttons;
