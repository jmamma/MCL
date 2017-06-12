#ifndef MIDIDUINO_PRIVATE_H__
#define MIDIDUINO_PRIVATE_H__

#include "helpers.h"

void init(void);

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


#define FIRMWARE_LENGTH_ADDR ((uint16_t *)0x00)
#define FIRMWARE_CHECKSUM_ADDR ((uint16_t *)0x02)
#define START_MAIN_APP_ADDR ((uint16_t *)0x04)

#define BOARD_ID 0x41

void start_bootloader(void);

void handleIncomingMidi();

#endif /* MIDIDUIDNO_PRIVATE_H__ */
