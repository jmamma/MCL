// hardware.h — desktop platform GPIO mock + helper declarations.
//
// Mirrors the surface MCL/src/platform/rp2040/hardware.h exposes (encoder_t,
// LED toggles, SPI pin defines, USB-mode macros) so cross-platform MCL code
// that #include's "hardware.h" finds everything it expects. Most are inline
// no-ops on desktop. Compatible with both C and C++ TUs (helpers.c is C).
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Process-global mock pin state, written by digitalWrite, read by digitalRead.
extern uint8_t desktop_gpio_state[256];

extern void change_usb_mode(uint8_t mode);
extern void picow_init();

#ifdef __cplusplus
}
#endif

// SPI1 pin defines (used by oled.h). Plain macros so C TUs compile.
#define SPI1_MISO_PIN 8
#define SPI1_MOSI_PIN 11
#define SPI1_SCK_PIN  10
#define SPI1_SS_PIN   9
#define LED_BUILTIN   13

// Match rp2040's encoder polling struct.
typedef struct encoder_s {
    int8_t normal;
    int8_t button;
} encoder_t;

#ifdef __cplusplus
// LED toggle helpers — no-ops on desktop, same as the non-TBD rp2040 path.
// C++-only because helpers.c doesn't need them.
inline void toggleLed()  {}
inline void toggleLed2() {}
// Virtual front-panel status LEDs. Desktop/WASM implements these through
// GUI_hardware.led so the hosting product can render the same state as the
// physical target.
void setLed();
void clearLed();
void setLed2();
void clearLed2();
#endif

// USB / SPI mode macros — empty stubs.
#define IS_MEGACMD() (true)
#define SET_USB_MODE(x)        do {} while (0)
#define LOCAL_SPI_ENABLE()     do {} while (0)
#define LOCAL_SPI_DISABLE()    do {} while (0)
#define EXTERNAL_SPI_ENABLE()  do {} while (0)
#define EXTERNAL_SPI_DISABLE() do {} while (0)

#define USB_SERIAL  3
#define USB_MIDI    2
#define USB_STORAGE 1
#define USB_DFU     0

// SD_CONFIG is the rp2040's SdSpiConfig / SdioConfig. Desktop SD is backed by
// std::filesystem — the value passed to SD.begin() is ignored. Provide any
// integer so MCLSd.cpp:58 compiles.
#define SD_CONFIG 0
