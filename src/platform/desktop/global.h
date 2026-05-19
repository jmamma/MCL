// global.h — desktop platform extern declarations.
//
// Mirrors MCL/src/platform/rp2040/global.h:1-54 minus the RP2040-only
// MidiUartP4 / TBD device hooks. Defining the same externs allows MCL's
// cross-platform code (src/mcl/Midi/, src/mcl/GUI/, src/mcl/MCL/) to link
// unchanged against this static lib.
#pragma once

#include <cstdint>

extern const uint16_t firmware_checksum;

class MidiClass;
class MidiUartClass;
class MidiUartUSBClass;
class MidiSysexClass;

extern MidiUartClass    MidiUart;
extern MidiUartClass    MidiUart2;
extern MidiUartUSBClass MidiUartUSB;

extern MidiClass        Midi;
extern MidiClass        Midi2;
extern MidiClass        MidiUSB;

extern MidiSysexClass   MidiSysex;
extern MidiSysexClass   MidiSysex2;
extern MidiSysexClass   MidiSysexUSB;

extern MidiUartClass    seq_tx1;
extern MidiUartClass    seq_tx2;
extern MidiUartClass    seq_tx3;
extern MidiUartClass    seq_tx4;

extern volatile uint16_t g_fps;
extern volatile uint16_t g_clock_fps;
extern volatile uint16_t g_clock_minutes;
extern volatile uint16_t g_clock_ticks;

extern void handleIncomingMidi();

// Pull GuiClass + LightPage/gui_event_t in via GUI.h — same as
// MCL/src/platform/rp2040/global.h:51-53. Cross-platform code in
// MCL/src/mcl/MCL/mcl.h:18 expects these names to be complete here.
#include "GUI.h"

extern GuiClass GUI;
