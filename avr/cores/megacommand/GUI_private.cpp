#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "GUI_private.h"
//#include "helpers.h"
#include "Core.h"
#include "LCD.h"

SR165Class::SR165Class() {
  SR165_DDR_PORT |= _BV(SR165_SHLOAD) | _BV(SR165_CLK);
  CLEAR_BIT8(SR165_DDR_PORT, SR165_OUT);
  CLEAR_BIT8(SR165_DATA_PORT, SR165_OUT);
  SET_BIT8(SR165_DATA_PORT, SR165_CLK);
  SET_BIT8(SR165_DATA_PORT, SR165_SHLOAD);
}

/**********************************************/

EncodersClass::EncodersClass() {
  clearEncoders();
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    sr_old2s[i] = 0;
  }
  sr_old = 0;
}

/**********************************************/

ButtonsClass::ButtonsClass() {
  clear();
}

SR165Class SR165;
EncodersClass Encoders;
ButtonsClass Buttons;
