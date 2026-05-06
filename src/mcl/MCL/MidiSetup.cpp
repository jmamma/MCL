#include "MidiSetup.h"
#include "MidiClock.h"
#include "TurboLight.h"
#include "SeqPages.h"
#include "MCLSysConfig.h"
#include "MidiActivePeering.h"
#include "DeviceManager.h"
#include "NoteInterface.h"
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MNM/MNM.h"
#include "../Drivers/A4/A4.h"
#ifdef PLATFORM_TBD
#include "../Drivers/TBD/TBD.h"
#endif
#include "MCLSeq.h"

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

} // namespace

void MidiSetup::cfg_clock_recv() {
  MidiClock.mode = MidiClock.EXTERNAL_UART1;
  MidiClock.uart_clock_recv = nullptr;

  // Always forward clock to MD, unless MD is the source.
  MidiClock.uart_clock_forward1 = MD.uart;
  switch (mcl_cfg.clock_rec) {
  case MIDI_CLOCK_SOURCE_PORT1:
    MidiClock.uart_clock_recv = MD.uart;
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
}

void MidiSetup::cfg_ports(bool boot) {
  DEBUG_PRINT_FN();

  mclsys_normalize_midi_config();
  configure_driver_ports();

  // Always receive transport on port1 for MD.
  MidiClock.uart_transport_recv1 = MD.uart;
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
    break;
#endif
  }
  if (MidiClock.uart_transport_forward1 == MidiClock.uart_transport_recv2) {
    MidiClock.uart_transport_forward1 = nullptr;
  }

  MidiClock.uart_transport_forward2 = nullptr;
  MidiClock.uart_transport_forward3 = nullptr;
#ifdef PLATFORM_TBD
  MidiClock.uart_transport_forward4 = nullptr;
#endif
  switch (mcl_cfg.midi_transport_send) {
  case 1:
    if (MidiClock.uart_transport_recv2 == &MidiUart2 && mcl_cfg.uart2_device > 0) {
    }
    else {
      MidiClock.uart_transport_forward2 = &MidiUart2;
    }
    break;
  case 2:
    MidiClock.uart_transport_forward3 = &MidiUartUSB;
    break;
  case 3:
    if (MidiClock.uart_transport_recv2 == &MidiUart2 && mcl_cfg.uart2_device > 0) {
    }
    else {
      MidiClock.uart_transport_forward2 = &MidiUart2;
    }
    MidiClock.uart_transport_forward3 = &MidiUartUSB;
    break;
  }
#ifdef PLATFORM_TBD
  cfg_p4_transport_forward();
#endif

  cfg_clock_recv();

  MidiClock.uart_clock_forward2 = nullptr;
  MidiClock.uart_clock_forward3 = nullptr;
#ifdef PLATFORM_TBD
  MidiClock.uart_clock_forward4 = nullptr;
#endif
  switch (mcl_cfg.clock_send) {
  case 1:
    if (MidiClock.uart_clock_recv == &MidiUart2 && mcl_cfg.uart2_device > 0) {
    }
    else {
      MidiClock.uart_clock_forward2 = &MidiUart2;
    }
    break;
  case 2:
    #ifndef DEBUGMODE
    MidiClock.uart_clock_forward3 = &MidiUartUSB;
    #endif
    break;
  case 3:
    if (MidiClock.uart_clock_recv == &MidiUart2 && mcl_cfg.uart2_device > 0) {
    }
    else {
      MidiClock.uart_clock_forward2 = &MidiUart2;
    }
#ifndef DEBUGMODE
    MidiClock.uart_clock_forward3 = &MidiUartUSB;
    #endif
    break;
  }
#ifdef PLATFORM_TBD
  cfg_p4_clock_forward();
#endif
  Midi.uart_forward[0] = nullptr;
  Midi.uart_forward[1] = nullptr;
  if (mcl_cfg.midi_forward_1 == 1 || mcl_cfg.midi_forward_1 == 3) {
    Midi.uart_forward[0] = &MidiUart2;
  }
  if (mcl_cfg.midi_forward_1 == 2 || mcl_cfg.midi_forward_1 == 3) {
    Midi.uart_forward[1] = &MidiUartUSB;
    ;
  }

  Midi2.uart_forward[0] = nullptr;
  Midi2.uart_forward[1] = nullptr;
  if (mcl_cfg.midi_forward_2 == 1 || mcl_cfg.midi_forward_2 == 3) {
    Midi2.uart_forward[0] = &MidiUart;
  }
  if (mcl_cfg.midi_forward_2 == 2 || mcl_cfg.midi_forward_2 == 3) {
    Midi2.uart_forward[1] = &MidiUartUSB;
    ;
  }

  MidiUSB.uart_forward[0] = nullptr;
  MidiUSB.uart_forward[1] = nullptr;
  if (mcl_cfg.midi_forward_usb == 1 || mcl_cfg.midi_forward_usb == 3) {
    MidiUSB.uart_forward[0] = &MidiUart;
  }
  if (mcl_cfg.midi_forward_usb == 2 || mcl_cfg.midi_forward_usb == 3) {
    MidiUSB.uart_forward[1] = &MidiUart2;
    ;
  }

  #if defined(__AVR__) && !defined(DEBUGMODE)
  if (!boot) {
          turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.usb_turbo_speed),
                            MidiUSB.uart);
  }
  #endif

  // Apply driver/turbo for one physical UART port.
  // USB ports are not handled here (peering happens in MidiActivePeering::run()).
  auto apply_port = [](uint8_t port, MidiUartClass *uart,
                       uint8_t turbo_cfg, uint8_t device_cfg) {
    if (device_cfg == 2) {                                  // OFF
      midi_active_peering.force_connect(port, &null_midi_device);
    } else if (device_cfg == 0) {                           // GENER
      midi_active_peering.disconnect(port);
      midi_active_peering.force_connect(port, &generic_midi_device);
      turbo_light.set_speed(turbo_light.lookup_speed(turbo_cfg), uart);
    } else {                                                // MD / ELEKT (1)
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
  };

  apply_port(UART1_PORT, &MidiUart,  mcl_cfg.uart1_turbo_speed, mcl_cfg.uart1_device);
  apply_port(UART2_PORT, &MidiUart2, mcl_cfg.uart2_turbo_speed, mcl_cfg.uart2_device);

  // USB-hosted MD or ELEKT slot: run setup() on connected Elektron device.
  // (USB GENER is handled below; USB OFF leaves nothing to do.)
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);
  auto setup_usb_slot = [](const PortSlot &slot) {
    if (slot.port != UARTUSB_PORT) return;
    MidiDevice *dev = device_manager.device_for_port(slot.port);
    ElektronDevice *e = dev ? dev->asElektronDevice() : nullptr;
    if (e) {
      turbo_light.set_speed(turbo_light.lookup_speed(slot.turbo_cfg), slot.uart);
      delay(100);
      e->setup();
    }
  };
  setup_usb_slot(s[SLOT_MD]);
  setup_usb_slot(s[SLOT_ELEKT]);

  // USB GENER
  if (mcl_cfg.usb_device == 3) {
    midi_active_peering.disconnect(UARTUSB_PORT);
    midi_active_peering.force_connect(UARTUSB_PORT, &generic_midi_device);
  }

#ifdef PLATFORM_TBD
  cfg_p4_device_connection();
#endif

  if (MD.connected) {
    seq_ptc_page.midi_events.remove_callbacks();
    seq_ptc_page.midi_events.setup_callbacks();
  }
  note_interface.ni_midi_events.remove_callbacks();
  note_interface.ni_midi_events.setup_callbacks();
  mcl_seq.midi_events.remove_callbacks();
  mcl_seq.midi_events.setup_callbacks();
}

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
#ifdef PLATFORM_TBD
  } else if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    slots[SLOT_MD] = {UARTP4_PORT, &MidiP4, &MidiUartP4, 0, false};
#endif
  }

  PortSlot grid_y_slot = {UART2_PORT, &Midi2, &MidiUart2,
                          mcl_cfg.uart2_turbo_speed, false};
  if (mcl_cfg.grid_y_port == GRID_Y_PORT_USB) {
    grid_y_slot = {UARTUSB_PORT, &MidiUSB, &MidiUartUSB,
                   mcl_cfg.usb_turbo_speed, false};
  }
#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    grid_y_slot = {UARTP4_PORT, &MidiP4, &MidiUartP4, 0, false};
  }
#endif

  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    slots[SLOT_GENER] = grid_y_slot;
  } else if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT) {
    slots[SLOT_ELEKT] = grid_y_slot;
#ifdef PLATFORM_TBD
  } else if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    slots[SLOT_GENER] = grid_y_slot;
#endif
  }
}

void configure_driver_ports() {
  mclsys_normalize_midi_config();

  PortSlot s[SLOT_COUNT];
  resolve_slots(s);

#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD) {
    MD.setPort(s[SLOT_MD].midi, s[SLOT_MD].port);
  } else {
    MD.cleanup_listeners();
    MD.midi = s[SLOT_MD].midi;
    MD.uart = s[SLOT_MD].uart;
    MD.port = s[SLOT_MD].port;
  }
#else
  MD.setPort(s[SLOT_MD].midi, s[SLOT_MD].port);
#endif
  MNM.setPort(s[SLOT_ELEKT].midi, s[SLOT_ELEKT].port);
  Analog4.setPort(s[SLOT_ELEKT].midi, s[SLOT_ELEKT].port);

  // GENER falls back to ELEKT slot's port when no UART is configured GENER
  PortSlot &g = s[SLOT_GENER].port ? s[SLOT_GENER] : s[SLOT_ELEKT];
#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    generic_midi_device.setPort(g.midi, g.port);
  }
#else
  generic_midi_device.setPort(g.midi, g.port);
#endif

  mcl_seq.set_outputs(s[SLOT_MD].uart ? s[SLOT_MD].uart : MD.uart,
                      g.uart ? g.uart : generic_midi_device.uart);
  device_manager.update_active_slots();
}
