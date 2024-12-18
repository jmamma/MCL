#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>

#define OLED_DISPLAY

#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define OLED_SPEED 7000000UL

// Dedicated GPIO

#define OLED_RST 13
#define OLED_DC 14
#define OLED_CS 15
// SPI1

#define OLED_SCLK 10
#define OLED_MOSI 11


class Oled : public Adafruit_SSD1305 {
public:
  // Constructor, same as the base class constructor
  Oled(uint16_t w, uint16_t h, TwoWire *twi = &Wire,
                             int8_t rst_pin = -1, uint32_t preclk = 400000,
                             uint32_t postclk = 100000)
      : Adafruit_SSD1305(w, h, twi, rst_pin, preclk, postclk) {}

  // Constructor for SPI-based initialization
  Oled(uint16_t w, uint16_t h, int8_t mosi_pin, int8_t sclk_pin,
                             int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
      : Adafruit_SSD1305(w, h, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin) {}

  // Constructor for SPI initialization with custom bitrate
  Oled(uint16_t w, uint16_t h, SPIClass *spi, int8_t dc_pin,
                             int8_t rst_pin, int8_t cs_pin, uint32_t bitrate = 8000000UL)
      : Adafruit_SSD1305(w, h, spi, dc_pin, rst_pin, cs_pin, bitrate) {}

  // Method to get the current font
  const GFXfont* getFont() const {
    return this->gfxFont; // Access the font pointer from the base class
  }
};

extern Oled oled_display;

