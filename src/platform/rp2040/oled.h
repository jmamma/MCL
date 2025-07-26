#pragma once

#include <Adafruit_GFX.h>

#define OLED_DISPLAY

#if defined(PLATFORM_TBD)

#include <DaDa_SSD1309.h>
#define DISPLAY_TYPE DaDa_SSD1309
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_SPEED 10000000UL

#define OLED_MOSI 15
#define OLED_SCLK 14

#define OLED_DC 12
#define OLED_CS 13
#define OLED_RST 16

#else

#include <Adafruit_SSD1305.h>
#define DISPLAY_TYPE Adafruit_SSD1305
#define SSD1305_128_32
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_SPEED 7000000UL

// Dedicated GPIO

#define OLED_DC 14
#define OLED_CS 15
#define OLED_RST 13

// Use shared SPI1 pins
#define OLED_SCLK SPI1_SCK_PIN  // 10
#define OLED_MOSI SPI1_MOSI_PIN // 11

#endif

#define BLACK 0
#define WHITE 1
#define INVERT 2


class Oled : public DISPLAY_TYPE {
public:
  /*
  // Constructor, same as the base class constructor
  Oled(uint16_t w, uint16_t h, TwoWire *twi = &Wire,
                             int8_t rst_pin = -1, uint32_t preclk = 400000,
                             uint32_t postclk = 100000)
      : DISPLAY_TYPE(w, h, twi, rst_pin, preclk, postclk) {}

  // Constructor for SPI-based initialization
  Oled(uint16_t w, uint16_t h, int8_t mosi_pin, int8_t sclk_pin,
                             int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
      : DISPLAY_TYPE(w, h, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin) {}
  */
  // Constructor for SPI initialization with custom bitrate
  Oled(uint16_t w, uint16_t h, SPIClass *spi, int8_t dc_pin, int8_t rst_pin,
       int8_t cs_pin, uint32_t bitrate = 8000000UL)
      : DISPLAY_TYPE(w, h, spi, dc_pin, rst_pin, cs_pin, bitrate) {}
  uint16_t textbox_delay;
  uint16_t textbox_clock;
  char textbox_str[17];
  char textbox_str2[17];
  bool textbox_enabled = false;

  virtual void textbox(const char *text, const char *text2,
                       uint16_t delay = 800);

  void init_display();
  void display();

  void fillTriangle_3px(int16_t x0, int16_t y0, uint16_t color);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                  int16_t h, uint16_t color, bool flip_vert = false,
                  bool flip_horiz = false);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                  int16_t h, uint16_t color, uint16_t bg,
                  bool flip_vert = false, bool flip_horiz = false);
  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h,
                  uint16_t color, bool flip_vert = false,
                  bool flip_horiz = false);
  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h,
                  uint16_t color, uint16_t bg, bool flip_vert = false,
                  bool flip_horiz = false);

  void draw_textbox(char *text, char *text2);
  void draw_textbox(const char *text1, const char *text2);
  /*
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t
  color); virtual void fillScreen(uint16_t color); virtual void clearDisplay();
  */
  // Method to get the current font
  const GFXfont *getFont() const {
    return this->gfxFont; // Access the font pointer from the base class
  }
};

extern Oled oled_display;
