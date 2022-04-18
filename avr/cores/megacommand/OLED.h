#ifndef OLED_H__
#define OLED_H__

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include <ST7920_GFX_Library.h>
#define OLED_DISPLAY
#define OLED_CLK 52
#define OLED_MOSI 51

// Used for software or hardware SPI
#define OLED_CS 42
#define OLED_DC 44
#define OLED_RESET 38

#ifdef OLED_DISPLAY
extern ST7920 oled_display;
#endif

#endif /* OLED_H__ */
