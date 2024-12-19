#include "MidiSetup.h"
#include "MidiClock.h"
#include "TurboLight.h"
#include "SeqPages.h"
#include "MCLSysConfig.h"

void MidiSetup::cfg_clock_recv() {
  MidiClock.mode = MidiClock.EXTERNAL_UART1;

  // Always receive clock on port1, unless source.
  MidiClock.uart_clock_forward1 = &MidiUart;
  switch (mcl_cfg.clock_rec) {
  case 0:
    MidiClock.uart_clock_recv = &MidiUart;
    MidiClock.uart_clock_forward1 = nullptr;
    break;
  case 1:
    MidiClock.uart_clock_recv = &MidiUart2;
    break;
  case 2:
    MidiClock.uart_clock_recv = &MidiUartUSB;
    break;
  }
}

void MidiSetup::cfg_ports(bool boot) {
  DEBUG_PRINT_FN();

  // Always receive transport on port1 for MD.
  ElektronDevice *elektron_devs[2] = {
      midi_active_peering.get_device(UART1_PORT)->asElektronDevice(),
      midi_active_peering.get_device(UART2_PORT)->asElektronDevice(),
  };


  MidiClock.uart_transport_recv1 = &MidiUart;
  MidiClock.uart_transport_forward1 = &MidiUart;
  MidiClock.uart_transport_recv2 = nullptr;
  switch (mcl_cfg.midi_transport_rec) {
  case 0:
    MidiClock.uart_transport_forward1 = nullptr;
    break;
  case 1:
    MidiClock.uart_transport_recv2 = &MidiUart2;
    break;
  case 2:
    MidiClock.uart_transport_recv2 = &MidiUartUSB;
    break;
  }

  MidiClock.uart_transport_forward2 = nullptr;
  MidiClock.uart_transport_forward3 = nullptr;
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

  MidiClock.uart_clock_forward2 = nullptr;
  MidiClock.uart_clock_forward3 = nullptr;
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
  cfg_clock_recv();

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

  if (!boot) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.usb_turbo_speed),
                            MidiUSB.uart);
  }

  if (mcl_cfg.uart1_device == 0) {
     midi_active_peering.disconnect(UART1_PORT);
          midi_active_peering.force_connect(UART1_PORT, &generic_midi_device);
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo_speed),
                            Midi.uart);
  } else if (elektron_devs[0]) {
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo_speed),
                          Midi.uart);
      delay(100);
      elektron_devs[0]->setup();
  } else {
      midi_active_peering.force_connect(UART1_PORT, &null_midi_device);
  }

  if (mcl_cfg.uart2_device == 0) {
      midi_active_peering.disconnect(UART2_PORT);
      midi_active_peering.force_connect(UART2_PORT, &generic_midi_device);
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo_speed),
                            Midi2.uart);
  } else if (elektron_devs[1]) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo_speed),
                           Midi2.uart);
    delay(100);
    elektron_devs[1]->setup();

  } else {
    midi_active_peering.force_connect(UART2_PORT, &null_midi_device);
  }

  if (MD.connected) {
    seq_ptc_page.midi_events.remove_callbacks();
    seq_ptc_page.midi_events.setup_callbacks();
  }
}
