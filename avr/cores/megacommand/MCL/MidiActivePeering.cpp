#include "MCL_impl.h"
#include "ResourceManager.h"

void GenericMidiDevice::init_grid_devices() {
  uint8_t grid_idx = 1;
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    add_track_to_grid(grid_idx, i, &(mcl_seq.ext_tracks[i]), EXT_TRACK_TYPE);
  }
}

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

static bool resource_loaded = false;
static size_t resource_size = 0;
static void prepare_display(uint8_t* buf) {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  oled_display.setFont();
  oled_display.setCursor(60, 10);
  oled_display.println("Peering...");
#endif
  if (!resource_loaded) {
    R.Clear();
    R.use_icons_device();
    resource_loaded = true;
  }
}

// the general probe accept whatever devices.
static bool midi_device_setup(uint8_t port) { return true; }

static MidiDevice *port1_drivers[] = {&MD};

static MidiDevice *port2_drivers[] = {
    &MNM,
    &Analog4,
    &generic_midi_device,
};

static MidiDevice *connected_midi_devices[2] = {&null_midi_device,
                                                &null_midi_device};

void MidiActivePeering::disconnect(uint8_t port) {
  MidiDevice **drivers;
  uint8_t nr_drivers = 1;
  if (port == UART1_PORT) {
    drivers = port1_drivers;
  } else {
    drivers = port2_drivers;
    nr_drivers = 3;
  }
  for (size_t i = 0; i < nr_drivers; ++i) {
    if (drivers[i]->connected) {
      if (midi_active_peering.get_device(port)->asElektronDevice()) {
        turbo_light.set_speed(0, port);
      }
      drivers[i]->disconnect();
    }
  }
}

void MidiActivePeering::force_connect(uint8_t port, MidiDevice *driver) {
  MidiDevice **connected_dev;

  connected_dev = &connected_midi_devices[port - 1];

  midi_active_peering.disconnect(port);
  auto *pmidi = _getMidiUart(port);
  pmidi->device.init();
  pmidi->device.set_name(driver->name);
  pmidi->device.set_id(driver->id);
  driver->init_grid_devices();

  *connected_dev = driver;
}

static void probePort(uint8_t port, MidiDevice *drivers[], size_t nr_drivers,
                      MidiDevice **active_device, uint8_t* resource_buf) {
  auto *pmidi = _getMidiUart(port);
  auto *pmidi_class = _getMidiClass(port);
  if (!pmidi || !pmidi_class)
    return;
  uint8_t id = pmidi->device.get_id();
  oled_display.setTextColor(WHITE, BLACK);
  if (id != DEVICE_NULL && pmidi->recvActiveSenseTimer > 300 &&
      pmidi->speed > 31250) {
    MidiUart.set_speed((uint32_t)31250, port);
    for (size_t i = 0; i < nr_drivers; ++i) {
      if (drivers[i]->connected)
        drivers[i]->disconnect();
    }
#ifndef OLED_DISPLAY
    char str[16];
    GUI.flash_strings_fill(pmidi->device.get_name(str), "DISCONNECTED");
#endif
    // reset MidiID to none
    pmidi->device.init();
    // reset connected device to /dev/null
    *active_device = &null_midi_device;
  } else if (id == DEVICE_NULL && pmidi->recvActiveSenseTimer < 100) {
    bool probe_success = false;
    for (size_t i = 0; i < nr_drivers; ++i) {

      MidiIDSysexListener.setup(pmidi_class);
      MidiUart.set_speed((uint32_t)31250, port);
#ifdef OLED_DISPLAY
      auto oldfont = oled_display.getFont();
      prepare_display(resource_buf);
      uint8_t* icon = drivers[i]->icon();
      if (icon) {
        oled_display.drawBitmap(14, 8, icon, 34, 42, WHITE);
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

      for (int probe_retry = 0; probe_retry < 3 && !probe_success;
           ++probe_retry) {
        probe_success = drivers[i]->probe();
      } // for retries

      MidiIDSysexListener.cleanup();
      GUI.currentPage()->redisplay = true;
#ifdef OLED_DISPLAY
      oled_display.setFont(oldfont);
#endif

      if (probe_success) {
        pmidi->device.set_id(drivers[i]->id);
        pmidi->device.set_name(drivers[i]->name);
        drivers[i]->init_grid_devices();
        *active_device = drivers[i];
#ifndef OLED_DISPLAY
        GUI.flash_strings_fill(drivers[i].name, "CONNECTED");
#endif
        break;
      }
    } // for drivers
    GUI.currentPage()->redisplay = true;
  }
}

MidiDevice *MidiActivePeering::get_device(uint8_t port) {
  if (port == 1) {
    return connected_midi_devices[0];
  } else if (port == 2) {
    return connected_midi_devices[1];
  } else {
    return &null_midi_device;
  }
}

GenericMidiDevice::GenericMidiDevice()
    : MidiDevice(&Midi2, "MI", DEVICE_MIDI, false) {}
NullMidiDevice::NullMidiDevice()
    : MidiDevice(nullptr, "  ", DEVICE_NULL, false) {}

void MidiActivePeering::run() {
  byte resource_buf[RM_BUFSIZE];
  resource_loaded = false;
  probePort(UART1_PORT, port1_drivers, countof(port1_drivers),
            &connected_midi_devices[0], resource_buf);
#ifdef EXT_TRACKS
  probePort(UART2_PORT, port2_drivers, countof(port2_drivers),
            &connected_midi_devices[1], resource_buf);
  if (resource_loaded) {
    // XXX doesn't work yet
    //R.Restore(resource_buf, resource_size);
    GUI.currentPage()->init();
    resource_loaded = false;
  }
#endif
}
