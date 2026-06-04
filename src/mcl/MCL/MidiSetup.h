/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDISETUP_H__
#define MIDISETUP_H__

#include <stdint.h>

#define UART1_PORT 1
#define UART2_PORT 2
#define UARTUSB_PORT 3
#ifdef PLATFORM_TBD
#define UARTP4_PORT 4
#define MIDI_PORT_COUNT 4
#else
#define MIDI_PORT_COUNT 3
#endif

#define MIDI_CLOCK_SOURCE_PORT1 0
#define MIDI_CLOCK_SOURCE_PORT2 1
#define MIDI_CLOCK_SOURCE_USB 2
#ifdef PLATFORM_TBD
#define MIDI_CLOCK_SOURCE_INTERNAL 3
#define MIDI_CLOCK_SOURCE_COUNT 4
#else
#define MIDI_CLOCK_SOURCE_COUNT 3
#endif

#define GRID_X_DEVICE_OFF 0
#define GRID_X_DEVICE_MD 1
#ifdef PLATFORM_TBD
#define GRID_X_DEVICE_TBD 2
#define GRID_X_PORT_INT 0
#define GRID_X_PORT_1 1
#define GRID_X_PORT_USB 2
#else
#define GRID_X_PORT_1 0
#define GRID_X_PORT_USB 1
#endif

#ifdef PLATFORM_TBD
#define GRID_Y_DEVICE_TBD 0
#endif
#define GRID_Y_DEVICE_GENER 1
#define GRID_Y_DEVICE_ELEKT 2
#define GRID_Y_DEVICE_OFF 3
#define GRID_Y_PORT_INT 0
#define GRID_Y_PORT_2 1
#define GRID_Y_PORT_USB 2

class MidiClass;
class MidiUartClass;

class MidiSetup {
  public:
  void cfg_clock_recv();
  void cfg_ports(bool boot = false);
};

extern MidiSetup midi_setup;

void configure_driver_ports();
MidiClass *midi_class_for_port(uint8_t port);

enum SlotIdx : uint8_t {
  SLOT_MD = 0,
  SLOT_ELEKT = 1,
  SLOT_GENER = 2,
  SLOT_COUNT = 3,
};

struct PortSlot {
  uint8_t port;            // UART*_PORT, 0 = unassigned
  MidiClass *midi;
  MidiUartClass *uart;
  uint8_t turbo_cfg;       // mcl_cfg.uartX_turbo_speed
  bool off;                // true if device==2 (OFF) on this UART
};

void resolve_slots(PortSlot slots[SLOT_COUNT]);

#endif /* MIDISETUP_H__ */
