#include "MCL_impl.h"

void MidiSetup::cfg_ports() {
  DEBUG_PRINT_FN();

  Midi.uart_forward = nullptr;
  Midi2.uart_forward = nullptr;

  if (mcl_cfg.midi_forward == 1 || mcl_cfg.midi_forward == 3) {
    Midi.uart_forward = &MidiUart2;
  }
  if (mcl_cfg.midi_forward == 2 || mcl_cfg.midi_forward == 3) {
    Midi2.uart_forward = &MidiUart;
  }

  if (mcl_cfg.clock_send == 1) {
    MidiClock.transmit_uart2 = true;
  } else {
    MidiClock.transmit_uart2 = false;
  }
  if (mcl_cfg.clock_rec == 0) {
    MidiClock.mode = MidiClock.EXTERNAL_MIDI;
    MidiClock.transmit_uart1 = false;

  } else {
    MidiClock.transmit_uart1 = true;
    // MidiClock.transmit_uart2 = false;
    MidiClock.mode = MidiClock.EXTERNAL_UART2;
  }

  ElektronDevice *elektron_devs[2] = {
      midi_active_peering.get_device(UART1_PORT)->asElektronDevice(),
      midi_active_peering.get_device(UART2_PORT)->asElektronDevice(),
  };

  if (elektron_devs[0]) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo), 1);
    delay(100);
    elektron_devs[0]->setup();
  }
  if (mcl_cfg.uart2_device == 0) {
    midi_active_peering.force_connect(UART2_PORT, &generic_midi_device);
  } else if (elektron_devs[1]) {
    elektron_devs[1]->setup();
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo), 2);
  } else {
    midi_active_peering.force_connect(UART2_PORT, &null_midi_device);
  }
}
