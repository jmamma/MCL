#ifndef OLED_H__
#define OLED_H__

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>

#define OLED_DISPLAY
#define OLED_CLK 52
#define OLED_MOSI 51

// Used for software or hardware SPI
#define OLED_CS 42
#define OLED_DC 44
#define OLED_RESET 38

extern Adafruit_SSD1305 oled_display;

#endif /* OLED_H__ */
