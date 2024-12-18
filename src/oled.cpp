#include "Oled.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1305.h"
#include "DiagnosticPage.h"
#include "MCLSd.h"

// the most basic function, set a single pixel
void Oled::drawPixel(uint8_t 16, uint8_t 16, uint16_t color) {
  if (x >= width() || y >= height())
    return;
  uint16_t index = x + (y / 8) * SSD1305_LCDWIDTH;
  uint8_t bit = _BV(y & 7);

  if (color == WHITE)
    buffer[index] |= bit;
  else if (color == BLACK)
    buffer[index] &= ~bit;
  else // INVERT
    buffer[index] ^= bit;
}

void Oled::drawFastVLine(uint16_t x, uint16_t y, uint16_t h,
                                     uint16_t color) {
  if ((x >= width()) || (y >= height()))
   return;

  if (y + h > SSD1305_LCDHEIGHT) {
    h = SSD1305_LCDHEIGHT - y;
  }
 // initial pointer position
  uint8_t *p = buffer + x + (y / 8) * SSD1305_LCDWIDTH;

  // 1. upper part
  uint16_t h_ = 8 - (y & 0x07);

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

void Oled::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                uint16_t color) {
  if ((x >= width()) || (y >= height()))
   return;
        // for (int16_t x2 = x + w; x < x2; ++x)
  //{
  // drawFastVLine(x, y, h, color);
  //}
  // return;
  if (y + h > SSD1305_LCDHEIGHT) {
    h = SSD1305_LCDHEIGHT - y;
  }

  if (x + w > SSD1305_LCDWIDTH) {
    w = SSD1305_LCDWIDTH - x;
  }
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
    for (uint16_t x_ = x; x_ < xend; ++x_) {
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
    for (uint16_t x_ = x; x_ < xend; ++x_) {
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
    for (uint16_t x_ = x; x_ < xend; ++x_) {
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

// MCL-specific 3px triangle fill
void Oled::fillTriangle_3px(int16_t x0, int16_t y0, uint16_t color) {
  drawFastVLine(x0, y0, 5, color);
  drawFastVLine(x0+1, y0+1, 3, color);
  drawPixel(x0+2,y0+2, color);
}

oid Oled::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, uint16_t color, bool flip_vert, bool flip_horiz) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
      if (byte & 0x80) {
      uint8_t x_r, y_r;

      if (flip_vert) {
        x_r = x + w - i - 1;
      }

      else {
        x_r = x + i;
      }

      if (flip_horiz) {
        y_r = y + h - j - 1;
      }

      else {
        y_r = y + j;
      }

        writePixel(x_r, y_r, color);
      }
    }
  }
  endWrite();
}

// Draw a PROGMEM-resident 1-bit image at the specified (x,y) position,
// using the specified foreground (for set bits) and background (unset
// bits) colors.
void Oled::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, uint16_t color, uint16_t bg,
                              bool flip_vert, bool flip_horiz) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
      uint8_t x_r, y_r;

      if (flip_vert) {
        x_r = x + w - i - 1;
      }

      else {
        x_r = x + i;
      }

      if (flip_horiz) {
        y_r = y + h - j - 1;
      }

      else {
        y_r = y + j;
      }

      writePixel(x_r, y_r, (byte & 0x80) ? color : bg);
    }
  }
  endWrite();
}

// Draw a RAM-resident 1-bit image at the specified (x,y) position,
// using the specified foreground color (unset bits are transparent).
void Oled::drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w,
                              int16_t h, uint16_t color, bool flip_vert, bool flip_horiz) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      uint8_t x_r, y_r;
      if (flip_vert) {
        x_r = x + w - i - 1;
      }

      else {
        x_r = x + i;
      }

      if (flip_horiz) {
        y_r = y + h - j - 1;
      }

      else {
        y_r = y + j;
      }


      if (byte & 0x80)
        writePixel(x_r, y_r, color);
    }
  }
  endWrite();
}

// Draw a RAM-resident 1-bit image at the specified (x,y) position,
// using the specified foreground (for set bits) and background (unset
// bits) colors.
void Oled::drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w,
                              int16_t h, uint16_t color, uint16_t bg, bool flip_vert, bool flip_horiz) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  startWrite();
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      uint8_t x_r, y_r;
      if (flip_vert) {
        x_r = x + w - i - 1;
      }

      else {
        x_r = x + i;
      }

      if (flip_horiz) {
        y_r = y + h - j - 1;
      }

      else {
        y_r = y + j;
      }

      writePixel(x_r, y_r, (byte & 0x80) ? color : bg);
    }
  }
  endWrite();
}


void Oled::fillScreen(uint16_t color) {
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

void Oled::textbox(const char *text, const char *text2, uint16_t delay) {
  textbox_clock = slowclock;
  strncpy(textbox_str, text, sizeof(textbox_str));
  strncpy(textbox_str2, text2, sizeof(textbox_str2));
  textbox_delay = delay;
  textbox_enabled = true;
}

bool display_lock = false;

void Oled::display(void) {
  if (display_lock) { return; }

  display_lock = true;
  if (textbox_enabled) {
    if (clock_diff(textbox_clock, slowclock) < textbox_delay) {
      draw_textbox(textbox_str, textbox_str2);
    } else {
      textbox_enabled = false;
    }
  }
/*
   if (diag_page.is_active()) {
    diag_page.draw();
  }
*/
 //For dedicated SPI we do this.
  SD.m_card->setDedicatedSpi(false);
  Oled::display();
  SD.m_card->setDedicatedSpi(true);
  display_lock = false;
}

// clear everything
void Oled::clearDisplay(void) {
  memset(buffer, 0, (SSD1305_LCDWIDTH * SSD1305_LCDHEIGHT / 8));
}



