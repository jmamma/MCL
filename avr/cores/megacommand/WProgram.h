#ifndef WProgram_h
#define WProgram_h

//#define DISABLE_MACHINE_NAMES 1

#include "Core.h"

#define MIDIDUINO 1
#define SYSEX_BUF_SIZE 6000
//#define SYSEX_BUF_SIZE 128

#include "wiring_private.h"

//#define DEBUGMODE

#ifdef MEGACOMMAND
  #define SD_CS 53 //PB0
#else
  #define SD_CS 9  //PE7
#endif

#define SERIAL_SPEED 250000

#ifdef DEBUGMODE

#define DEBUG_INIT() Serial.begin(SERIAL_SPEED);

#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_DUMP(x)  { \
}
// __PRETTY_FUNCTION__ is a gcc extension
// #define DEBUG_PRINT_FN(x) { \
//   DEBUG_PRINT(F("func_call: ")); \
//   Serial.println(__PRETTY_FUNCTION__); \
// }
#define DEBUG_PRINT_FN(x)

#else
#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_DUMP(x)
#define DEBUG_PRINT_FN(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#include <avr/interrupt.h>
#ifdef __cplusplus
  void __mainInnerLoop(bool callLoop = true);

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


extern uint32_t write_count;
extern uint32_t write_count_time;
extern uint16_t minuteclock;

#endif /* WProgram_h */
