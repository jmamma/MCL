#include "MidiActivePeering.h"
#include "MCLGUI.h"
#include "MidiID.h"
#include "MidiIDSysex.h"
#include "MidiUart.h"
#include "MidiSetup.h"
#include "TurboLight.h"
#include "ResourceManager.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/A4/A4.h"
#include "../Drivers/MNM/MNM.h"

/// It is the caller's responsibility to check for null MidiUart device
static MidiUartClass *_getMidiUart(uint8_t port) {
  MidiUartClass *ret = nullptr;
  if (port == UART1_PORT)
    ret = &MidiUart;
#ifdef EXT_TRACKS
  else if (port == UART2_PORT)
    ret = &MidiUart2;
#endif
  else if (port == UARTUSB_PORT)
    ret = &MidiUartUSB;
  return ret;
}

/// It is the caller's responsibility to check for null MidiClass device
static MidiClass *_getMidiClass(uint8_t port) {
  MidiClass *ret = nullptr;
  if (port == UART1_PORT)
    ret = &Midi;
#ifdef EXT_TRACKS
  else if (port == UART2_PORT)
    ret = &Midi2;
#endif
  else if (port == UARTUSB_PORT)
    ret = &MidiUSB;
  return ret;
}

static uint8_t portToLogicalIdx(uint8_t port) {
  if (port == UARTUSB_PORT)
    return (mcl_cfg.usb_device >= 2) ? 1 : 0;
  return port - 1;
}

static bool resource_loaded = false;
static size_t resource_size = 0;
static void prepare_display(uint8_t *buf) {
  oled_display.clearDisplay();
  oled_display.setFont();
  oled_display.setCursor(60, 10);
  mcl_println_P(mclstr_peering);
  if (!resource_loaded) {
    R.Clear();
    R.use_icons_device();
    resource_loaded = true;
  }
}

// the general probe accept whatever devices.
static bool midi_device_setup(uint8_t port) { return true; }

static MidiDevice *port1_drivers[] = { &MD };

static MidiDevice *port2_drivers[] = {
    &MNM,
    &Analog4,
    &generic_midi_device,
};

static MidiDevice *generic_drivers[] = {
    &generic_midi_device,
};

static MidiDevice *connected_midi_devices[3] = {&null_midi_device,
                                                &null_midi_device,
                                                &null_midi_device};

void MidiActivePeering::disconnect(uint8_t port) {
  DEBUG_PRINTLN("disconnect");
  DEBUG_PRINTLN(port);
  MidiUartClass *pmidi = _getMidiUart(port);
  if (!pmidi) {
    return;
  }
  MidiDevice **drivers;
  uint8_t nr_drivers = 1;
  uint8_t device_idx;
  if (port == UART1_PORT) {
    drivers = port1_drivers;
    device_idx = 0;
  } else if (port == UART2_PORT) {
    drivers = port2_drivers;
    nr_drivers = 3;
    device_idx = 1;
  } else if (port == UARTUSB_PORT) {
    // USB port can host either port1 or port2 drivers
    drivers = port1_drivers;
    nr_drivers = 1;
    device_idx = portToLogicalIdx(port);
    // Also try port2 drivers
    for (size_t i = 0; i < countof(port2_drivers); ++i) {
      if (port2_drivers[i]->connected) {
        if (midi_active_peering.get_device(port)->asElektronDevice()) {
          turbo_light.set_speed(0, pmidi);
        }
        port2_drivers[i]->disconnect(device_idx);
      }
    }
  } else {
    return;
  }
  for (size_t i = 0; i < nr_drivers; ++i) {
    if (drivers[i]->connected) {
      if (midi_active_peering.get_device(port)->asElektronDevice()) {
        turbo_light.set_speed(0, pmidi);
      }
      drivers[i]->disconnect(device_idx);
    }
  }
}

void MidiActivePeering::force_connect(uint8_t port, MidiDevice *driver) {
  if (port < 1 || port > 3) return;
  MidiDevice **connected_dev;

  connected_dev = &connected_midi_devices[port - 1];

  midi_active_peering.disconnect(port);
  auto *pmidi = _getMidiUart(port);
  if (pmidi) {
    pmidi->device.init();
    pmidi->device.set_name(driver->name);
    pmidi->device.set_id(driver->id);
  }
  driver->on_connection(portToLogicalIdx(port));

  *connected_dev = driver;
  update_dev_cache();
}

static void probePort(uint8_t port, MidiDevice *drivers[], size_t nr_drivers,
                      MidiDevice **active_device, uint8_t *resource_buf) {
  MidiUartClass *pmidi = _getMidiUart(port);
  auto *pmidi_class = _getMidiClass(port);
  if (!pmidi || !pmidi_class)
    return;
  uint8_t id = pmidi->device.get_id();
  oled_display.setTextColor(WHITE, BLACK);
  if (id != DEVICE_NULL && port != UARTUSB_PORT &&
      pmidi->recvActiveSenseTimer > 300 && pmidi->speed > 31250) {

    if ((port == UART1_PORT && MidiClock.uart_clock_recv == pmidi) ||
        (port == UART2_PORT && MidiClock.uart_clock_recv == pmidi)) {
      // Disable MidiClock/Transport on disconnected port.
      MidiClock.uart_clock_recv = nullptr;
      MidiClock.init();
    }
    pmidi->set_speed((uint32_t)31250);
    DEBUG_PRINTLN("disconnecting");
    for (size_t i = 0; i < nr_drivers; ++i) {
      if (drivers[i]->connected)
        drivers[i]->disconnect(portToLogicalIdx(port));
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
      uint8_t *icon = drivers[i]->icon();
      if (icon) {
        oled_display.drawBitmap(14, 8, icon, 34, 24, WHITE);
      }
      mcl_gui.delay_progress(0);
      for (uint8_t probe_retry = 0; probe_retry < 6 && !probe_success;
           ++probe_retry) {
        DEBUG_PRINTLN("probing...");
        drivers[i]->in_probe = true;
        probe_success = drivers[i]->probe();
        drivers[i]->in_probe = false;
      } // for retries

      MidiIDSysexListener.cleanup();
      oled_display.setFont(oldfont);

      if (probe_success) {
        pmidi->device.set_id(drivers[i]->id);
        pmidi->device.set_name(drivers[i]->name);
        drivers[i]->on_connection(portToLogicalIdx(port));
        *active_device = drivers[i];
        // Re-enable MidiClock/Transport recv
        midi_setup.cfg_clock_recv();
        break;
      }
    } // for drivers
  }
}

MidiDevice *MidiActivePeering::get_device(uint8_t port) {
  if (port >= 1 && port <= 3) {
    return connected_midi_devices[port - 1];
  }
  return &null_midi_device;
}

void MidiActivePeering::update_dev_cache() {
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);
  dev1 = s[SLOT_MD].port    ? get_device(s[SLOT_MD].port)    : &null_midi_device;
  dev2 = s[SLOT_ELEKT].port ? get_device(s[SLOT_ELEKT].port) : &null_midi_device;
  // USB GENER maps to dev2 slot
  if (mcl_cfg.usb_device == 3) dev2 = get_device(UARTUSB_PORT);
}

NullMidiDevice::NullMidiDevice()
    : MidiDevice(nullptr, "  ", DEVICE_NULL, false) {}

bool usb_set_speed = true;

void MidiActivePeering::run() {
  byte resource_buf[RM_BUFSIZE];
  resource_loaded = false;

  // Setting USB turbo speed too early can cause OS upload to fail
#if defined(__AVR__) && !defined(DEBUGMODE)
  if (turbo_light.tmSpeeds[turbo_light.lookup_speed(mcl_cfg.usb_turbo_speed)] !=
          MidiUartUSB.speed &&
      read_clock_ms() > 4000 && usb_set_speed) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.usb_turbo_speed),
                          MidiUSB.uart);
    usb_set_speed = false;
  }
#endif

  PortSlot s[SLOT_COUNT];
  resolve_slots(s);

  auto probe_slot = [&](const PortSlot &slot,
                        MidiDevice **drivers, uint8_t nr) {
    if (!slot.port || slot.off) return;
    // GENER replaces the slot's driver list when this UART is set to GENER
    bool is_gener = (slot.port == UART1_PORT && mcl_cfg.uart1_device == 0) ||
                    (slot.port == UART2_PORT && mcl_cfg.uart2_device == 0);
    if (is_gener) { drivers = generic_drivers; nr = 1; }
    probePort(slot.port, drivers, nr,
              &connected_midi_devices[slot.port - 1], resource_buf);
  };

  probe_slot(s[SLOT_MD], port1_drivers, countof(port1_drivers));
#ifdef EXT_TRACKS
  probe_slot(s[SLOT_ELEKT], port2_drivers, countof(port2_drivers));
  if (resource_loaded) {
    // XXX doesn't work yet
    // R.Restore(resource_buf, resource_size);
    GUI.currentPage()->init();
    resource_loaded = false;
  }
#endif
  update_dev_cache();
}
