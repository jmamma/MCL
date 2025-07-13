#pragma once

#include <inttypes.h>
#include "Core.h"
#include "platform.h"
#include "hardware.h"

#ifdef PLATFORM_TBD
#include "Ui.h"
extern class Ui tbd_ui;
#endif

class SR165Class {
  inline void rst();
  inline void clk();
 public:
  SR165Class();
  ALWAYS_INLINE() uint8_t read();
  ALWAYS_INLINE() uint16_t read16();
  ALWAYS_INLINE() uint8_t read_norst();
};

#ifdef PLATFORM_TBD
#define GUI_NUM_ENCODERS 4
#define GUI_NUM_BUTTONS  4 + 5 + 13 + 16
#else
#define GUI_NUM_ENCODERS 4
#define GUI_NUM_BUTTONS  8
#endif

class EncodersClass {
  uint16_t sr_old;
  uint8_t sr_old2s[GUI_NUM_ENCODERS];
#ifdef PLATFORM_TBD
  uint16_t pot_old_positions[GUI_NUM_ENCODERS];
#endif
 public:
  encoder_t encoders[GUI_NUM_ENCODERS];

  EncodersClass();

  void poll(uint16_t sr);
  void clearEncoders();

#ifdef PLATFORM_TBD
  void pollTBD(const ui_data_t& ui_data);
#endif

  ALWAYS_INLINE() int8_t getNormal(uint8_t i) { return encoders[i].normal; }
  //ALWAYS_INLINE() int8_t getButton(uint8_t i) { return encoders[i].button; }

  ALWAYS_INLINE() int8_t limitValue(int8_t value, int8_t min, int8_t max) {
    return (value > max) ? max : (value < min ? min : value);
  }
};
#define ENCODER_NORMAL(i) (encoders[(i)].normal)
#define ENCODER_SHIFT(i)  (encoders[(i)].shift)
#define ENCODER_BUTTON(i) (encoders[(i)].button)
#define ENCODER_BUTTON_SHIFT(i) (encoders[(i)].button_shift)

#define B_BIT_CURRENT        0
#define B_BIT_OLD            1
#define B_BIT_PRESSED_ONCE   2
#define B_BIT_DOUBLE_CLICK   3
#define B_BIT_CLICK          4
#define B_BIT_LONG_CLICK     5

// XXX adjust these to correct length of irq time
#define DOUBLE_CLICK_TIME 200
#define LONG_CLICK_TIME   800

#define B_STATUS(i, bit)        IS_BIT_SET8(Buttons.buttons[i].status, bit)
#define B_SET_STATUS(i, bit)    SET_BIT8(Buttons.buttons[i].status, bit)
#define B_CLEAR_STATUS(i, bit)  CLEAR_BIT8(Buttons.buttons[i].status, bit)
#define B_STORE_STATUS(i, bit, j) { if (j) B_SET_STATUS(i, bit);  \
      else B_CLEAR_STATUS(i, bit); }

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

#define B_LONG_CLICK(i)          B_STATUS(i, B_BIT_LONG_CLICK)
#define SET_B_LONG_CLICK(i)      B_SET_STATUS(i, B_BIT_LONG_CLICK)
#define CLEAR_B_LONG_CLICK(i)    B_CLEAR_STATUS(i, B_BIT_LONG_CLICK)

#define B_PRESS_TIME(i)            (buttons[(i)].press_time)

#define B_LAST_PRESS_TIME(i)       (buttons[(i)].last_press_time)

#define BUTTON_DOWN(button)           (!B_CURRENT(button))
#define BUTTON_UP(button)             (B_CURRENT(button))
#define OLD_BUTTON_DOWN(button)       (!B_OLD(button))
#define OLD_BUTTON_UP(button)         (B_OLD(button))
#define BUTTON_PRESSED(button)        (OLD_BUTTON_UP(button) && BUTTON_DOWN(button))
#define BUTTON_DOUBLE_CLICKED(button) (B_DOUBLE_CLICK(button))
#define BUTTON_LONG_CLICKED(button)   (B_LONG_CLICK(button))
#define BUTTON_CLICKED(button)        (B_CLICK(button))
#define BUTTON_RELEASED(button)       (OLD_BUTTON_DOWN(button) && BUTTON_UP(button))
#define BUTTON_PRESS_TIME(button)     (clock_diff(B_PRESS_TIME(button), slowclock))

typedef struct button_s {
  uint8_t  status;
  //uint16_t press_time;
  //uint16_t last_press_time;
} button_t;

class ButtonsClass {
 public:
  button_t buttons[GUI_NUM_BUTTONS];

  static const uint8_t ENCODER1 = 0;
  static const uint8_t ENCODER2 = 1;
  static const uint8_t ENCODER3 = 2;
  static const uint8_t ENCODER4 = 3;
  static const uint8_t BUTTON1 = 4;
  static const uint8_t BUTTON2 = 5;
  static const uint8_t BUTTON3 = 6;
  static const uint8_t BUTTON4 = 7;
  static const uint8_t FUNC_BUTTON1 = 8;
  static const uint8_t FUNC_BUTTON2 = 9;
  static const uint8_t FUNC_BUTTON3 = 10;
  static const uint8_t FUNC_BUTTON4 = 11;
  static const uint8_t FUNC_BUTTON5 = 12;
  static const uint8_t FUNC_BUTTON6 = 13;
  static const uint8_t FUNC_BUTTON7 = 14;
  static const uint8_t FUNC_BUTTON8 = 15;
  static const uint8_t FUNC_BUTTON9 = 16;
  static const uint8_t TRIG_BUTTON1 = 17;

  static const uint16_t ENCODER1_MASK = _BV(ENCODER1);
  static const uint16_t ENCODER2_MASK = _BV(ENCODER2);
  static const uint16_t ENCODER3_MASK = _BV(ENCODER3);
  static const uint16_t ENCODER4_MASK = _BV(ENCODER4);
  static const uint16_t BUTTON1_MASK = _BV(BUTTON1);
  static const uint16_t BUTTON2_MASK = _BV(BUTTON2);
  static const uint16_t BUTTON3_MASK = _BV(BUTTON3);
  static const uint16_t BUTTON4_MASK = _BV(BUTTON4);

  ButtonsClass();

  ALWAYS_INLINE() void clear();
  ALWAYS_INLINE() void poll(uint8_t sr);
#ifdef PLATFORM_TBD
  void pollTBD(const ui_data_t& ui_data);
#endif

};

class GUIHardware {
private:
    bool inGui;
    uint16_t oldsr;
public:
    ButtonsClass Buttons;  // Made public for macro access
#ifdef PLATFORM_TBD
    uint32_t last_ui_systicks;
#endif

    GUIHardware() : inGui(false), oldsr(0) {}
    void init();
    void poll();
    void clear();

    friend class GuiClass;
};
extern GUIHardware GUI_hardware;
extern SR165Class SR165;
extern EncodersClass Encoders;
extern ButtonsClass Buttons;

