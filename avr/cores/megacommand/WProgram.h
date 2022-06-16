
#ifndef WProgram_h
#define WProgram_h

//#define DISABLE_MACHINE_NAMES 1

#include "Core.h"

#define MIDIDUINO 1
#define SYSEX_BUF_SIZE 6000

#include "wiring_private.h"

#define DEBUGMODE

#ifdef MEGACOMMAND
  #define SD_CS 53 //PB0
#else
  #define SD_CS 9  //PE7
#endif

#define SERIAL_SPEED 250000

#ifdef DEBUGMODE

#define DEBUG_INIT() { change_usb_mode(0x03);  MidiUartUSB.mode = UART_SERIAL; MidiUartUSB.set_speed(SERIAL_SPEED); }

#define DEBUG_PRINT(x)  MidiUartUSB.print(x)
#define DEBUG_PRINTLN(x)  MidiUartUSB.println(x)
#define DEBUG_DUMP(x)  { \
}
// __PRETTY_FUNCTION__ is a gcc extension
// #define DEBUG_PRINT_FN(x) { \
//   DEBUG_PRINT(F("func_call: ")); \
//   Serial.println(__PRETTY_FUNCTION__); \
// }
//
#define DEBUG_CHECK_STACK() { if ((int) SP < 0x200 || (int)SP > 0x2200) { cli(); setLed2(); setLed(); while (1); } }
#define DEBUG_PRINT_FN(x)

#else
#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_DUMP(x)
#define DEBUG_PRINT_FN(x)
#define DEBUG_CHECK_STACK()
#endif

#ifdef __cplusplus
extern "C" {
#include <inttypes.h>
#include <avr/interrupt.h>
}
#endif

#include "CommonTools/helpers.h"

/* default config flags */
#define MIDIDUINO_POLL_GUI     1
#define MIDIDUINO_POLL_GUI_IRQ 1
#define MIDIDUINO_HANDLE_SYSEX 1
#define MIDIDUINO_MIDI_CLOCK   1
#define MIDIDUINO_USE_GUI      1
// #define MIDIDUINO_EXTERNAL_RAM 1
// #define MDIIDUINO_SD_CARD      1

#define USB_SERIAL  3
#define USB_MIDI    2
#define USB_STORAGE 1
#define USB_DFU     0

#define IS_MEGACMD() IS_BIT_CLEAR(PINK,PK2)
#define SET_USB_MODE(x) { PORTK = ((x)); }

#define LOCAL_SPI_ENABLE() { DDRB = 0xFF; PORTL |= _BV(PL4); }
#define LOCAL_SPI_DISABLE() { DDRB = 0; PORTB = 0; }

#define EXTERNAL_SPI_ENABLE() { PORTL |= _BV(PL3); }
#define EXTERNAL_SPI_DISABLE() { PORTL &= ~_BV(PL3); }

#include "mididuino_private.h"
#include "memory.h"

#ifdef __cplusplus

#include "LCD.h"
#include "OLED.h"
#include "GUI_private.h"
#include "MidiUart.h"

#include "MidiClock.h"
#include "Stack.h"
#include "GUI.h"
#include "Midi.h"
#include "WMath.h"
#endif

extern void(* hardwareReset) (void);
extern void change_usb_mode(uint8_t mode);
extern uint32_t write_count;
extern uint32_t write_count_time;
extern uint16_t minuteclock;

#endif /* WProgram_h */
