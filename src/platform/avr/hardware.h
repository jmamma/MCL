#pragma once
#define USB_SERIAL  3
#define USB_MIDI    2
#define USB_STORAGE 1
#define USB_DFU     0

#ifdef MEGACOMMAND
  const int SPI1_CS_PIN = 53; //PB0
#else
  const int SPI1_CS_PIN = 9;  //PE7
#endif

extern void change_usb_mode(uint8_t mode);
extern void(* hardwareReset) (void);
