/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDISETUP_H__
#define MIDISETUP_H__

#include <stdint.h>

#define UART1_PORT 1
#define UART2_PORT 2
#define UARTUSB_PORT 3

class MidiClass;
class MidiUartClass;

class MidiSetup {
  public:
  void cfg_clock_recv();
  void cfg_ports(bool boot = false);
};

extern MidiSetup midi_setup;

void configure_driver_ports();

enum SlotIdx : uint8_t {
  SLOT_MD = 0,
  SLOT_ELEKT = 1,
  SLOT_GENER = 2,
  SLOT_COUNT = 3,
};

struct PortSlot {
  uint8_t port;            // UART1_PORT / UART2_PORT / UARTUSB_PORT, 0 = unassigned
  MidiClass *midi;
  MidiUartClass *uart;
  uint8_t turbo_cfg;       // mcl_cfg.uartX_turbo_speed
  bool off;                // true if device==2 (OFF) on this UART
};

void resolve_slots(PortSlot slots[SLOT_COUNT]);

#endif /* MIDISETUP_H__ */
