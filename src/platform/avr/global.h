// global.h
#pragma once

extern const uint16_t firmware_checksum;

class MidiClass;
class MidiUartClass;
class MidiSysexClass;

extern MidiUartClass MidiUart;
extern MidiUartClass MidiUart2;
extern MidiUartClass MidiUartUSB;

extern MidiClass Midi;
extern MidiClass Midi2;
extern MidiClass MidiUSB;

extern MidiSysexClass MidiSysex;
extern MidiSysexClass MidiSysex2;
extern MidiSysexClass MidiSysexUSB;

extern MidiUartClass seq_tx1;
extern MidiUartClass seq_tx2;
extern MidiUartClass seq_tx3;
extern MidiUartClass seq_tx4;

extern volatile uint16_t g_fps;
extern volatile uint16_t g_clock_fps;

extern volatile uint16_t g_clock_minutes;
extern volatile uint16_t g_clock_ticks;

extern void handleIncomingMidi();

#include "GUI.h"

extern GuiClass GUI;
