#include "MidiSetup.hh"

void MidiSetup::cfg_ports() {

  MidiClock.stop();

  if (cfg.clock_send == 1) {
    MidiClock.transmit_uart2 = true;
  }
  else {
    MidiClock.transmit_uart2 = false;
  }
  if (cfg.clock_rec == 0) {
    MidiClock.mode = MidiClock.EXTERNAL_MIDI;
    MidiClock.transmit_uart1 = false;

  }
  else {
    MidiClock.transmit_uart1 = true;
    MidiClock.transmit_uart2 = false;
    MidiClock.mode = MidiClock.EXTERNAL_UART2;
  }
  MidiClock.start();
  if (MD.connected) {
    send_globals();

    delay(100);

    switchGlobal(7);
  }

  if (MD.connected) {
    turboSetSpeed(cfg_speed_to_turbo(cfg.uart1_turbo), 1);
  }
  if (Analog4.connected) {
    turboSetSpeed(cfg_speed_to_turbo(cfg.uart2_turbo), 2);
  }
}
