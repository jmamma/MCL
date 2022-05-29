/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1305 drivers

  Pick one up today in the adafruit shop!
  ------> https://www.adafruit.com/products/2675

These displays use I2C or SPI to communicate

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any
redistribution
*********************************************************************/

#ifdef __AVR__
#include <avr/pgmspace.h>
#include <util/delay.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#else
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1305.h"
#include "glcdfont.c"
#include <SPI.h>
#include <Wire.h>
#include <stdlib.h>
#include "DiagnosticPage.h"

#ifdef SPI_HAS_TRANSACTION
SPISettings oledspi = SPISettings(16000000, MSBFIRST, SPI_MODE0);
#else
#define ADAFRUIT_SSD1305_SPI SPI_CLOCK_DIV2
#endif

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// a 5x7 font table
extern const uint8_t PROGMEM font[];

// the memory buffer for the LCD

static uint8_t buffer[SSD1305_LCDHEIGHT * SSD1305_LCDWIDTH / 8] = {};

// the most basic function, set a single pixel
void Adafruit_SSD1305::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= width()) || (y >= height()) || (x < 0) || (y < 0))
    return;

  // check rotation, move pixel around if necessary*
  switch (2) {
  case 1:
    adagfx_swap(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:
    //  x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:
    adagfx_swap(x, y);
    y = HEIGHT - y - 1;
    break;
  }

draw_pixel:
  // x is which column
  if (color == WHITE)
    buffer[x + (y / 8) * SSD1305_LCDWIDTH] |= _BV((y % 8));
  else if (color == INVERT) {
    color =
        (buffer[x + (y / 8) * SSD1305_LCDWIDTH] & _BV((y % 8))) ? BLACK : WHITE;
    goto draw_pixel;
  } else // BLACK
    buffer[x + (y / 8) * SSD1305_LCDWIDTH] &= ~_BV((y % 8));
}

void Adafruit_SSD1305::drawFastVLine(int16_t x, int16_t y, int16_t h,
                                     uint16_t color) {
  if (x < 0) {
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (y + h > SSD1305_LCDHEIGHT) {
    h = SSD1305_LCDHEIGHT - y;
  }
  // check rotation, move pixel around if necessary*
  // MegaCommand rotation
  y = SSD1305_LCDHEIGHT - y - h;

  // initial pointer position
  uint8_t *p = buffer + x + (y / 8) * SSD1305_LCDWIDTH;

  // 1. upper part
  uint8_t h_ = 8 - (y & 0x07);

  if (h_ != 8) {
    // higher bits ON
    uint8_t mask = ~(0xFF >> h_);

    if (h_ > h) {
      mask = mask & (0xFF >> (h_ - h));
      h_ = h;
    }

    if (color == WHITE) {
      *p |= mask;
    } else if (color == BLACK) {
      *p &= ~mask;
    } else { // INVERT
      *p ^= mask;
    }
    p += SSD1305_LCDWIDTH;
    h -= h_;
  }

  // 2. center fast part
  while (h >= 8) {
    if (color == WHITE) {
      *p = 0xFF;
    } else if (color == BLACK) {
      *p = 0x00;
    } else { // INVERT
      *p = ~*p;
    }
    h -= 8;
    p += SSD1305_LCDWIDTH;
  }

  // 3. lower part
  if (h != 0) {
    uint8_t mask = ((1 << h) - 1);
    if (color == WHITE) {
      *p |= mask;
    } else if (color == BLACK) {
      *p &= ~mask;
    } else { // INVERT
      *p ^= mask;
    }
  }
}

void Adafruit_SSD1305::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                uint16_t color) {
  // for (int16_t x2 = x + w; x < x2; ++x)
  //{
  // drawFastVLine(x, y, h, color);
  //}
  // return;
  if (x < 0) {
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (y + h > SSD1305_LCDHEIGHT) {
    h = SSD1305_LCDHEIGHT - y;
  }

  if (x + w > SSD1305_LCDWIDTH) {
    w = SSD1305_LCDHEIGHT - x;
  }
  // check rotation, move pixel around if necessary*
  // MegaCommand rotation
  y = SSD1305_LCDHEIGHT - y - h;

  // initial pointer position
  uint8_t *p = buffer + x + (y / 8) * SSD1305_LCDWIDTH;
  const int16_t xend = x + w;

  // 1. upper part
  int16_t h_ = 8 - (y & 0x07);
  if (h_ != 8) {
    // higher bits ON
    uint8_t mask = ~(0xFF >> h_);

    if (h_ > h) {
      mask = mask & (0xFF >> (h_ - h));
      h_ = h;
    }

    uint8_t *px = p;
    for (uint8_t x_ = x; x_ < xend; ++x_) {
      if (color == WHITE) {
        *px |= mask;
      } else if (color == BLACK) {
        *px &= ~mask;
      } else { // INVERT
        *px ^= mask;
      }
      ++px;
    }
    p += SSD1305_LCDWIDTH;
    h -= h_;
  }

  // 2. center fast part
  while (h >= 8) {
    uint8_t *px = p;
    for (uint8_t x_ = x; x_ < xend; ++x_) {
      if (color == WHITE) {
        *px = 0xFF;
      } else if (color == BLACK) {
        *px = 0x00;
      } else { // INVERT
        *px = ~*px;
      }
      ++px;
    }
    h -= 8;
    p += SSD1305_LCDWIDTH;
  }

  // 3. lower part
  if (h != 0) {
    uint8_t mask = ((1 << h) - 1);
    uint8_t *px = p;
    for (uint8_t x_ = x; x_ < xend; ++x_) {
      if (color == WHITE) {
        *px |= mask;
      } else if (color == BLACK) {
        *px &= ~mask;
      } else { // INVERT
        *px ^= mask;
      }
      ++px;
    }
  }
}

void Adafruit_SSD1305::fillScreen(uint16_t color) {
  if (color == BLACK) {
    clearDisplay();
  } else if (color == WHITE) {
    memset(buffer, 0xFF, (SSD1305_LCDWIDTH * SSD1305_LCDHEIGHT / 8));
  } else { // INVERT
    for (uint8_t *p = buffer,
                 *e = buffer + (SSD1305_LCDWIDTH * SSD1305_LCDHEIGHT / 8);
         p != e; ++p) {
      *p = ~*p;
    }
  }
}

void Adafruit_SSD1305::begin(uint8_t i2caddr) {
  _i2caddr = i2caddr;

  // set pin directions
  if (sclk != -1) {
    pinMode(sid, OUTPUT);
    pinMode(sclk, OUTPUT);
#ifdef __AVR__
    clkport = portOutputRegister(digitalPinToPort(sclk));
    clkpinmask = digitalPinToBitMask(sclk);
    mosiport = portOutputRegister(digitalPinToPort(sid));
    mosipinmask = digitalPinToBitMask(sid);
#endif
  } else if (cs != -1) {
    // hardware SPI
    SPI.begin();
  } else {
    // I2C
    Wire.begin();
  }

  if (cs != -1) {
    pinMode(dc, OUTPUT);
    pinMode(cs, OUTPUT);
  }

  if (rst != -1) {
    pinMode(rst, OUTPUT);

    digitalWrite(rst, HIGH);
    // VDD (3.3V) goes high at start, lets just chill for a ms
    delay(1);
    // bring reset low
    digitalWrite(rst, LOW);
    // wait 10ms
    delay(10);
    // bring out of reset
    digitalWrite(rst, HIGH);
  }

#if defined SSD1305_128_32
  // Init sequence for 128x32 OLED module
  command(SSD1305_DISPLAYOFF);          // 0xAE
  command(SSD1305_SETLOWCOLUMN | 0x0);  // low col = 0
  command(SSD1305_SETHIGHCOLUMN | 0x0); // hi col = 0
  command(SSD1305_SETSTARTLINE | 0x0);  // line #0
  command(0x2E);                        //??
  command(SSD1305_SETCONTRAST);         // 0x81
  command(0x32);
  command(SSD1305_SETBRIGHTNESS); // 0x82
  command(0x80);
  command(SSD1305_SEGREMAP | 0x00);
  command(SSD1305_NORMALDISPLAY); // 0xA6
  command(SSD1305_SETMULTIPLEX);  // 0xA8
  command(0x3F);                  // 1/64
  command(SSD1305_MASTERCONFIG);
  command(0x8e); /* external vcc supply */
  command(SSD1305_COMSCANDEC);
  command(SSD1305_SETDISPLAYOFFSET); // 0xD3
  command(0x40);
  command(SSD1305_SETDISPLAYCLOCKDIV); // 0xD5
  command(0xf0);
  command(SSD1305_SETAREACOLOR);
  command(0x05);
  command(SSD1305_SETPRECHARGE); // 0xd9
  command(0xF1);
  command(SSD1305_SETCOMPINS); // 0xDA
  command(0x12);

  command(SSD1305_SETLUT);
  command(0x3F);
  command(0x3F);
  command(0x3F);
  command(0x3F);

#endif

#if defined SSD1305_128_64
  // Init sequence for 128x64 OLED module
  command(SSD1305_DISPLAYOFF);          // 0xAE
  command(SSD1305_SETLOWCOLUMN | 0x4);  // low col = 0
  command(SSD1305_SETHIGHCOLUMN | 0x4); // hi col = 0
  command(SSD1305_SETSTARTLINE | 0x0);  // line #0
  command(0x2E);                        //??
  command(SSD1305_SETCONTRAST);         // 0x81
  command(0x32);
  command(SSD1305_SETBRIGHTNESS); // 0x82
  command(0x80);
  command(SSD1305_SEGREMAP | 0x01);
  command(SSD1305_NORMALDISPLAY); // 0xA6
  command(SSD1305_SETMULTIPLEX);  // 0xA8
  command(0x3F);                  // 1/64
  command(SSD1305_MASTERCONFIG);
  command(0x8e); /* external vcc supply */
  command(SSD1305_COMSCANDEC);
  command(SSD1305_SETDISPLAYOFFSET); // 0xD3
  command(0x40);
  command(SSD1305_SETDISPLAYCLOCKDIV); // 0xD5
  command(0xf0);
  command(SSD1305_SETAREACOLOR);
  command(0x05);
  command(SSD1305_SETPRECHARGE); // 0xd9
  command(0xF1);
  command(SSD1305_SETCOMPINS); // 0xDA
  command(0x12);

  command(SSD1305_SETLUT);
  command(0x3F);
  command(0x3F);
  command(0x3F);
  command(0x3F);
#endif

  command(SSD1305_DISPLAYON); //--turn on oled panel
}

void Adafruit_SSD1305::invertDisplay(uint8_t i) {
  if (i) {
    command(SSD1305_INVERTDISPLAY);
  } else {
    command(SSD1305_NORMALDISPLAY);
  }
}

void Adafruit_SSD1305::command(uint8_t c) {

  if (cs != -1) {
    // SPI of sorts

    digitalWrite(cs, HIGH);
    digitalWrite(dc, LOW);
    delay(1);
    if (sclk == -1) {
#ifdef SPI_HAS_TRANSACTION
      SPI.beginTransaction(oledspi);
#else
      SPI.setDataMode(SPI_MODE0);
      SPI.setClockDivider(ADAFRUIT_SSD1305_SPI);
#endif
    }
    digitalWrite(cs, LOW);
    spixfer(c);
    digitalWrite(cs, HIGH);

#ifdef SPI_HAS_TRANSACTION
    if (sclk == -1)
      SPI.endTransaction(); // release the SPI bus
#endif
  } else {
    // I2C
    uint8_t control = 0x00; // Co = 0, D/C = 0
    Wire.beginTransmission(_i2caddr);
    Wire.write(control);
    Wire.write(c);
    Wire.endTransmission();
  }
}

uint8_t Adafruit_SSD1305::getBuffer(uint16_t i) { return buffer[i]; }
uint8_t* Adafruit_SSD1305::getBuffer() { return buffer; }

void Adafruit_SSD1305::data(uint8_t c) {
  if (cs != -1) {
    // SPI of sorts
    digitalWrite(cs, HIGH);
    digitalWrite(dc, HIGH);

    if (sclk == -1) {
#ifdef SPI_HAS_TRANSACTION
      SPI.beginTransaction(oledspi);
#else
      SPI.setDataMode(SPI_MODE0);
      SPI.setClockDivider(ADAFRUIT_SSD1305_SPI);
#endif
    }

    digitalWrite(cs, LOW);
    spixfer(c);
    digitalWrite(cs, HIGH);

#ifdef SPI_HAS_TRANSACTION
    if (sclk == -1)
      SPI.endTransaction(); // release the SPI bus
#endif
  } else {
    // I2C
    uint8_t control = 0x40; // Co = 0, D/C = 1
    Wire.beginTransmission(_i2caddr);
    Wire.write(control);
    Wire.write(c);
    Wire.endTransmission();
  }
}

void Adafruit_SSD1305::textbox(const char *text, const char *text2, uint16_t delay) {
  textbox_clock = slowclock;
  strncpy(textbox_str, text, sizeof(textbox_str));
  strncpy(textbox_str2, text2, sizeof(textbox_str2));
  textbox_delay = delay;
  textbox_enabled = true;
}

bool display_lock = false;

void Adafruit_SSD1305::display(void) {
  if (display_lock) { return; }

  if (screen_saver && screen_saver_active) {
    return;
  }
  display_lock = true;
  if (textbox_enabled) {
    if (clock_diff(textbox_clock, slowclock) < textbox_delay) {
      draw_textbox(textbox_str, textbox_str2);
    } else {
      textbox_enabled = false;
    }
  }

#ifdef ENABLE_DIAG_LOGGING
  if (diag_page.is_active()) {
    diag_page.draw();
  }
#endif

  if(screen_saver) {
    clearDisplay();
  }

  screen_saver_active = screen_saver;

  uint16_t i = 0;
  uint8_t page;
  if (SSD1305_LCDHEIGHT == 64)
    page = 0;
  if (SSD1305_LCDHEIGHT == 32)
    page = 4;

  for (; page < 8; page++) {
    command(SSD1305_SETPAGESTART + page);
    command(0x00);
    command(0x10);

    if (cs == -1) {
      // save I2C bitrate
#ifdef __AVR__
      uint8_t twbrbackup = TWBR;
      TWBR = 12; // upgrade to 400KHz!
#endif

      // Serial.println(TWBR, DEC);
      // Serial.println(TWSR & 0x3, DEC);

      // I2C has max 16 bytes per xmision
      // send a bunch of data in one xmission
      for (uint8_t w = 0; w < 128 / 16; w++) {
        Wire.beginTransmission(_i2caddr);
        Wire.write(0x40);
        for (uint8_t x = 0; x < 16; x++) {
          Wire.write(buffer[i++]);
        }
        Wire.endTransmission();
      }

#ifdef __AVR__
      TWBR = twbrbackup;
#endif
    } else {
      // SPI
      if (sclk == -1) {
#ifdef SPI_HAS_TRANSACTION
        SPI.beginTransaction(oledspi);
#else
        SPI.setDataMode(SPI_MODE0);
        SPI.setClockDivider(ADAFRUIT_SSD1305_SPI);
#endif
      }

      digitalWrite(cs, HIGH);
      digitalWrite(dc, HIGH);
      digitalWrite(cs, LOW);

      for (uint8_t x = 0; x < 128; x++) {
        spixfer(buffer[i++]);
      }

      digitalWrite(cs, HIGH);
#ifdef SPI_HAS_TRANSACTION
      if (sclk == -1)
        SPI.endTransaction(); // release the SPI bus
#endif
    }
  }
  display_lock = false;
}

// clear everything
void Adafruit_SSD1305::clearDisplay(void) {
  memset(buffer, 0, (SSD1305_LCDWIDTH * SSD1305_LCDHEIGHT / 8));
}

void Adafruit_SSD1305::spixfer(uint8_t x) {
  if (sclk == -1) {
    SPI.transfer(x);
    return;
  }
  // software spi
  // Serial.println("Software SPI");

  for (uint8_t bit = 0x80; bit; bit >>= 1) {
#if defined(AVR)
    *clkport &= ~clkpinmask;
    if (x & bit)
      *mosiport |= mosipinmask;
    else
      *mosiport &= ~mosipinmask;
    *clkport |= clkpinmask;
#else
    digitalWrite(sclk, LOW);
    if (x & bit)
      digitalWrite(sid, HIGH);
    else
      digitalWrite(sid, LOW);
    digitalWrite(sclk, HIGH);
#endif
  }
}
