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
#ifdef PLATFORM_TBD
    enc1_long_press_seen     = false;
    enc1_rotated_while_held  = false;
    enc4_long_press_seen     = false;
    enc4_rotated_while_held  = false;
    tbd_menu_latched         = false;
#endif
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
// TBD physical button -> active-low bit extraction.
// Each macro evaluates true when the named button is pressed.
#define TBD_BUTTON_TOP_LEFT(ui)   (!((ui).f_btns   & (1 << 0)))
#define TBD_BUTTON_TOP_RIGHT(ui)  (!((ui).f_btns   & (1 << 1)))
#define TBD_BUTTON_ENC(ui, n)     (!((ui).f_btns   & (1 << ((n) + 2)))) // n=0..3
#define TBD_BUTTON_LEFT(ui)       (!((ui).mcl_btns & (1 << 0)))
#define TBD_BUTTON_DOWN(ui)       (!((ui).mcl_btns & (1 << 1)))
#define TBD_BUTTON_RIGHT(ui)      (!((ui).mcl_btns & (1 << 2)))
#define TBD_BUTTON_UP(ui)         (!((ui).mcl_btns & (1 << 3)))
#define TBD_BUTTON_A(ui)          (!((ui).mcl_btns & (1 << 4)))
#define TBD_BUTTON_B(ui)          (!((ui).mcl_btns & (1 << 5)))
#define TBD_BUTTON_X(ui)          (!((ui).mcl_btns & (1 << 6)))
#define TBD_BUTTON_Y(ui)          (!((ui).mcl_btns & (1 << 7)))
#define TBD_BUTTON_PLAY(ui)       (!((ui).mcl_btns & (1 << 8)))
#define TBD_BUTTON_REC(ui)        (!((ui).mcl_btns & (1 << 9)))
#define TBD_BUTTON_STOP(ui)       (!((ui).mcl_btns & (1 << 10)))
#define TBD_BUTTON_NO(ui)         (!((ui).mcl_btns & (1 << 11)))
#define TBD_BUTTON_TRIG(ui, n)    (!((ui).d_btns  & (1 << (n)))) // n=0..15

void ButtonsClass::pollTBD(const ui_data_t& ui_data) {
  // Per-press latches for ENC1 (PageSelect toggle) and ENC4 (sticky shift).
  // Each clears on press edge, then latches if (a) the panel flagged a
  // long-press or (b) the encoder rotated while held. Both must be false
  // on release for tbd_handleEvent to honor the tap. Must run BEFORE
  // STORE_B_CURRENT so B_OLD still reflects last cycle.
  // f_btns_long_press: bit 2 = ENC1, bit 5 = ENC4 (= bit (n+2) for ENCn).
  // TBD_BUTTON_ENC follows AVR convention (1 = not pressed, 0 = pressed),
  // so a "pressed" predicate negates it. B_OLD likewise: !B_OLD == pressed.
  {
    const bool enc1_was_pressed = !B_OLD(ENCODER1);
    const bool enc1_is_pressed  = !TBD_BUTTON_ENC(ui_data, 0);
    if (enc1_is_pressed && !enc1_was_pressed) {
      enc1_long_press_seen    = false;
      enc1_rotated_while_held = false;
    }
    if (enc1_is_pressed) {
      if (ui_data.f_btns_long_press & (1 << 2)) enc1_long_press_seen = true;
      if (ui_data.pot_states[0] & 0x03)         enc1_rotated_while_held = true;
    }

    const bool enc4_was_pressed = !B_OLD(ENCODER4);
    const bool enc4_is_pressed  = !TBD_BUTTON_ENC(ui_data, 3);
    if (enc4_is_pressed && !enc4_was_pressed) {
      enc4_long_press_seen    = false;
      enc4_rotated_while_held = false;
    }
    if (enc4_is_pressed) {
      if (ui_data.f_btns_long_press & (1 << 5)) enc4_long_press_seen = true;
      if (ui_data.pot_states[3] & 0x03)         enc4_rotated_while_held = true;
    }
  }

  // Encoder click buttons. ENC1 click is intercepted in tbd_handleEvent
  // as a tap-toggle for MCL PageSelect (open/close); BUTTON2 is only ever
  // synthesized through that interception, never mirrored here.
  for (uint8_t i = 0; i < 4; i++) {
    STORE_B_CURRENT(ENCODER1 + i, TBD_BUTTON_ENC(ui_data, i));
  }
  // BUTTON2 has no TBD physical source — keep it pinned to the
  // "not pressed" state (B_CURRENT==1 in AVR convention) so the BOOT_MENU
  // check in MCL::setup doesn't fire spuriously at boot.
  STORE_B_CURRENT(BUTTON2, 1);

  // Arrow cluster (left -> up -> down -> right)
  STORE_B_CURRENT(FUNC_BUTTON7, TBD_BUTTON_LEFT(ui_data));
  STORE_B_CURRENT(FUNC_BUTTON6, TBD_BUTTON_UP(ui_data));
  STORE_B_CURRENT(FUNC_BUTTON8, TBD_BUTTON_DOWN(ui_data));
  STORE_B_CURRENT(FUNC_BUTTON9, TBD_BUTTON_RIGHT(ui_data));

  // Transport row (play / rec / stop / no)
  STORE_B_CURRENT(FUNC_BUTTON2, TBD_BUTTON_PLAY(ui_data));
  STORE_B_CURRENT(FUNC_BUTTON1, TBD_BUTTON_REC(ui_data));
  STORE_B_CURRENT(FUNC_BUTTON3, TBD_BUTTON_STOP(ui_data));
  STORE_B_CURRENT(FUNC_BUTTON5, TBD_BUTTON_NO(ui_data));

  // MCL universal slots:
  //   TOP_LEFT -> BUTTON1 (left action: NO/save/cancel, page-dependent)
  //   TOP_RIGHT-> BUTTON4 (right action: YES/load/confirm, page-dependent)
  // BUTTON2 (MCL PageSelect) is driven only by ENC1 click above.
  // BUTTON3 mirrors the ENC4 page-shift-menu latch. SeqPage and GridPage
  // open their per-page shift menu on BUTTON3 press and apply on release,
  // so toggling tbd_menu_latched here synthesizes the same event pair an
  // AVR rig produces by holding/releasing a hardware shift key. The MDX
  // passthrough modifier is a separate input on MCL_B (TBD_KEY_SPS).
  STORE_B_CURRENT(BUTTON1, TBD_BUTTON_TOP_LEFT(ui_data));
  STORE_B_CURRENT(BUTTON4, TBD_BUTTON_TOP_RIGHT(ui_data));
  STORE_B_CURRENT(BUTTON3, !tbd_menu_latched);

  // Cluster: left column = modifiers, right column = direct MD actions.
  //   MCL_Y (TL) -> MDX_KEY_FUNC
  //   MCL_X (TR) -> MDX_KEY_PAGE
  //   MCL_A (BR) -> MDX_KEY_YES
  //   MCL_B (BL) -> MDX_KEY_SPS  (held = reroute panel to MDX passthrough)
  STORE_B_CURRENT(TBD_KEY_FUNC, TBD_BUTTON_Y(ui_data));
  STORE_B_CURRENT(TBD_KEY_PAGE, TBD_BUTTON_X(ui_data));
  STORE_B_CURRENT(TBD_KEY_YES,  TBD_BUTTON_A(ui_data));
  STORE_B_CURRENT(TBD_KEY_SPS,  TBD_BUTTON_B(ui_data));

  // Sequencer Buttons
  for (int i = 0; i < 16; i++) {
    STORE_B_CURRENT(i + TRIG_BUTTON1, TBD_BUTTON_TRIG(ui_data, i));
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
