#pragma once

// global.h stub for desktop builds

#include <stdint.h>

extern const uint16_t firmware_checksum;

// Forward declarations
class MidiClass;
class MidiUartClass;
class MidiSysexClass;
class GuiClass;

extern volatile uint16_t g_fps;
extern volatile uint16_t g_clock_fps;
extern volatile uint16_t g_clock_minutes;
extern volatile uint16_t g_clock_ticks;

inline void handleIncomingMidi() {}
