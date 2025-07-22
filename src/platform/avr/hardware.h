#pragma once
#define USB_SERIAL  3
#define USB_MIDI    2
#define USB_STORAGE 1
#define USB_DFU     0

#include "helpers.h"


#ifdef MEGACOMMAND
#define SPI1_SS_PIN 53 //PB0
#else
#define SPI1_SS_PIN 9  //PE7
#endif

#define SD_CONFIG SdSpiConfig(SPI1_SS_PIN, DEDICATED_SPI, SD_SCK_MHZ(12), &SPI)

#ifdef MEGACOMMAND

inline void toggleLed(void) {
  TOGGLE_BIT(PORTE, PE5);
}
inline void setLed(void) {
  SET_BIT(PORTE, PE5);
}
inline void clearLed(void) {
  CLEAR_BIT(PORTE, PE5);
}
inline void setLed2(void) {
  SET_BIT(PORTE, PE4);
}
inline void clearLed2(void) {
  CLEAR_BIT(PORTE, PE4);
}
inline void toggleLed2(void) {
  TOGGLE_BIT(PORTE, PE4);
}
#else
inline void toggleLed(void) {
  TOGGLE_BIT(PORTE, PE4);
}

inline void setLed(void) {
  CLEAR_BIT(PORTE, PE4);
}

inline void clearLed(void) {
  SET_BIT(PORTE, PE4);
}

inline void setLed2(void) {
  CLEAR_BIT(PORTE, PE5);
}

inline void clearLed2(void) {
  SET_BIT(PORTE, PE5);
}

inline void toggleLed2(void) {
  TOGGLE_BIT(PORTE, PE5);
}
#endif

typedef struct encoder_s {
  int8_t normal;
  int8_t button;
} encoder_t;

extern void change_usb_mode(uint8_t mode);
extern void(* hardwareReset) (void);
