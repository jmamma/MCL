#ifndef GUI_PRIVATE_H__
#define GUI_PRIVATE_H__

#include "Core.h"
#include "helpers.h"
#include <avr/pgmspace.h>
#include <inttypes.h>

#define ENCODER_NORMAL(i) (encoders[(i)].normal)
#define ENCODER_SHIFT(i) (encoders[(i)].shift)
#define ENCODER_BUTTON(i) (encoders[(i)].button)
#define ENCODER_BUTTON_SHIFT(i) (encoders[(i)].button_shift)

#define B_BIT_CURRENT 0
#define B_BIT_OLD 1
#define B_BIT_PRESSED_ONCE 2
#define B_BIT_DOUBLE_CLICK 3
#define B_BIT_CLICK 4
#define B_BIT_LONG_CLICK 5

// XXX adjust these to correct length of irq time
#define DOUBLE_CLICK_TIME 200
#define LONG_CLICK_TIME 800

#define B_STATUS(i, bit) IS_BIT_SET8(Buttons.buttons[i].status, bit)
#define __B_STATUS(i, bit) IS_BIT_SET8(buttons[i].status, bit)
#define B_SET_STATUS(i, bit) SET_BIT8(Buttons.buttons[i].status, bit)
#define __B_SET_STATUS(i, bit) SET_BIT8(buttons[i].status, bit)
#define B_CLEAR_STATUS(i, bit) CLEAR_BIT8(Buttons.buttons[i].status, bit)
#define __B_CLEAR_STATUS(i, bit) CLEAR_BIT8(buttons[i].status, bit)
#define B_STORE_STATUS(i, bit, j)                                              \
  {                                                                            \
    if (j)                                                                     \
      B_SET_STATUS(i, bit);                                                    \
    else                                                                       \
      B_CLEAR_STATUS(i, bit);                                                  \
  }
#define __B_STORE_STATUS(i, bit, j)                                            \
  {                                                                            \
    if (j)                                                                     \
      __B_SET_STATUS(i, bit);                                                  \
    else                                                                       \
      __B_CLEAR_STATUS(i, bit);                                                \
  }

#define B_CURRENT(i) B_STATUS(i, B_BIT_CURRENT)
#define __B_CURRENT(i) __B_STATUS(i, B_BIT_CURRENT)
#define SET_B_CURRENT(i) B_SET_STATUS(i, B_BIT_CURRENT)
#define CLEAR_B_CURRENT(i) B_CLEAR_STATUS(i, B_BIT_CURRENT)
#define STORE_B_CURRENT(i, j) B_STORE_STATUS(i, B_BIT_CURRENT, j)
#define __STORE_B_CURRENT(i, j) __B_STORE_STATUS(i, B_BIT_CURRENT, j)

#define B_OLD(i) B_STATUS(i, B_BIT_OLD)
#define SET_B_OLD(i) B_SET_STATUS(i, B_BIT_OLD)
#define CLEAR_B_OLD(i) B_CLEAR_STATUS(i, B_BIT_OLD)
#define STORE_B_OLD(i, j) B_STORE_STATUS(i, B_BIT_OLD, j)
#define __STORE_B_OLD(i, j) __B_STORE_STATUS(i, B_BIT_OLD, j)

#define B_PRESSED_ONCE(i) B_STATUS(i, B_BIT_PRESSED_ONCE)
#define SET_B_PRESSED_ONCE(i) B_SET_STATUS(i, B_BIT_PRESSED_ONCE)
#define CLEAR_B_PRESSED_ONCE(i) B_CLEAR_STATUS(i, B_BIT_PRESSED_ONCE)

#define B_CLICK(i) B_STATUS(i, B_BIT_CLICK)
#define SET_B_CLICK(i) B_SET_STATUS(i, B_BIT_CLICK)
#define CLEAR_B_CLICK(i) B_CLEAR_STATUS(i, B_BIT_CLICK)
#define __CLEAR_B_CLICK(i) __B_CLEAR_STATUS(i, B_BIT_CLICK)

#define B_DOUBLE_CLICK(i) B_STATUS(i, B_BIT_DOUBLE_CLICK)
#define SET_B_DOUBLE_CLICK(i) B_SET_STATUS(i, B_BIT_DOUBLE_CLICK)
#define CLEAR_B_DOUBLE_CLICK(i) B_CLEAR_STATUS(i, B_BIT_DOUBLE_CLICK)
#define __CLEAR_B_DOUBLE_CLICK(i) __B_CLEAR_STATUS(i, B_BIT_DOUBLE_CLICK)

#define B_LONG_CLICK(i) B_STATUS(i, B_BIT_LONG_CLICK)
#define SET_B_LONG_CLICK(i) B_SET_STATUS(i, B_BIT_LONG_CLICK)
#define CLEAR_B_LONG_CLICK(i) B_CLEAR_STATUS(i, B_BIT_LONG_CLICK)
#define __CLEAR_B_LONG_CLICK(i) __B_CLEAR_STATUS(i, B_BIT_LONG_CLICK)

#define B_PRESS_TIME(i) (buttons[(i)].press_time)

#define B_LAST_PRESS_TIME(i) (buttons[(i)].last_press_time)

#define BUTTON_DOWN(button) (!B_CURRENT(button))
#define __BUTTON_DOWN(button) (!__B_CURRENT(button))
#define BUTTON_UP(button) (B_CURRENT(button))
#define OLD_BUTTON_DOWN(button) (!B_OLD(button))
#define OLD_BUTTON_UP(button) (B_OLD(button))
#define BUTTON_PRESSED(button) (OLD_BUTTON_UP(button) && BUTTON_DOWN(button))
#define BUTTON_DOUBLE_CLICKED(button) (B_DOUBLE_CLICK(button))
#define BUTTON_LONG_CLICKED(button) (B_LONG_CLICK(button))
#define BUTTON_CLICKED(button) (B_CLICK(button))
#define BUTTON_RELEASED(button) (OLD_BUTTON_DOWN(button) && BUTTON_UP(button))
#define BUTTON_PRESS_TIME(button) (clock_diff(B_PRESS_TIME(button), slowclock))

#ifdef MEGACOMMAND
#define SR165_OUT PL0
#define SR165_SHLOAD PL1
#define SR165_CLK PL2

#define SR165_DATA_PORT PORTL
#define SR165_DDR_PORT DDRL
#define SR165_PIN_PORT PINL
#else
#define SR165_OUT PD5
#define SR165_SHLOAD PD6
#define SR165_CLK PD7

#define SR165_DATA_PORT PORTD
#define SR165_DDR_PORT DDRD
#define SR165_PIN_PORT PIND
#endif

#define SR165_DELAY()                                                          \
  {} // asm("nop"); } // asm("nop");  asm("nop");  }

class SR165Class {
  ALWAYS_INLINE() void clk() {
    CLEAR_BIT8(SR165_DATA_PORT, SR165_CLK);
    SET_BIT8(SR165_DATA_PORT, SR165_CLK);
  }

  ALWAYS_INLINE() void rst() {
    CLEAR_BIT8(SR165_DATA_PORT, SR165_SHLOAD);
    SET_BIT8(SR165_DATA_PORT, SR165_SHLOAD);
  }

public:
  SR165Class();
  ALWAYS_INLINE() uint8_t read() {
    rst();

    uint8_t res = 0;
    uint8_t i = 0;
    for (i = 0; i < 8; i++) {
      res <<= 1;
      res |= IS_BIT_SET8(SR165_PIN_PORT, SR165_OUT);
      clk();
    }

    return res;
  }

  ALWAYS_INLINE() uint8_t read_norst() {
    uint8_t res = 0;
    uint8_t i = 0;
    for (i = 0; i < 8; i++) {
      res <<= 1;
      res |= IS_BIT_SET8(SR165_PIN_PORT, SR165_OUT);
      clk();
    }

    return res;
  }

  ALWAYS_INLINE() uint16_t read16() {
    rst();

    uint16_t res = 0;
    uint8_t i = 0;
    for (i = 0; i < 16; i++) {
      res <<= 1;
      res |= IS_BIT_SET8(SR165_PIN_PORT, SR165_OUT);
      clk();
    }

    return res;
  }
};

#define GUI_NUM_ENCODERS 4
#define GUI_NUM_BUTTONS 8

typedef struct button_s {
  uint8_t status;
  uint16_t press_time;
  uint16_t last_press_time;
} button_t;

class ButtonsClass {
public:
  button_t buttons[GUI_NUM_BUTTONS];

  static const uint8_t BUTTON1 = 4;
  static const uint8_t BUTTON2 = 5;
  static const uint8_t BUTTON3 = 6;
  static const uint8_t BUTTON4 = 7;
  static const uint8_t ENCODER1 = 0;
  static const uint8_t ENCODER2 = 1;
  static const uint8_t ENCODER3 = 2;
  static const uint8_t ENCODER4 = 3;

  static const uint16_t ENCODER1_MASK = _BV(ENCODER1);
  static const uint16_t ENCODER2_MASK = _BV(ENCODER2);
  static const uint16_t ENCODER3_MASK = _BV(ENCODER3);
  static const uint16_t ENCODER4_MASK = _BV(ENCODER4);
  static const uint16_t BUTTON1_MASK = _BV(BUTTON1);
  static const uint16_t BUTTON2_MASK = _BV(BUTTON2);
  static const uint16_t BUTTON3_MASK = _BV(BUTTON3);
  static const uint16_t BUTTON4_MASK = _BV(BUTTON4);

  ButtonsClass();
  ALWAYS_INLINE() void clear() {
    for (int i = 0; i < GUI_NUM_BUTTONS; i++) {
      __CLEAR_B_DOUBLE_CLICK(i);
      __CLEAR_B_CLICK(i);
      __CLEAR_B_LONG_CLICK(i);
      __STORE_B_OLD(i, __B_CURRENT(i));
    }
  }

  ALWAYS_INLINE() void poll(uint8_t but) {
    uint8_t but_tmp = but;

    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; i++) {
      __STORE_B_CURRENT(i, IS_BIT_SET8(but_tmp, 0));

      // disable button stuff for now
      /*
      uint16_t sclock = read_slowclock();
      if (BUTTON_PRESSED(i)) {
        B_PRESS_TIME(i) =  sclock;

        if (B_PRESSED_ONCE(i)) {
          uint16_t diff = clock_diff(B_LAST_PRESS_TIME(i), B_PRESS_TIME(i));
          if (diff < DOUBLE_CLICK_TIME) {
            SET_B_DOUBLE_CLICK(i);
            CLEAR_B_PRESSED_ONCE(i);
          }
        } else {
          B_LAST_PRESS_TIME(i) = B_PRESS_TIME(i);
          SET_B_PRESSED_ONCE(i);
        }
      }

      if (BUTTON_DOWN(i) && B_PRESSED_ONCE(i)) {
        uint16_t diff = clock_diff(B_LAST_PRESS_TIME(i), sclock);
        if (diff > LONG_CLICK_TIME) {
          SET_B_LONG_CLICK(i);
          CLEAR_B_PRESSED_ONCE(i);
        }
      }

      if (BUTTON_UP(i) && B_PRESSED_ONCE(i)) {
        uint16_t diff = clock_diff(B_LAST_PRESS_TIME(i), sclock);
        if (diff > LONG_CLICK_TIME) {
          CLEAR_B_PRESSED_ONCE(i);
        } else if (diff > DOUBLE_CLICK_TIME) {
          CLEAR_B_PRESSED_ONCE(i);
          SET_B_CLICK(i);
        }
      }
      */

      but_tmp >>= 1;
    }
  }
};
extern ButtonsClass Buttons;

typedef struct encoder_s {
  int8_t normal;
  int8_t button;
} encoder_t;

class EncodersClass {
  uint16_t sr_old;
  uint8_t sr_old2s[GUI_NUM_ENCODERS];

public:
  encoder_t encoders[GUI_NUM_ENCODERS];

  EncodersClass();

  ALWAYS_INLINE() void poll(uint16_t sr) {
    uint16_t sr_tmp = sr;
    /*
    if (sr != sr_old) {
      LCD.line1();
      LCD.putnumber(sr & 0x3);
      LCD.puts(" ");
      LCD.putnumber(sr_old & 0x3);
    }
    */

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
        } else if (((sr_old2s[i] & 3) == 0 && (sr_old & 3) == 2 &&
                    (sr & 3) == 3) ||
                   ((sr_old2s[i] & 3) == 3 && (sr_old & 3) == 1 &&
                    (sr & 3) == 0)) {
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

  ALWAYS_INLINE() void clearEncoders() {
    // USE_LOCK();
    // SET_LOCK();
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
      ENCODER_NORMAL(i) = ENCODER_BUTTON(i) = 0;
    }
    //  CLEAR_LOCK();
  }

  ALWAYS_INLINE() int8_t getNormal(uint8_t i) { return encoders[i].normal; }
  ALWAYS_INLINE() int8_t getButton(uint8_t i) { return encoders[i].button; }

  ALWAYS_INLINE() int8_t limitValue(int8_t value, int8_t min, int8_t max) {
    if (value > max)
      return max;
    if (value < min)
      return min;
    return value;
  }
};

extern SR165Class SR165;
extern EncodersClass Encoders;

#endif /* GUI_PRIVATE_H__ */
