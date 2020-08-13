#include "MCL.h"
#include "MidiSetup.h"

void MidiSetup::cfg_ports() {
  DEBUG_PRINT_FN();
  MidiClock.stop();

  if (mcl_cfg.midi_forward == 1) {
    Midi.forward = true;
  } else {
    Midi.forward = false;
  }
  if (mcl_cfg.midi_forward == 2) {
    Midi2.forward = true;
  } else {
    Midi2.forward = false;
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
    //MidiClock.transmit_uart2 = false;
    MidiClock.mode = MidiClock.EXTERNAL_UART2;
  }
  if (mcl_cfg.extmidi & 1) {
    Midi.ext_in = true;
    Midi2.ext_in = true;
  } else {
    Midi.ext_in = false;
    Midi2.ext_in = false;
  }
  if (mcl_cfg.extmidi & 2) {
    Midi.ext_out = true;
    Midi2.ext_out = true;
  } else {
    Midi.ext_out = false;
    Midi2.ext_out = false;
  }

  if (MD.connected) {
    md_exploit.send_globals();

    delay(100);

    md_exploit.switch_global(7);
  }

  if (MD.connected) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo), 1);
  }
  if (Analog4.connected) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo), 2);
  }

  MidiClock.start();

}
