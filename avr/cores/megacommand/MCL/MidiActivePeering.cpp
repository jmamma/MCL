//#include "MidiActivePeering.h"
#include "MCL.h"

uint8_t MidiActivePeering::get_device(uint8_t port) {
  uint8_t uart1_device = MidiUart.device.get_id();
  uint8_t uart2_device = MidiUart2.device.get_id();
  if (port == UART1_PORT) {
    return uart1_device;
  }
  if (port == UART2_PORT) {
    return uart2_device;
  }
  return 255;
}

void MidiActivePeering::md_setup() {
  DEBUG_PRINT_FN();

  MidiIDSysexListener.setup();
  MidiUart.set_speed((uint32_t)31250, 1);

  for (uint8_t x = 0; x < 3 && MD.connected == false; x++) {

    delay(300);
    if (MidiUart.device.getBlockingId(DEVICE_MD, UART1_PORT,
                                      CALLBACK_TIMEOUT)) {

      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo), 1);

      delay(1200);

      md_exploit.rec_global = 1;

      md_exploit.send_globals();
      md_exploit.switch_global(7);
      //      uint8_t curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
      for (uint8_t x = 0; x < 2; x++) {
        for (uint8_t y = 0; y < 16; y++) {
          MD.setStatus(0x22, y);
        }
      }
      MD.setStatus(0x22, 0);
      MD.connected = true;
      GUI.flash_strings_fill("MD", "CONNECTED");

      return;
    }
  }
  MD.connected = false;

  MidiIDSysexListener.cleanup();
}

void MidiActivePeering::a4_setup() {
  DEBUG_PRINT_FN();
  MidiUart.set_speed(31250, 2);
  for (uint8_t x = 0; x < 3 && Analog4.connected == false; x++) {
    delay(300);
    if (Analog4.getBlockingSettings(0)) {
      GUI.flash_strings_fill("A4", "CONNECTED");

      Analog4.connected = true;
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo), 2);
    }
  }
  if (Analog4.connected == false) {
    // If sysex not receiverd assume generic midi device;
    MidiUart2.device.set_id(DEVICE_MIDI);
    GUI.flash_strings_fill("MIDI DEVICE", "CONNECTED");
  }
}
void MidiActivePeering::check() {
  char str[16];
  uint8_t uart1_device = MidiUart.device.get_id();
  uint8_t uart2_device = MidiUart2.device.get_id();

  if (uart1_device != DEVICE_NULL) {
    if ((MidiUart.recvActiveSenseTimer > 300) && (MidiUart.speed > 31250)) {
      MidiUart.set_speed((uint32_t)31250, 1);
      MD.connected = false;

      GUI.flash_strings_fill(MidiUart.device.get_name(str), "DISCONNECTED");
      MidiUart.device.init();
    }
  } else if (uart1_device == DEVICE_NULL) {
    if (MidiUart.recvActiveSenseTimer < 100) {
      md_setup();
    }
  }

  if (Analog4.connected == true) {
    if ((MidiUart2.recvActiveSenseTimer > 300) && (MidiUart2.speed > 31250)) {
      MidiUart.set_speed(31250, 2);
      Analog4.connected = false;
      GUI.flash_strings_fill(MidiUart2.device.get_name(str), "DISCONNECTED");
      MidiUart2.device.init();
    }
  } else if ((Analog4.connected == false) && (uart2_device == DEVICE_NULL)) {
    if (MidiUart2.recvActiveSenseTimer < 100) {
      a4_setup();
    }
  }
}
