// global.cpp — desktop platform global instances.
//
// Mirrors MCL/src/platform/rp2040/global.cpp:1-156 minus the RP2040/TBD-only
// hardware bring-up. Provides every extern MCL's cross-platform code expects:
// MidiUart{,2,USB} + seq_tx{1..4}, Sysex parsers, Midi wrappers, sysex
// listeners, MD/MNM/A4 device singletons, generic/null MIDI devices, the
// MidiSetup + MidiActivePeering managers, GuiClass GUI, SdFat_ SD, oled
// display, and the volatile g_clock_*/g_fps/g_random_state counters.
//
// Without these, any consumer that links libmcl_desktop.a fails with
// "Undefined symbols" at link time (caught by mcl_desktop_linktest).
#include "platform.h"
#include "MidiUart.h"
#include "MidiSysex.h"
#include "MidiIDSysex.h"
#include "Midi.h"
#include "Devices/MidiSetup.h"
#include "memory.h"
#include "oled.h"
#include "GUI.h"
#include "../../mcl/Drivers/MD/MD.h"
#include "../../mcl/Drivers/MNM/MNM.h"
#include "../../mcl/Drivers/A4/A4.h"
#include "../../mcl/Drivers/Generic/GenericMidiDevice.h"
#include "Devices/MidiActivePeering.h"
#include "global.h"

// Hosted port 0 is the manifest-declared SPS link and can carry a worst-case
// v0x41 pattern dump. Other UARTs retain the compact internal TX ring.
namespace {
uint8_t sps_tx_buf[SPS_TX_BUF_SIZE];
}
MidiUartClass    MidiUart(sps_tx_buf, (uint16_t)sizeof(sps_tx_buf));
MidiUartClass    MidiUart2;
MidiUartUSBClass MidiUartUSB;
MidiUartClass    seq_tx1;
MidiUartClass    seq_tx2;
MidiUartClass    seq_tx3;
MidiUartClass    seq_tx4;

// Sysex parsers. Port 0 uses the hosted SPS capacity from memory.h; the
// external/USB parsers retain their compact legacy buffers.
namespace {
uint8_t sysex_buf1   [SYSEX1_DATA_LEN];
uint8_t sysex_buf2   [SYSEX2_DATA_LEN];
uint8_t sysex_bufusb [SYSEXUSB_DATA_LEN];
RingBuffer<> sysex_rb1   (sysex_buf1,   SYSEX1_DATA_LEN);
RingBuffer<> sysex_rb2   (sysex_buf2,   SYSEX2_DATA_LEN);
RingBuffer<> sysex_rbusb (sysex_bufusb, SYSEXUSB_DATA_LEN);
}

MidiSysexClass MidiSysex   (&MidiUart,    &sysex_rb1);
MidiSysexClass MidiSysex2  (&MidiUart2,   &sysex_rb2);
MidiSysexClass MidiSysexUSB(&MidiUartUSB, &sysex_rbusb);

// Midi wrappers tie a UART to a Sysex parser.
MidiClass Midi   (&MidiUart,    &MidiSysex);
MidiClass Midi2  (&MidiUart2,   &MidiSysex2);
MidiClass MidiUSB(&MidiUartUSB, &MidiSysexUSB);

// MidiUartParent::handle_midi_lock is a static volatile member; provide
// the storage.
volatile uint8_t MidiUartParent::handle_midi_lock = 0;

// Clocks read by MCL UI/sequencer code. Lazy-tick from millis() — see
// platform.cpp for the tick helper; here we just provide storage.
volatile uint16_t g_clock_fast   = 0;
volatile uint16_t g_clock_ms     = 0;
volatile uint16_t g_clock_ticks  = 0;
volatile uint16_t g_clock_minutes = 0;
volatile uint16_t g_clock_fps    = 0;
volatile uint16_t g_fps          = 0;
volatile uint16_t g_random_state = 0;

// Placeholder firmware ID; rp2040 derives this from a code checksum.
const uint16_t firmware_checksum = 0xDE5C;

// GUI
GuiClass GUI;

// Sysex listeners — singletons MCL's MIDI-routing code registers globally.
MidiIDSysexListenerClass MidiIDSysexListener;
MDSysexListenerClass     MDSysexListener;
MNMSysexListenerClass    MNMSysexListener;
A4SysexListenerClass     A4SysexListener;

// Device drivers
MDClass            MD;
MNMClass           MNM;
A4Class            Analog4;
GenericMidiDevice  generic_midi_device;
NullMidiDevice     null_midi_device;

// Active-port peering + MIDI setup helpers
MidiActivePeering midi_active_peering;
MidiSetup         midi_setup;

// OLED — desktop Oled is a self-contained stub (see oled.h); the rp2040
// constructor wants (w, h, SPI*, dc, rst, cs, speed). Our Oled has a
// default ctor — already instantiated in oled.cpp. Nothing to add here.

// SdFat singleton. Backed by std::filesystem (SdFat.cpp).
SdFat_ SD;

// MCL's cross-platform code calls handleIncomingMidi() from its scheduler
// to pump byte streams through each MidiClass. Same body as rp2040 minus
// the P4 (TBD-only) branch.
void handleIncomingMidi() {
    uint8_t lock = MidiUartParent::handle_midi_lock;
    MidiUartParent::handle_midi_lock = 1;

    Midi.processSysex();
    Midi2.processSysex();
    Midi.processMidi();
    Midi2.processMidi();

    MidiUartUSB.poll();
    MidiUSB.processSysex();
    MidiUSB.processMidi();

    MidiUartParent::handle_midi_lock = lock;
}
