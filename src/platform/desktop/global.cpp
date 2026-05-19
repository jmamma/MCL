// global.cpp — desktop platform global instances.
//
// Step 1 defines only what the platform stubs themselves reference. The
// MidiUart/Midi/MidiSysex/GUI instances get fleshed out in step 3 once MCL's
// cross-platform headers (src/mcl/Midi/, src/mcl/GUI/) are on the include
// path; for now the externs in global.h compile but nothing in the desktop
// platform actually instantiates them, so leaving them undefined is fine
// until something pulls them in.
#include "platform.h"
#include "MidiUart.h"
#include "global.h"

// MCL's own Diagnostic/DebugBuffer.cpp defines `DebugBuffer debugBuffer;`,
// so we don't instantiate it here.

// Frame counters MCL UI code reads; benign at zero on desktop.
volatile uint16_t g_fps           = 0;
volatile uint16_t g_clock_fps     = 0;
volatile uint16_t g_clock_minutes = 0;
volatile uint16_t g_clock_ticks   = 0;

// Placeholder. Real firmware ID would be derived from a checksum of the
// installed code; on desktop a fixed value is fine.
const uint16_t firmware_checksum = 0xDE5C;

// MidiUart placeholder instances — wired to ring buffers in step 3.
MidiUartClass    MidiUart;
MidiUartClass    MidiUart2;
MidiUartUSBClass MidiUartUSB;
MidiUartClass    seq_tx1;
MidiUartClass    seq_tx2;
MidiUartClass    seq_tx3;
MidiUartClass    seq_tx4;

// handleIncomingMidi is declared extern in MCL's cross-platform code; provide
// a no-op until step 3 wires in the real handler from src/mcl/Midi/.
void handleIncomingMidi() {}
