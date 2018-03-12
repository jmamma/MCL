#include "MidiActivePeering.h"

void MidiActivePeering::md_setup() {
  MidiUart.setSpeed((uint32_t)31250, 1);

  for (uint8_t x = 0; x < 3 && MD.connected == false; x++) {

    delay(300);
    if (MD.getBlockingStatus(MD_CURRENT_GLOBAL_SLOT_REQUEST,
                             CALLBACK_TIMEOUT)) {

      turboSetSpeed(cfg_speed_to_turbo(cfg.uart1_turbo), 1);

      delay(100);

      switchGlobal(7);
      MD.resetMidiMap();
      MD.global.baseChannel = 9;

      if (!MD.getBlockingGlobal(7)) {
        MD.connected = false;
        return;
      }
      if (!global_one.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
        GUI.flash_strings_fill("GLOBAL", "ERROR");
        MD.connected = false;
        return;
      }
      if (rec_global != 1) {

        rec_global = 1;
        send_globals();
      }
      switchGlobal(7);
      uint8_t curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
      for (uint8_t x = 0; x < 2; x++) {
        for (uint8_t y = 0; y < 16; y++) {
          MD.setStatus(0x22, y);
        }
      }
      MD.setStatus(0x22, curtrack);
      MD.connected = true;
      GUI.flash_strings_fill("MD", "CONNECTED");

      return;
    }
  }
  MD.connected = false;
}

void MidiActivePeering::a4_setup() {
  MidiUart.setSpeed(31250, 2);
  for (uint8_t x = 0; x < 3 && Analog4.connected == false; x++) {
    delay(300);
    if (Analog4.getBlockingSettings(0)) {
      GUI.flash_strings_fill("A4", "CONNECTED");

      Analog4.connected = true;
      uart2_device = DEVICE_A4;
      turboSetSpeed(cfg_speed_to_turbo(cfg.uart2_turbo), 2);
    }
  }
  if (Analog4.connected == false) {
    // If sysex not receiverd assume generic midi device;
    uart2_device = DEVICE_MIDI;
    GUI.flash_strings_fill("MIDI DEVICE", "CONNECTED");
  }
}
void MidiActivePeering::check() {
  if (MD.connected == true) {
    if ((MidiUart.recvActiveSenseTimer > 300) && (MidiUart.speed > 31250)) {
      //  if (!MD.getBlockingStatus(0x22,CALLBACK_TIMEOUT)) {
      MidiUart.setSpeed((uint32_t)31250, 1);
      MD.connected = false;
      GUI.flash_strings_fill("MD", "DISCONNECTED");
      //   }
    }
  } else if (MD.connected == false) {
    if (MidiUart.recvActiveSenseTimer < 100) {
      md_setup();
      // if (Analog4.connected == false) { a4_setup(); }
    }
  }

  if (Analog4.connected == true) {
    if ((MidiUart2.recvActiveSenseTimer > 300) && (MidiUart2.speed > 31250)) {
      //  if (!MD.getBlockingStatus(0x22,CALLBACK_TIMEOUT)) {
      MidiUart.setSpeed(31250, 2);
      Analog4.connected = false;
      uart2_device = DEVICE_NULL;
      GUI.flash_strings_fill("A4", "DISCONNECTED");
      //   }
    }
  } else if ((Analog4.connected == false) && (uart2_device == DEVICE_NULL)) {
    if (MidiUart2.recvActiveSenseTimer < 100) {
      // delay(2000);
      a4_setup();
    }
  }
}

MidiActivePeering midi_active_peering;
