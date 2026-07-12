#ifndef OLED_H__
#define OLED_H__

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include "AvrHardwarePins.h"

#define OLED_DISPLAY
#define OLED_CLK 52
#define OLED_MOSI 51

// Used for software or hardware SPI
#define OLED_CS MCL_OLED_CS_PIN
#define OLED_DC MCL_OLED_DC_PIN
#define OLED_RESET MCL_OLED_RESET_PIN

#ifdef OLED_DISPLAY
extern Adafruit_SSD1305 oled_display;
#endif

#endif /* OLED_H__ */
