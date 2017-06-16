#include "WProgram.h"
#include "LFO.h"

LFO::LFO() {
  type = LFO_NONE;
  acc = 0;
  inc = 0;
  oldacc = 0;
  oldValue = 0;
  curValue = 0;
  amount = 0;
}

uint8_t LFO::getValue() {
  uint8_t acc8 = acc >> 8;
  switch (type) {
  case LFO_TRIANGLE:
    if (acc8 > 127)
      curValue = 256 - acc8;
    else
      curValue = acc8;
    break;

  case LFO_SQUARE:
    if (acc8 > 127)
      curValue = 127;
    else
      curValue = 0;
    break;

  case LFO_RANDOM:
    if (acc8 != oldacc) {
      curValue = random(127);
    }
    break;

  default:
    curValue = 0;
  }

  oldValue = curValue;
  
  return curValue;
}
