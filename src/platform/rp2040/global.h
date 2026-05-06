// global.h
#pragma once

extern const uint16_t firmware_checksum;

class MidiClass;
class MidiUartClass;
class MidiUartUSBClass;
class MidiUartP4Class;
class MidiSysexClass;
class TbdDevice;

extern MidiUartClass MidiUart;
extern MidiUartClass MidiUart2;
extern MidiUartUSBClass MidiUartUSB;
#ifdef PLATFORM_TBD
extern MidiUartP4Class MidiUartP4;
#endif

extern MidiClass Midi;
extern MidiClass Midi2;
extern MidiClass MidiUSB;
#ifdef PLATFORM_TBD
extern MidiClass MidiP4;
#endif

extern MidiSysexClass MidiSysex;
extern MidiSysexClass MidiSysex2;
extern MidiSysexClass MidiSysexUSB;
#ifdef PLATFORM_TBD
extern MidiSysexClass MidiSysexP4;
#endif

extern MidiUartClass seq_tx1;
extern MidiUartClass seq_tx2;
extern MidiUartClass seq_tx3;
extern MidiUartClass seq_tx4;

extern volatile uint16_t g_fps;
extern volatile uint16_t g_clock_fps;

extern volatile uint16_t g_clock_minutes;
extern volatile uint16_t g_clock_ticks;

extern void handleIncomingMidi();

#ifdef PLATFORM_TBD
extern TbdDevice TBD;
#endif

#include "GUI.h"

extern GuiClass GUI;
