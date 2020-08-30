/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "LCDParent.h"
#include <string.h>
#include "OLED.h"
//#include <Wire.h>
//#include <SPI.h>
/**
 * \addtogroup LCD
 *
 * @{
 *
 * \addtogroup lcd_parent LCD Parent Class
 *
 * @{
 *
 * \file
 * LCD Parent Class
 **/
#define LINE_LENGTH 16

void LCDParentClass::puts_p(PGM_P s) {
  uint8_t len = 0;
  char c;
  while (((c = pgm_read_byte(s)) != 0) && (len++ < LINE_LENGTH)){
#ifndef OLED_DISPLAY
    putdata(c);
#else
    oled_display.write(c);
#endif
    s++;
  }  
}

void LCDParentClass::puts(const char *s) {
  uint8_t len = 0;
  while (*s != 0 && (len++ < LINE_LENGTH)) {
#ifndef OLED_DISPLAY  
  putdata(*s);
#else
  oled_display.write(*s);
#endif  
  s++;
  }
}

void LCDParentClass::line1() {
#ifdef OLED_DISPLAY  
  oled_display.setCursor(0,0);
#else
  putcommand(0x80);
#endif
}

void LCDParentClass::line1(const char *s) {
  line1();
  puts(s);
}

void LCDParentClass::line1_p(PGM_P s) {
  line1();
  puts_p(s);
}

void LCDParentClass::line1_fill(const char *s) {
  line1();
  puts_fill(s);
}

void LCDParentClass::line1_p_fill(PGM_P s) {
  line1();
  puts_p_fill(s);
}

void LCDParentClass::line2() {
#ifdef OLED_DISPLAY
  oled_display.setCursor(0,15);
#else
  putcommand(0xc0);
#endif
}

void LCDParentClass::line2(const char *s) {
  line2();
  puts(s);
}

void LCDParentClass::line2_p(PGM_P s) {
  line2();
  puts_p(s);
}

void LCDParentClass::line2_fill(const char *s) {
  line2();
  puts_fill(s);
}

void LCDParentClass::line2_p_fill(PGM_P s) {
  line2();
  puts_p_fill(s);
}

void LCDParentClass::clearLine() {
  uint8_t i;
  for (i = 0; i < 16; i++) {
#ifndef OLED_DISPLAY    
    putdata(' ');
#else
    oled_display.write(' ');
#endif
  }
}

void LCDParentClass::puts_fill(const char *s, uint8_t i) {
  while (*s != 0) {
#ifndef OLED_DISPLAY
    putdata(*s);
#else
    oled_display.write(*s);
#endif
    s++;
    i--;
  }
  while (i--) {
#ifndef OLED_DISPLAY
    putdata(' ');
#else
    oled_display.write(' ');
#endif
  }
}

void LCDParentClass::puts_fill(const char *s) {
  puts_fill(s, LINE_LENGTH);
}

void LCDParentClass::puts_p_fill(PGM_P s, uint8_t i) {
  char c;
  while ((c = pgm_read_byte(s)) != 0) {
#ifndef OLED_DISPLAY  
   putdata(c);
#else
   oled_display.write(c);
#endif 
    s++;
    i--;
  }
  while (i--) {
#ifdef OLED_DISPLAY
    oled_display.write(c);
#else
    putdata(' ');
#endif
  }
}

void LCDParentClass::puts_p_fill(PGM_P s) {
  puts_p_fill(s, LINE_LENGTH);
}

void LCDParentClass::put(const char *data, uint8_t cnt) {
  while (cnt--)
    putdata(*data++);
}

void LCDParentClass::putnumber(uint8_t num) {
  putdata(num / 100 + '0');
  putdata((num % 100) / 10 + '0');
  putdata((num % 10) + '0');
}

void LCDParentClass::putnumber16(uint16_t num) {
  putdata(num / 10000 + '0');
  putdata((num % 10000) / 1000 + '0');
  putdata((num % 1000) / 100 + '0');
  putdata((num % 100) / 10 + '0');
  putdata((num % 10) + '0');
}

void LCDParentClass::putcx(uint8_t i) {
  if (i < 10) {
    putdata(i + '0');
  } else {
    putdata(i - 10 + 'a');
  }
}

void LCDParentClass::putnumberx(uint8_t num) {
  putcx((num >> 4) & 0xF);
  putcx(num & 0xF);
}

void LCDParentClass::putnumberx16(uint16_t num) {
  putcx((num >> 12) & 0xF);
  putcx((num >> 8) & 0xF);
  putcx((num >> 4) & 0xF);
  putcx(num & 0xF);
}

void LCDParentClass::putnumber32(uint32_t num) {
  uint8_t res[10];
  uint8_t i;
  for (i = 0; i < 10; i++) {
    res[i] = num % 10;
    num /= 10;
  }
  for (i = 0; i < 10; i++) {
    putdata(res[9-i] + '0');
  }
}

void LCDParentClass::blinkCursor(bool blink) {
  if (blink) {
    putcommand(0xF);
  } else {
    putcommand(0xC);
  }
}

void LCDParentClass::moveCursor(uint8_t row, uint8_t column) {
  putcommand(0x80 | (row * 0x40 + column));
}

void LCDParentClass::createChar(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    putcommand(0x40 | (location << 3));
    for (int i=0; i<8; i++) {
        putdata(charmap[i]);
    }
}

