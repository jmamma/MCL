//#include "MidiActivePeering.h"
#include "MCL.h"

uint8_t MidiActivePeering::get_device(uint8_t port) {
  if (port == UART1_PORT) {
    uint8_t uart1_device = MidiUart.device.get_id();
    return uart1_device;
  }
#ifdef EXT_TRACKS
  if (port == UART2_PORT) {
    uint8_t uart2_device = MidiUart2.device.get_id();
    return uart2_device;
  }
#endif
  return 255;
}

void MidiActivePeering::md_setup() {
  DEBUG_PRINT_FN();

  MidiIDSysexListener.setup(&Midi);
  MidiUart.set_speed((uint32_t)31250, 1);
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at_fill(0, "Peering...");
  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  LCD.goLine(1);
  LCD.puts(GUI.lines[1].data);
#ifdef OLED_DISPLAY
  oled_display.display();
#endif
  // Hack to prevent unnecessary delay on MC boot
  MD.connected = false;
  if ((slowclock > 3000) || (MidiClock.div16th_counter > 4)) {
    delay(4600);
  }
  for (uint8_t x = 0; x < 3 && MD.connected == false; x++) {
    if (MidiUart.device.getBlockingId(DEVICE_MD, UART1_PORT,
                                      CALLBACK_TIMEOUT)) {
      DEBUG_PRINTLN("Midi ID: success");
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo), 1);
      // wait 300 ms, shoul be enought time to allow midiclock tempo to be
      // calculated before proceeding.
      delay(400);

      md_exploit.rec_global = 1;

      md_exploit.send_globals();
      md_exploit.switch_global(7);
      //      uint8_t curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);
      MD.getCurrentTrack(CALLBACK_TIMEOUT);
      for (uint8_t x = 0; x < 2; x++) {
        for (uint8_t y = 0; y < 16; y++) {
          MD.setStatus(0x22, y);
        }
      }
      MD.setStatus(0x22, MD.currentTrack);
      MD.connected = true;
      // MD.setTempo(MidiClock.tempo * 24);
      MD.getCurrentKit();
      MD.getBlockingKit(MD.currentKit);
#ifndef OLED_DISPLAY
      GUI.flash_strings_fill("MD", "CONNECTED");
#endif
    }
    if (MD.connected == false) {
      DEBUG_PRINTLN("delay");
      delay(250);
    }
  }

  MidiIDSysexListener.cleanup();
}

void MidiActivePeering::a4_setup() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at_fill(0, "Peering...");
  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  LCD.goLine(1);
  LCD.puts(GUI.lines[1].data);
#ifdef OLED_DISPLAY
  oled_display.display();
#endif
  MidiUart.set_speed(31250, 2);
  for (uint8_t x = 0; x < 3 && Analog4.connected == false; x++) {
    delay(300);
    if (Analog4.getBlockingSettings(0)) {
      MidiUart2.device.set_id(DEVICE_A4);
#ifdef OLED_DISPLAY
      GUI.flash_strings_fill("A4", "CONNECTED");
#endif
      Analog4.connected = true;
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo), 2);
    }
  }
  if (Analog4.connected == false) {
    // If sysex not receiverd assume generic midi device;
    MidiUart2.device.set_id(DEVICE_MIDI);
#ifndef OLED_DISPLAY
    GUI.flash_strings_fill("MIDI DEVICE", "CONNECTED");
#endif
  }
}
void MidiActivePeering::check() {
  char str[16];
  uint8_t uart1_device = MidiUart.device.get_id();

  if (uart1_device != DEVICE_NULL) {
    if ((MidiUart.recvActiveSenseTimer > 300) && (MidiUart.speed > 31250)) {
      MidiUart.set_speed((uint32_t)31250, 1);
      MD.connected = false;
#ifndef OLED_DISPLAY
      GUI.flash_strings_fill(MidiUart.device.get_name(str), "DISCONNECTED");
#endif
      MidiUart.device.init();
    }
  } else if (uart1_device == DEVICE_NULL) {
    if (MidiUart.recvActiveSenseTimer < 100) {
      md_setup();
    }
  }
#ifdef EXT_TRACKS
  uint8_t uart2_device = MidiUart2.device.get_id();
  if (Analog4.connected == true) {
    if ((MidiUart2.recvActiveSenseTimer > 300) && (MidiUart2.speed > 31250)) {
      MidiUart.set_speed(31250, 2);
      Analog4.connected = false;
#ifndef OLED_DISPLAY
      GUI.flash_strings_fill(MidiUart2.device.get_name(str), "DISCONNECTED");
#endif
      MidiUart2.device.init();
    }
  } else if ((Analog4.connected == false) && (uart2_device == DEVICE_NULL)) {
    if (MidiUart2.recvActiveSenseTimer < 100) {
      a4_setup();
    }
  }
#endif
}
