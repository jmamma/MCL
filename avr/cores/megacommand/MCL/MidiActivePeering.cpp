#include "MCL_impl.h"

/// It is the caller's responsibility to check for null MidiUart device
static MidiUartParent *_getMidiUart(uint8_t port) {
  if (port == UART1_PORT)
    return &MidiUart;
#ifdef EXT_TRACKS
  else if (port == UART2_PORT)
    return &MidiUart2;
#endif
  else
    return nullptr;
}

/// It is the caller's responsibility to check for null MidiClass device
static MidiClass *_getMidiClass(uint8_t port) {
  if (port == UART1_PORT)
    return &Midi;
#ifdef EXT_TRACKS
  else if (port == UART2_PORT)
    return &Midi2;
#endif
  else
    return nullptr;
}

static void prepare_display() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  oled_display.setFont();
  oled_display.setCursor(60, 10);
  oled_display.println("Peering...");
#endif
}

static void delay_progress(uint16_t clock_) {
  uint16_t myclock = slowclock;
  while (clock_diff(myclock, slowclock) < clock_) {
    mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
  }
}

// TODO port is ignored
static bool md_setup(uint8_t port) {
  DEBUG_PRINT_FN();

  bool ts = md_track_select.state;
  bool ti = trig_interface.state;

  if (ts) {
    md_track_select.off();
  }
  if (ti) {
    trig_interface.off();
  }

  // Hack to prevent unnecessary delay on MC boot
  MD.connected = false;
  uint16_t myclock = slowclock;

  if ((slowclock > 3000) || (MidiClock.div16th_counter > 4)) {
    delay_progress(4600);
  }

  // Begin main probe sequence
  if (MidiUart.device.getBlockingId(DEVICE_MD, UART1_PORT, CALLBACK_TIMEOUT)) {
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
#ifdef OLED_DISPLAY
      oled_display.textbox("UPGRADE ", "MACHINEDRUM");
      oled_display.display();
#else
      gfx.display_text("UPGRADE", "MACHINEDRUM");
#endif
      while (1)
        ;
    }
    MD.getBlockingKit(0xF7);
  }

  if (MD.connected == false) {
    DEBUG_PRINTLN("delay");
    delay_progress(250);
  }

  if (ts) {
    md_track_select.on();
  }
  if (ti) {
    trig_interface.on();
  }

  return MD.connected;
}

// TODO port is ignored
static bool a4_setup(uint8_t port) {
  DEBUG_PRINT_FN();

  delay_progress(300);
  if (Analog4.getBlockingSettings(0)) {
    Analog4.connected = true;
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo), 2);
  }
  return Analog4.connected;
}

static bool mnm_setup(uint8_t port) {
  uint16_t myclock = slowclock;
  if (255 != MNM.getCurrentKit(CALLBACK_TIMEOUT)) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo), UART2_PORT);
    // wait 400 ms, shoul be enought time to allow midiclock tempo to be
    // calculated before proceeding.
    myclock = slowclock;

    delay_progress(400);

    //MNM.global.origPosition = 7;
    //MNM.global.arpOut = true;
    //MNM.global.autotrackChannel = 9;
    //MNM.global.baseFreq = 440;

    //MNM.loadGlobal(7);
    if (!MNM.getBlockingGlobal(7)) {
      return false;
    }

    //DIAG_PRINTLN("mnm getglobal ok");

    auto &g = MNM.global;
    MNM.connected = g.fromSysex(MNM.midi);
    if (!MNM.connected) {
      DEBUG_PRINTLN("MNM fromSysex failed");
      return false;
    }

    DEBUG_DUMP(g.arpOut);
    DEBUG_DUMP(g.autotrackChannel);
    DEBUG_DUMP(g.baseChannel);
    DEBUG_DUMP(g.channelSpan);
    DEBUG_DUMP(g.clockIn);
    DEBUG_DUMP(g.clockOut);
    DEBUG_DUMP(g.ctrlIn);
    DEBUG_DUMP(g.ctrlOut);
    DEBUG_DUMP(g.keyboardOut);
    DEBUG_DUMP(g.midiClockOut);
    DEBUG_DUMP(g.transportIn);
    DEBUG_DUMP(g.transportOut);
    DEBUG_DUMP(g.origPosition);

    g.arpOut = true;
    g.autotrackChannel = 9;
    g.baseChannel = 1;
    g.channelSpan = 6;
    g.clockIn = true;
    g.clockOut = true;
    g.ctrlIn = true;
    g.ctrlOut = true;
    g.keyboardOut = 2;
    g.midiClockOut = 1;
    g.transportIn = true;
    g.transportOut = true;
    g.origPosition = 7;

    delay_progress(400);

    MNMDataToSysexEncoder encoder(MNM.midi->uart);
    g.toSysex(encoder);

    delay_progress(400);

    MNM.loadGlobal(7);

    return MNM.connected;
  }

  return false;
}

// the general probe accept whatever devices.
static bool midi_device_setup(uint8_t port) { return true; }

static void md_disconnect() { MD.connected = false; }

static void a4_disconnect() { Analog4.connected = false; }

static void mnm_disconnect() { MNM.connected = false; }

static midi_peer_driver_t port1_drivers[] = {
  {DEVICE_MD, "MD", md_setup, md_disconnect, icon_md},
};

static midi_peer_driver_t port2_drivers[] = {
  {DEVICE_MNM, "MM", mnm_setup, mnm_disconnect, icon_mnm },
  {DEVICE_A4, "A4", a4_setup, a4_disconnect, icon_a4},
  {DEVICE_MIDI, "MIDI Device", midi_device_setup, nullptr, nullptr},
};

static void probePort(uint8_t port, midi_peer_driver_t drivers[],
                      size_t nr_drivers) {
  auto *pmidi = _getMidiUart(port);
  auto *pmidi_class = _getMidiClass(port);
  if (!pmidi || !pmidi_class)
    return;
  uint8_t id = pmidi->device.get_id();
  if (id != DEVICE_NULL && pmidi->recvActiveSenseTimer > 300 &&
      pmidi->speed > 31250) {
    MidiUart.set_speed((uint32_t)31250, port);
    for (size_t i = 0; i < nr_drivers; ++i) {
      if (drivers[i].disconnect) drivers[i].disconnect();
    }
#ifndef OLED_DISPLAY
    char str[16];
    GUI.flash_strings_fill(pmidi->device.get_name(str), "DISCONNECTED");
#endif
    // reset MidiID to none
    pmidi->device.init();
  } else if (id == DEVICE_NULL && pmidi->recvActiveSenseTimer < 100) {
    bool probe_success = false;
    for (size_t i = 0; i < nr_drivers; ++i) {

      MidiIDSysexListener.setup(pmidi_class);
      MidiUart.set_speed((uint32_t)31250, port);
#ifdef OLED_DISPLAY
      auto oldfont = oled_display.getFont();
      prepare_display();
      if (drivers[i].icon) {
        oled_display.drawBitmap(14, 8, drivers[i].icon, 34, 42, WHITE);
      }
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

      for (int probe_retry = 0; probe_retry < 3 && !probe_success; ++probe_retry) {
        probe_success = drivers[i].probe(port);
      } // for retries

      MidiIDSysexListener.cleanup();
      GUI.currentPage()->redisplay = true;
#ifdef OLED_DISPLAY
      oled_display.setFont(oldfont);
#endif

      if (probe_success) {
        pmidi->device.set_id(drivers[i].id);
        pmidi->device.set_name(drivers[i].name);
#ifndef OLED_DISPLAY
        GUI.flash_strings_fill(drivers[i].name, "CONNECTED");
#endif
        break;
      }
    } // for drivers
    GUI.currentPage()->redisplay = true;
  }
}

uint8_t MidiActivePeering::get_device(uint8_t port) {
  auto pmidi = _getMidiUart(port);
  if (pmidi)
    return pmidi->device.get_id();
  else
    return DEVICE_NULL;
}

void MidiActivePeering::run() {
  probePort(UART1_PORT, port1_drivers, countof(port1_drivers));
#ifdef EXT_TRACKS
  probePort(UART2_PORT, port2_drivers, countof(port2_drivers));
#endif
}

MidiActivePeering midi_active_peering;
