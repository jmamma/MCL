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

#ifdef OLED_DISPLAY
extern Adafruit_SSD1305 oled_display;
#endif

