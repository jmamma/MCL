#ifndef LCD_H__
#define LCD_H__

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "helpers.h"
#include "LCDParent.hh"

class LCDClass : public LCDParentClass {
 public:
  LCDClass();
  void initLCD();
 private:
  virtual void putnibble(uint8_t nibble);
  virtual void putbyte(uint8_t byte);
  virtual void putcommand(uint8_t cmd);
  virtual void putdata(uint8_t data);
  virtual void enable();
};

extern LCDClass LCD;

#endif /* LCD_H__ */
