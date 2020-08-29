/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef LCDPARENT_H__
#define LCDPARENT_H__

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

#ifndef HOST_MIDIDUINO
extern "C" {
#include <avr/io.h>
#include <avr/interrupt.h>
}
#include <avr/pgmspace.h>
#include <inttypes.h>
#else
#include "WProgram.h"
#endif

/**
 * This is an abstracted version of a LCD class for a HD44780 display,
 * just leaving the implementation of sending bytes to the controller
 * up to the child class.
 **/
class LCDParentClass {
	/**
	 * \addtogroup lcd_parent
	 * @{
	 **/
	
protected:
	/** Toggle the enable line (left to child). **/
  virtual void enable() { }
	/** Send a nibble to the display (left to child). **/
  virtual void putnibble(uint8_t nibble) { }
	/** Send a command byte to the display (left to child). **/
  virtual void putcommand(uint8_t command) { }
	/** Send a data byte to the display (left to child). **/
  virtual void putdata(uint8_t data) { }

  public:
#ifdef HOST_MIDIDUINO
  virtual ~LCDParentClass() { }
#endif

	/** Go to the first line. **/
  void line1();
	/** Go to the first line and display the given string. **/
  void line1(char *s);
	/** Go to the first line and display the given string, filling up with whitespace. **/
  void line1_fill(char *s);
	/** Go to the first line and display the given program-space string. **/
  void line1_p(PGM_P s);
	/** Go to the first line and display the given program-space string, filling up with whitespace. **/
  void line1_p_fill(PGM_P s);
	/** Go to the second line. **/
  void line2();
	/** Go to the second line and display the given string. **/
  void line2(char *s);
	/** Go to the second line and display the given string, filling up with whitespace. **/
  void line2_fill(char *s);
	/** Go to the second line and display the given program-space string. **/
  void line2_p(PGM_P s);
	/** Go to the second line and display the given program-space string, filling up with whitespace. **/
  void line2_p_fill(PGM_P s);

	/** Go to the given line index (works for line 0 and line 1). **/
  inline void goLine(uint8_t line) {
    switch (line) {
    case 0:
      line1();
      break;
    case 1:
      line2();
      break;
    }
  }

	/** Clear the current line with whitespace. **/
  void clearLine();

	/** Put the given string at the current position on the display. **/
  void puts(char *s);
	/** Put the given string at the current position on the display and fill up to i with whitespace. **/
  void puts_fill(char *s, uint8_t i);
	/** Put the given string at the current position on the display and fill up to the line length with whitespace. **/
  void puts_fill(char *s);
	/** Put the given program-space string at the current position on the display. **/
  void puts_p(PGM_P s);
	/**
	 * Put the given program-space string at the current position on the
	 * display and fill up to i with whitespace.
	 **/
  void puts_p_fill(PGM_P s, uint8_t i);
	/**
	 * Put the given program-space string at the current position on the
	 * display and fill up to the line length with whitespace.
	 **/
  void puts_p_fill(PGM_P s);

	/** Put the given data to the display. **/
  void put(char *data, uint8_t cnt);

	/** Put the number in base 10 to the display. **/
  void putnumber(uint8_t num);
	/** Put the number in base 10 to the display. **/
  void putnumber16(uint16_t num);
	/** Put the given digit in base 16 to the display. **/
  void putcx(uint8_t i);
	/** Put the given number in base 16 to the display. **/
  void putnumberx(uint8_t num);
	/** Put the given number in base 16 to the display. **/
  void putnumberx16(uint16_t num);
	/** Put the given number in base 10 to the display. **/
  void putnumber32(uint32_t num);

	/** switch to blinking cursor if blink is true, or to normal cursor if blink is false. **/
  void blinkCursor(bool blink);
	/** move the cursor to the given position. **/
  void moveCursor(uint8_t row, uint8_t column);
    
   void createChar(uint8_t location, uint8_t charmap[]);

	/* @} */
};

#endif /* LCDPARENT_H__ */
