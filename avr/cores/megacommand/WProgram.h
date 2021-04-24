#ifndef WProgram_h
#define WProgram_h

//#define DISABLE_MACHINE_NAMES 1

#include "Core.h"

#define MIDIDUINO 1
#define SYSEX_BUF_SIZE 6000

#include "wiring_private.h"

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

// #define DEBUGMODE

#ifdef MEGACOMMAND
  #define SD_CS 53 //PB0
#else
  #define SD_CS 9  //PE7
#endif

#define SERIAL_SPEED 1000000

#include "CommonTools/Debug.h"

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
