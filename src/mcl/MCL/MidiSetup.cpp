#include "MidiSetup.h"
#include "MidiClock.h"
#include "MidiID.h"
#include "TurboLight.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "MCLSysConfig.h"
#include "MidiActivePeering.h"
#include "DeviceManager.h"
#include "NoteInterface.h"
#include "MCLActions.h"
#include "global.h"
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MNM/MNM.h"
#include "../Drivers/A4/A4.h"
#ifdef PLATFORM_TBD
#include "../Drivers/TBD/TBD.h"
#endif
#include "Sequencer/MCLSeq.h"

MidiClass *midi_class_for_port(uint8_t port) {
  if (port == UART1_PORT) return &Midi;
#ifdef EXT_TRACKS
  if (port == UART2_PORT) return &Midi2;
#endif
  if (port == UARTUSB_PORT) return &MidiUSB;
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT) return &MidiP4;
#endif
  return nullptr;
}

namespace {

#ifdef PLATFORM_TBD
bool tbd_p4_device_active() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD ||
         mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD;
}

bool midi_forward_list_has(MidiUartClass *uart, MidiUartClass *a,
                           MidiUartClass *b, MidiUartClass *c) {
  return uart != nullptr && (uart == a || uart == b || uart == c);
}

void cfg_p4_clock_forward() {
  MidiClock.uart_clock_forward4 = nullptr;
  if (!tbd_p4_device_active() || MidiClock.uart_clock_recv == &MidiUartP4 ||
      midi_forward_list_has(&MidiUartP4, MidiClock.uart_clock_forward1,
                            MidiClock.uart_clock_forward2,
                            MidiClock.uart_clock_forward3)) {
    return;
  }
  MidiClock.uart_clock_forward4 = &MidiUartP4;
}

void cfg_p4_transport_forward() {
  MidiClock.uart_transport_forward4 = nullptr;
  if (!tbd_p4_device_active() ||
      MidiClock.uart_transport_recv1 == &MidiUartP4 ||
      MidiClock.uart_transport_recv2 == &MidiUartP4 ||
      midi_forward_list_has(&MidiUartP4, MidiClock.uart_transport_forward1,
                            MidiClock.uart_transport_forward2,
                            MidiClock.uart_transport_forward3)) {
    return;
  }
  MidiClock.uart_transport_forward4 = &MidiUartP4;
}

void cfg_p4_device_connection() {
  if (tbd_p4_device_active()) {
    if (device_manager.device_for_port(UARTP4_PORT) == &TBD && TBD.connected) {
      TBD.sync_grid_devices();
      device_manager.update_active_slots();
      return;
    }
    midi_active_peering.force_connect(UARTP4_PORT, &TBD);
    return;
  }

  if (device_manager.device_for_port(UARTP4_PORT) != &null_midi_device) {
    midi_active_peering.disconnect(UARTP4_PORT);
  }
}
#endif

void apply_physical_port(uint8_t port, MidiUartClass *uart,
                         uint8_t turbo_cfg, uint8_t device_cfg) {
  if (device_cfg == 2) { // OFF
    midi_active_peering.force_connect(port, &null_midi_device);
  } else if (device_cfg == 0) { // GENER
    midi_active_peering.disconnect(port);
    midi_active_peering.force_connect(port, &generic_midi_device);
    turbo_light.set_speed(turbo_light.lookup_speed(turbo_cfg), uart);
  } else { // MD / ELEKT (1)
    MidiDevice *dev = device_manager.device_for_port(port);
    ElektronDevice *e = dev ? dev->asElektronDevice() : nullptr;
    if (e) {
      turbo_light.set_speed(turbo_light.lookup_speed(turbo_cfg), uart);
      delay(100);
      e->setup();
    } else {
      midi_active_peering.force_connect(port, &null_midi_device);
    }
  }
}

void cfg_send_forwards(uint8_t mode, MidiUartClass *recv,
                       bool block_first_when_recv,
                       MidiUartClass *&first, MidiUartClass *first_target,
                       MidiUartClass *&second, MidiUartClass *second_target) {
  first = nullptr;
  second = nullptr;
  if ((mode & 1) && !(recv == first_target && block_first_when_recv)) {
    first = first_target;
  }
  if (mode & 2) {
    second = second_target;
  }
}

void cfg_midi_forwards(MidiClass &midi, uint8_t mode, MidiUartClass *first,
                       MidiUartClass *second) {
  midi.uart_forward[0] = nullptr;
  midi.uart_forward[1] = nullptr;
  if (mode & 1) {
    midi.uart_forward[0] = first;
  }
  if (mode & 2) {
    midi.uart_forward[1] = second;
  }
}

MidiUartClass *port1_clock_transport_uart() {
#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    return &MidiUart;
  }
#endif
  return MD.uart;
}

#ifdef PLATFORM_TBD
void setup_usb_slot(const PortSlot &slot) {
  if (slot.port != UARTUSB_PORT) return;
  MidiDevice *dev = device_manager.device_for_port(slot.port);
  ElektronDevice *e = dev ? dev->asElektronDevice() : nullptr;
  if (e) {
    turbo_light.set_speed(turbo_light.lookup_speed(slot.turbo_cfg), slot.uart);
    delay(100);
    e->setup();
  }
}

void cfg_usb_device_connection(const PortSlot slots[SLOT_COUNT]) {
  uint8_t expected_slot = DeviceManager::LOGICAL_SLOT_NONE;
  if (slots[SLOT_MD].port == UARTUSB_PORT) {
    expected_slot = SLOT_MD;
  } else if (slots[SLOT_ELEKT].port == UARTUSB_PORT) {
    expected_slot = SLOT_ELEKT;
  } else if (slots[SLOT_GENER].port == UARTUSB_PORT) {
    expected_slot = SLOT_GENER;
  }

  if (expected_slot == SLOT_GENER) {
    midi_active_peering.disconnect(UARTUSB_PORT);
    midi_active_peering.force_connect(UARTUSB_PORT, &generic_midi_device);
    return;
  }

  MidiDevice *attached = device_manager.device_for_port(UARTUSB_PORT);
  uint8_t attached_slot = device_manager.logical_idx_for_port(UARTUSB_PORT);
  uint8_t usb_device_id = MidiUartUSB.device.get_id();
  bool stale = expected_slot == DeviceManager::LOGICAL_SLOT_NONE &&
               (attached != &null_midi_device ||
                usb_device_id != DEVICE_NULL);

  if (!stale && attached == &generic_midi_device) {
    stale = true;
  }
  if (!stale && attached != &null_midi_device &&
      attached_slot != expected_slot) {
    stale = true;
  }
  if (!stale && attached == &null_midi_device &&
      usb_device_id != DEVICE_NULL) {
    stale = true;
  }
  if (!stale && attached != &null_midi_device &&
      usb_device_id != attached->id) {
    stale = true;
  }

  if (stale) {
    midi_active_peering.force_connect(UARTUSB_PORT, &null_midi_device);
  }
}
#endif

void remove_port_sensitive_callbacks() {
  seq_ptc_page.midi_events.remove_callbacks();
  note_interface.ni_midi_events.remove_callbacks();
  mcl_actions_midievents.remove_callbacks();
  mcl_seq.midi_events.remove_callbacks();
}

void setup_port_sensitive_callbacks() {
  if (MD.connected) {
    seq_ptc_page.midi_events.setup_callbacks();
  }
  note_interface.ni_midi_events.setup_callbacks();
  mcl_actions_midievents.setup_callbacks();
  mcl_seq.midi_events.setup_callbacks();
}

#ifndef PLATFORM_TBD
uint8_t avr_grid_x_device_cfg() {
  return (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD) ? 1 : 2;
}

uint8_t avr_grid_y_device_cfg() {
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) return 0;
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT) return 1;
  return 2;
}

uint8_t turbo_cfg_for_port(uint8_t port, uint8_t uart_turbo_cfg) {
  return (port == UARTUSB_PORT) ? mcl_cfg.usb_turbo_speed : uart_turbo_cfg;
}

struct AvrPhysicalSlot {
  uint8_t port;
  MidiUartClass *uart;
  uint8_t device_cfg;
  uint8_t turbo_cfg;
};

AvrPhysicalSlot avr_grid_x_slot() {
  uint8_t port = (mcl_cfg.usb_device == 1) ? UARTUSB_PORT : UART1_PORT;
  return {port,
          (port == UARTUSB_PORT) ? &MidiUartUSB : &MidiUart,
          avr_grid_x_device_cfg(),
          turbo_cfg_for_port(port, mcl_cfg.uart1_turbo_speed)};
}

AvrPhysicalSlot avr_grid_y_slot() {
  uint8_t port =
      (mcl_cfg.usb_device == 2 || mcl_cfg.usb_device == 3) ? UARTUSB_PORT
                                                           : UART2_PORT;
  return {port,
          (port == UARTUSB_PORT) ? &MidiUartUSB : &MidiUart2,
          avr_grid_y_device_cfg(),
          turbo_cfg_for_port(port, mcl_cfg.uart2_turbo_speed)};
}
#endif

void detach_stale_physical_devices() {
  for (uint8_t port = UART1_PORT; port <= MIDI_PORT_COUNT; ++port) {
    MidiDevice *device = device_manager.device_for_port(port);
    if (device != &null_midi_device && device->port != port) {
      device_manager.detach_port(port);
    }
  }
}

} // namespace

void MidiSetup::cfg_clock_recv() {
  MidiClock.mode = MidiClock.EXTERNAL_UART1;
  MidiClock.uart_clock_recv = nullptr;

  // Always forward clock to MD, unless MD is the source.
  MidiClock.uart_clock_forward1 = MD.uart;
  switch (mcl_cfg.clock_rec) {
  case MIDI_CLOCK_SOURCE_PORT1:
    MidiClock.uart_clock_recv = port1_clock_transport_uart();
    MidiClock.uart_clock_forward1 = nullptr;
    break;
  case MIDI_CLOCK_SOURCE_PORT2:
    MidiClock.uart_clock_recv = &MidiUart2;
    break;
  case MIDI_CLOCK_SOURCE_USB:
    MidiClock.uart_clock_recv = &MidiUartUSB;
    break;
#ifdef PLATFORM_TBD
  case MIDI_CLOCK_SOURCE_INTERNAL:
    MidiClock.mode = MidiClock.INTERNAL_MIDI;
    MidiClock.resetInternalClockSource(true);
    break;
#endif
  }
  // Don't echo clock back to the port it came from.
  if (MidiClock.uart_clock_forward1 == MidiClock.uart_clock_recv) {
    MidiClock.uart_clock_forward1 = nullptr;
  }
#ifdef PLATFORM_TBD
  cfg_p4_clock_forward();
#endif
  if (MidiClock.uart_clock_recv) {
    MidiClock.reset_clock_phase = true;
  }
}

void MidiSetup::cfg_ports(bool boot) {
  DEBUG_PRINT_FN();

  remove_port_sensitive_callbacks();
  configure_driver_ports();

  // Always receive transport on port1 for MD.
  MidiClock.uart_transport_recv1 = port1_clock_transport_uart();
  MidiClock.uart_transport_forward1 = MD.uart;
  MidiClock.uart_transport_recv2 = nullptr;
  switch (mcl_cfg.midi_transport_rec) {
  case MIDI_CLOCK_SOURCE_PORT1:
    MidiClock.uart_transport_forward1 = nullptr;
    break;
  case MIDI_CLOCK_SOURCE_PORT2:
    MidiClock.uart_transport_recv2 = &MidiUart2;
    break;
  case MIDI_CLOCK_SOURCE_USB:
    MidiClock.uart_transport_recv2 = &MidiUartUSB;
    break;
#ifdef PLATFORM_TBD
  case MIDI_CLOCK_SOURCE_INTERNAL:
    MidiClock.uart_transport_recv1 = nullptr;
    if (tbd_p4_device_active()) {
      MidiClock.uart_transport_recv2 = &MidiUartP4;
    }
    break;
#endif
  }
  if (MidiClock.uart_transport_forward1 == MidiClock.uart_transport_recv2) {
    MidiClock.uart_transport_forward1 = nullptr;
  }

  cfg_send_forwards(mcl_cfg.midi_transport_send,
                    MidiClock.uart_transport_recv2, mcl_cfg.uart2_device > 0,
                    MidiClock.uart_transport_forward2, &MidiUart2,
                    MidiClock.uart_transport_forward3, &MidiUartUSB);
#ifdef PLATFORM_TBD
  MidiClock.uart_transport_forward4 = nullptr;
#endif
#ifdef PLATFORM_TBD
  cfg_p4_transport_forward();
#endif

  cfg_clock_recv();

  cfg_send_forwards(mcl_cfg.clock_send, MidiClock.uart_clock_recv,
                    mcl_cfg.uart2_device > 0, MidiClock.uart_clock_forward2,
                    &MidiUart2, MidiClock.uart_clock_forward3,
#ifndef DEBUGMODE
                    &MidiUartUSB
#else
                    nullptr
#endif
                    );
#ifdef PLATFORM_TBD
  MidiClock.uart_clock_forward4 = nullptr;
#endif
#ifdef PLATFORM_TBD
  cfg_p4_clock_forward();
#endif
  cfg_midi_forwards(Midi, mcl_cfg.midi_forward_1, &MidiUart2, &MidiUartUSB);
  cfg_midi_forwards(Midi2, mcl_cfg.midi_forward_2, &MidiUart, &MidiUartUSB);
  cfg_midi_forwards(MidiUSB, mcl_cfg.midi_forward_usb, &MidiUart, &MidiUart2);

  #if defined(__AVR__) && !defined(DEBUGMODE)
  if (!boot) {
          turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.usb_turbo_speed),
                            MidiUSB.uart);
  }
  #endif

  // USB ports are not handled here (peering happens in MidiActivePeering::run()).
#ifdef PLATFORM_TBD
  apply_physical_port(UART1_PORT, &MidiUart, mcl_cfg.uart1_turbo_speed,
                      mcl_cfg.uart1_device);
  apply_physical_port(UART2_PORT, &MidiUart2, mcl_cfg.uart2_turbo_speed,
                      mcl_cfg.uart2_device);

  // USB-hosted MD or ELEKT slot: run setup() on connected Elektron device.
  // (USB GENER is handled below; USB OFF leaves nothing to do.)
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);
  cfg_usb_device_connection(s);
  setup_usb_slot(s[SLOT_MD]);
  setup_usb_slot(s[SLOT_ELEKT]);
#else
  AvrPhysicalSlot x_slot = avr_grid_x_slot();
  AvrPhysicalSlot y_slot = avr_grid_y_slot();

  apply_physical_port(x_slot.port, x_slot.uart, x_slot.turbo_cfg,
                      x_slot.device_cfg);
  apply_physical_port(y_slot.port, y_slot.uart, y_slot.turbo_cfg,
                      y_slot.device_cfg);
#endif

#ifdef PLATFORM_TBD
  cfg_p4_device_connection();
#endif

  setup_port_sensitive_callbacks();
}

#ifdef PLATFORM_TBD
void resolve_slots(PortSlot slots[SLOT_COUNT]) {
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    slots[i] = {0, nullptr, nullptr, 0, false};

  slots[SLOT_MD] = {UART1_PORT, &Midi, &MidiUart,
                    mcl_cfg.uart1_turbo_speed, true};
  slots[SLOT_ELEKT] = {UART2_PORT, &Midi2, &MidiUart2,
                       mcl_cfg.uart2_turbo_speed, true};

  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD &&
      mcl_cfg.grid_x_port == GRID_X_PORT_USB) {
    slots[SLOT_MD] = {UARTUSB_PORT, &MidiUSB, &MidiUartUSB,
                      mcl_cfg.usb_turbo_speed, false};
  } else if (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD) {
    slots[SLOT_MD] = {UART1_PORT, &Midi, &MidiUart,
                      mcl_cfg.uart1_turbo_speed, false};
  } else if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    slots[SLOT_MD] = {UARTP4_PORT, &MidiP4, &MidiUartP4, 0, false};
  }

  PortSlot grid_y_slot = {UART2_PORT, &Midi2, &MidiUart2,
                          mcl_cfg.uart2_turbo_speed, false};
  if (mcl_cfg.grid_y_port == GRID_Y_PORT_USB) {
    grid_y_slot = {UARTUSB_PORT, &MidiUSB, &MidiUartUSB,
                   mcl_cfg.usb_turbo_speed, false};
  }
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    grid_y_slot = {UARTP4_PORT, &MidiP4, &MidiUartP4, 0, false};
  }

  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    slots[SLOT_GENER] = grid_y_slot;
  } else if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT) {
    slots[SLOT_ELEKT] = grid_y_slot;
  } else if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    slots[SLOT_GENER] = grid_y_slot;
  }
}
#endif

void configure_driver_ports() {
  mclsys_normalize_midi_config();

#ifdef PLATFORM_TBD
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);

  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD) {
    MD.setPort(s[SLOT_MD].midi, s[SLOT_MD].port);
  } else {
    MD.cleanup_listeners();
    if (MD.connected) {
      MD.disconnect(DeviceIdx::Primary);
    }
    MD.midi = s[SLOT_MD].midi;
    MD.uart = s[SLOT_MD].uart;
    MD.port = s[SLOT_MD].port;
  }
  MNM.setPort(s[SLOT_ELEKT].midi, s[SLOT_ELEKT].port);
  Analog4.setPort(s[SLOT_ELEKT].midi, s[SLOT_ELEKT].port);

  // GENER falls back to ELEKT slot's port when no UART is configured GENER
  PortSlot &g = s[SLOT_GENER].port ? s[SLOT_GENER] : s[SLOT_ELEKT];
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    generic_midi_device.setPort(g.midi, g.port);
  }

  mcl_seq.set_outputs(s[SLOT_MD].uart ? s[SLOT_MD].uart : MD.uart,
                      g.uart ? g.uart : generic_midi_device.uart);
#else
  uint8_t md_port = (mcl_cfg.usb_device == 1) ? UARTUSB_PORT : UART1_PORT;
  MidiClass *md_midi = (mcl_cfg.usb_device == 1) ? &MidiUSB : &Midi;

  uint8_t ext_port = (mcl_cfg.usb_device == 2) ? UARTUSB_PORT : UART2_PORT;
  MidiClass *ext_midi = (mcl_cfg.usb_device == 2) ? &MidiUSB : &Midi2;

  MidiClass *gen_midi = ext_midi;
  uint8_t gen_port = ext_port;
  if (mcl_cfg.usb_device == 3) {
    gen_midi = &MidiUSB;
    gen_port = UARTUSB_PORT;
  } else if (mcl_cfg.uart1_device == 0) {
    gen_midi = &Midi;
    gen_port = UART1_PORT;
  }

  MD.setPort(md_midi, md_port);
  MNM.setPort(ext_midi, ext_port);
  Analog4.setPort(ext_midi, ext_port);
  generic_midi_device.setPort(gen_midi, gen_port);

  mcl_seq.set_outputs(MD.uart, generic_midi_device.uart);
#endif
  detach_stale_physical_devices();
  device_manager.update_active_slots();
}
