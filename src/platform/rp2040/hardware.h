#pragma once


const int SPI1_MISO_PIN = 8;
const int SPI1_MOSI_PIN = 11;
const int SPI1_SCK_PIN = 10;
const int SPI1_SS_PIN = 9;

#if defined(PLATFORM_TBD)

#define SD_CONFIG SdioConfig(RP_CLK_GPIO, RP_CMD_GPIO, RP_DAT0_GPIO)

#else

#define SD_CONFIG SdSpiConfig(SPI1_SS_PIN, DEDICATED_SPI, SD_SCK_MHZ(12), &SPI1)

#endif

#define USB_SERIAL  3
#define USB_MIDI    2
#define USB_STORAGE 1
#define USB_DFU     0

#ifdef PLATFORM_TBD

#define RP_CLK_GPIO 2 // Set to CLK GPIO
#define RP_CMD_GPIO 3 // Set to CMD GPIO
#define RP_DAT0_GPIO 4

#define RP_UART1_RX 37
#define RP_UART1_TX 36

#define RP_UART0_RX 45
#define RP_UART0_TX 44

#else

#define RP_UART0_RX 1
#define RP_UART0_TX 0

#define RP_UART1_RX 5
#define RP_UART1_TX 4

#endif

#define IS_MEGACMD() (true)
#define SET_USB_MODE(x) { }

#define LOCAL_SPI_ENABLE() { }
#define LOCAL_SPI_DISABLE() { }

#define EXTERNAL_SPI_ENABLE() { }
#define EXTERNAL_SPI_DISABLE() { }

typedef struct encoder_s {
  int8_t normal;
  int8_t button;
} encoder_t;

inline void toggleLed(void) {
}
inline void toggleLed2(void) {
}

#ifdef PLATFORM_TBD
void setLed(void);
void clearLed(void);
void setLed2(void);
void clearLed2(void);
#else
inline void setLed(void) {
}
inline void clearLed(void) {
}
inline void setLed2(void) {
}
inline void clearLed2(void) {
}
#endif
extern void change_usb_mode(uint8_t mode);

extern void picow_init();
