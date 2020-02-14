//#include "MidiActivePeering.h"
#include "MCL.h"
#include "MidiActivePeering.h"

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

void MidiActivePeering::prepare_display() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  oled_display.setFont();
  oled_display.setCursor(60, 10);
  oled_display.println("Peering...");
#endif
}

void MidiActivePeering::delay_progress(uint16_t clock_) {
  uint16_t myclock = slowclock;
  while (clock_diff(myclock, slowclock) < clock_) {
    mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
  }
}

void MidiActivePeering::md_setup() {
  DEBUG_PRINT_FN();

  MidiIDSysexListener.setup(&Midi);
  MidiUart.set_speed((uint32_t)31250, 1);
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();
  prepare_display();
  oled_display.drawBitmap(14, 8, icon_md, 34, 42, WHITE);
  oled_display.display();
#else
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at_fill(0, "Peering...");
  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  LCD.goLine(1);
  LCD.puts(GUI.lines[1].data);
#endif
  // Hack to prevent unnecessary delay on MC boot
  MD.connected = false;
  uint16_t myclock = slowclock;

  md_track_select.off();
  if ((slowclock > 3000) || (MidiClock.div16th_counter > 4)) {
    delay_progress(4600);
  }

  for (uint8_t x = 0; x < 3 && MD.connected == false; x++) {
    if (MidiUart.device.getBlockingId(DEVICE_MD, UART1_PORT,
                                      CALLBACK_TIMEOUT)) {
      DEBUG_PRINTLN("Midi ID: success");
      turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo), 1);
      // wait 300 ms, shoul be enought time to allow midiclock tempo to be
      // calculated before proceeding.
      myclock = slowclock;

      delay_progress(400);
      md_exploit.send_globals();
      MD.getCurrentTrack(CALLBACK_TIMEOUT);
      for (uint8_t x = 0; x < 2; x++) {
        for (uint8_t y = 0; y < 16; y++) {
          mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
          MD.setStatus(0x22, y);
        }
      }
      MD.setStatus(0x22, MD.currentTrack);
      MD.connected = true;
      MD.setGlobal(7);
      MD.global.baseChannel = 9;
      if (!MD.get_fw_caps()) {
         oled_display.textbox("UPGRADE ", "MACHINEDRUM");
         oled_display.display();
         while (1);
      }
      MD.activate_track_select();
      MD.getBlockingKit(0xF7);
#ifndef OLED_DISPLAY
      GUI.flash_strings_fill("MD", "CONNECTED");
#endif
    }
    if (MD.connected == false) {
      DEBUG_PRINTLN("delay");
      delay(250);
    }
  }
  if (mcl_cfg.track_select == 1) {
    md_track_select.on();
  }
  MidiIDSysexListener.cleanup();
#ifdef OLED_DISPLAY
  oled_display.setFont(oldfont);
#endif
}

void MidiActivePeering::a4_setup() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY

  auto oldfont = oled_display.getFont();
  prepare_display();
  oled_display.drawBitmap(14, 8, icon_a4, 34, 42, WHITE);
  oled_display.display();
#else
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at_fill(0, "Peering...");
  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  LCD.goLine(1);
  LCD.puts(GUI.lines[1].data);
#endif
  MidiUart.set_speed(31250, 2);
  for (uint8_t x = 0; x < 3 && Analog4.connected == false; x++) {
    delay_progress(300);
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
#ifdef OLED_DISPLAY
  oled_display.setFont(oldfont);
#endif
}
void MidiActivePeering::run() {
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

MidiActivePeering midi_active_peering;
