#include "MidiSetup.h"
#include "MidiClock.h"
#include "TurboLight.h"
#include "SeqPages.h"
#include "MCLSysConfig.h"
#include "MidiActivePeering.h"
#include "MD.h"
#include "MNM.h"
#include "A4.h"
#include "MCLSeq.h"

void MidiSetup::cfg_clock_recv() {
  MidiClock.mode = MidiClock.EXTERNAL_UART1;

  // Always forward clock to MD, unless MD is the source.
  MidiClock.uart_clock_forward1 = MD.uart;
  switch (mcl_cfg.clock_rec) {
  case 0:
    MidiClock.uart_clock_recv = MD.uart;
    MidiClock.uart_clock_forward1 = nullptr;
    break;
  case 1:
    MidiClock.uart_clock_recv = &MidiUart2;
    break;
  case 2:
    MidiClock.uart_clock_recv = &MidiUartUSB;
    break;
  }
  // Don't echo clock back to the port it came from.
  if (MidiClock.uart_clock_forward1 == MidiClock.uart_clock_recv) {
    MidiClock.uart_clock_forward1 = nullptr;
  }
}

void MidiSetup::cfg_ports(bool boot) {
  DEBUG_PRINT_FN();

  configure_driver_ports();

  // Always receive transport on port1 for MD.
  ElektronDevice *elektron_devs[2] = {
      midi_active_peering.dev1->asElektronDevice(),
      midi_active_peering.dev2->asElektronDevice(),
  };


  MidiClock.uart_transport_recv1 = MD.uart;
  MidiClock.uart_transport_forward1 = MD.uart;
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
  if (MidiClock.uart_transport_forward1 == MidiClock.uart_transport_recv2) {
    MidiClock.uart_transport_forward1 = nullptr;
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

  #if defined(__AVR__) && !defined(DEBUGMODE)
  if (!boot) {
          turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.usb_turbo_speed),
                            MidiUSB.uart);
  }
  #endif

  uint8_t md_port = (mcl_cfg.usb_device == 1) ? UARTUSB_PORT : UART1_PORT;
  uint8_t ext_port = (mcl_cfg.usb_device == 2) ? UARTUSB_PORT : UART2_PORT;

  if (mcl_cfg.uart1_device == 2 && md_port != UARTUSB_PORT) {
      // OFF (physical port only; USB peering handled by run())
      midi_active_peering.force_connect(md_port, &null_midi_device);
  } else if (mcl_cfg.uart1_device == 0) {
      midi_active_peering.disconnect(md_port);
      midi_active_peering.force_connect(md_port, &generic_midi_device);
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo_speed),
                            MD.uart);
  } else if (elektron_devs[0]) {
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo_speed),
                          MD.uart);
      delay(100);
      elektron_devs[0]->setup();
  } else {
      midi_active_peering.force_connect(md_port, &null_midi_device);
  }

  if (mcl_cfg.uart2_device == 2 && ext_port != UARTUSB_PORT) {
      // OFF (physical port only; USB peering handled by run())
      midi_active_peering.force_connect(ext_port, &null_midi_device);
  } else if (mcl_cfg.uart2_device == 0) {
      midi_active_peering.disconnect(ext_port);
      midi_active_peering.force_connect(ext_port, &generic_midi_device);
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo_speed),
                            mcl_seq.ext_uart);
  } else if (elektron_devs[1]) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo_speed),
                           mcl_seq.ext_uart);
    delay(100);
    elektron_devs[1]->setup();
  } else {
    midi_active_peering.force_connect(ext_port, &null_midi_device);
  }

  // USB GENER handling
  if (mcl_cfg.usb_device == 3) {
    midi_active_peering.disconnect(UARTUSB_PORT);
    midi_active_peering.force_connect(UARTUSB_PORT, &generic_midi_device);
  }

  if (MD.connected) {
    seq_ptc_page.midi_events.remove_callbacks();
    seq_ptc_page.midi_events.setup_callbacks();
  }
}

void configure_driver_ports() {
  // MD on USB if usb_device==1, else default to UART1
  uint8_t md_port = (mcl_cfg.usb_device == 1) ? UARTUSB_PORT : UART1_PORT;
  MidiClass *md_midi = (mcl_cfg.usb_device == 1) ? &MidiUSB : &Midi;

  // ELEKT on USB if usb_device==2, else default to UART2
  uint8_t ext_port = (mcl_cfg.usb_device == 2) ? UARTUSB_PORT : UART2_PORT;
  MidiClass *ext_midi = (mcl_cfg.usb_device == 2) ? &MidiUSB : &Midi2;

  // GENER: check USB first, then PORT2, then PORT1
  MidiClass *gen_midi = ext_midi;
  uint8_t gen_port = ext_port;
  if (mcl_cfg.usb_device == 3) {
    gen_midi = &MidiUSB; gen_port = UARTUSB_PORT;
  } else if (mcl_cfg.uart1_device == 0) {
    gen_midi = &Midi; gen_port = UART1_PORT;
  }

  MD.setPort(md_midi, md_port);
  MNM.setPort(ext_midi, ext_port);
  Analog4.setPort(ext_midi, ext_port);
  generic_midi_device.setPort(gen_midi, gen_port);

  mcl_seq.set_ports(MD.uart, generic_midi_device.uart);

  midi_active_peering.update_dev_cache();
}
