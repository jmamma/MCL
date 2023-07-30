#include "MCL_impl.h"
#include "ResourceManager.h"

void GenericMidiDevice::init_grid_devices(uint8_t device_idx) {
  uint8_t grid_idx = 1;
  GridDeviceTrack gdt;

  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    gdt.init(EXT_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.ext_tracks[i]));
    add_track_to_grid(grid_idx, i, &gdt);
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
  oled_display.clearDisplay();
  oled_display.setFont();
  oled_display.setCursor(60, 10);
  oled_display.println("Peering...");
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
        DEBUG_PRINTLN("disconnect");
        DEBUG_PRINTLN(port);
  MidiUartClass *pmidi = _getMidiUart(port);
  if (!pmidi) { return; }
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
         turbo_light.set_speed(0, pmidi);
      }
      drivers[i]->disconnect(port - 1);
    }
  }
}

void MidiActivePeering::force_connect(uint8_t port, MidiDevice *driver) {
  MidiDevice **connected_dev;

  connected_dev = &connected_midi_devices[port - 1];

  midi_active_peering.disconnect(port);
  auto *pmidi = _getMidiUart(port);
  if (pmidi) {
    pmidi->device.init();
    pmidi->device.set_name(driver->name);
    pmidi->device.set_id(driver->id);
  }
  driver->init_grid_devices(port - 1);

  *connected_dev = driver;
}

static void probePort(uint8_t port, MidiDevice *drivers[], size_t nr_drivers,
                      MidiDevice **active_device, uint8_t* resource_buf) {
  MidiUartClass *pmidi = _getMidiUart(port);
  auto *pmidi_class = _getMidiClass(port);
  if (!pmidi || !pmidi_class)
    return;
  uint8_t id = pmidi->device.get_id();
  oled_display.setTextColor(WHITE, BLACK);
  if (id != DEVICE_NULL && pmidi->recvActiveSenseTimer > 300 &&
      pmidi->speed > 31250) {


    if ((port == UART1_PORT && MidiClock.uart_clock_recv == pmidi) || (port == UART2_PORT && MidiClock.uart_clock_recv == pmidi)) {
      //Disable MidiClock/Transport on disconnected port.
      MidiClock.uart_clock_recv = nullptr;
      MidiClock.init();
    }
    pmidi->set_speed((uint32_t)31250);
    DEBUG_PRINTLN("disconnecting");
    for (size_t i = 0; i < nr_drivers; ++i) {
      if (drivers[i]->connected)
        drivers[i]->disconnect(port - 1);
    }
   // reset MidiID to none
    pmidi->device.init();
    // reset connected device to /dev/null
    *active_device = &null_midi_device;
  } else if (id == DEVICE_NULL && pmidi->recvActiveSenseTimer < 100) {
    bool probe_success = false;
    for (size_t i = 0; i < nr_drivers; ++i) {

      MidiIDSysexListener.setup(pmidi_class);

      auto oldfont = oled_display.getFont();
      prepare_display(resource_buf);
      uint8_t* icon = drivers[i]->icon();
      if (icon) {
        oled_display.drawBitmap(14, 8, icon, 34, 42, WHITE);
      }
      mcl_gui.delay_progress(0);
      for (uint8_t probe_retry = 0; probe_retry < 6 && !probe_success;
           ++probe_retry) {
        DEBUG_PRINTLN("probing...");
        probe_success = drivers[i]->probe();
      } // for retries

      MidiIDSysexListener.cleanup();
      oled_display.setFont(oldfont);

      if (probe_success) {
        pmidi->device.set_id(drivers[i]->id);
        pmidi->device.set_name(drivers[i]->name);
        drivers[i]->init_grid_devices(port - 1);
        *active_device = drivers[i];
        //Re-enable MidiClock/Transport recv
        midi_setup.cfg_clock_recv();
        break;
      }
    } // for drivers
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

bool usb_set_speed = true;

void MidiActivePeering::run() {
  byte resource_buf[RM_BUFSIZE];
  resource_loaded = false;

  //Setting USB turbo speed too early can cause OS upload to fail
#ifndef DEBUGMODE
  if (turbo_light.tmSpeeds[turbo_light.lookup_speed(mcl_cfg.usb_turbo)] != MidiUartUSB.speed && slowclock > 4000 && usb_set_speed) {
     turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.usb_turbo), MidiUSB.uart);
     usb_set_speed = false;
  }
#endif

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
