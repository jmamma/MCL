#include "MidiActivePeering.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiID.h"
#include "MidiIDSysex.h"
#include "MidiUart.h"
#include "MidiSetup.h"
#include "DeviceManager.h"
#include "TurboLight.h"
#include "ResourceManager.h"
#include "../Drivers/DriverRegistry.h"
#include "../Drivers/MidiDevice.h"

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
#ifdef PLATFORM_TBD
  else if (port == UARTP4_PORT)
    ret = &MidiUartP4;
#endif
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
#ifdef PLATFORM_TBD
  else if (port == UARTP4_PORT)
    ret = &MidiP4;
#endif
  return ret;
}

static uint8_t portToLogicalIdx(uint8_t port) {
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT)
    return 1;
#endif
  if (port == UARTUSB_PORT)
    return (mcl_cfg.usb_device >= 2) ? 1 : 0;
  return port - 1;
}

static bool resource_loaded = false;
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

static void disconnect_driver_list(DriverRegistry::DriverList drivers,
                                   uint8_t device_idx, uint8_t port,
                                   MidiUartClass *pmidi) {
  const bool reset_turbo = device_manager.port_is_elektron(port);

  for (size_t i = 0; i < drivers.count; ++i) {
    MidiDevice *driver = drivers.items[i];
    if (!driver->connected) continue;
    if (reset_turbo) turbo_light.set_speed(0, pmidi);
    driver->disconnect(device_idx);
  }
}

static DriverRegistry::DriverList drivers_for_slot(uint8_t slot_idx,
                                                   bool force_generic) {
  if (force_generic) return DriverRegistry::generic_drivers();
  return slot_idx == SLOT_MD ? DriverRegistry::md_drivers()
                             : DriverRegistry::elektron_drivers();
}

void MidiActivePeering::disconnect(uint8_t port) {
  DEBUG_PRINTLN("disconnect");
  DEBUG_PRINTLN(port);
  MidiUartClass *pmidi = _getMidiUart(port);
  if (!pmidi) {
    return;
  }
  DriverRegistry::DriverList drivers = DriverRegistry::generic_drivers();
  uint8_t device_idx;
  if (port == UART1_PORT) {
    drivers = DriverRegistry::md_drivers();
    device_idx = 0;
  } else if (port == UART2_PORT) {
    drivers = DriverRegistry::elektron_drivers();
    device_idx = 1;
  } else if (port == UARTUSB_PORT) {
    // USB port can host either MD-slot or ELEKT-slot drivers.
    drivers = DriverRegistry::md_drivers();
    device_idx = portToLogicalIdx(port);
    disconnect_driver_list(DriverRegistry::elektron_drivers(), device_idx,
                           port, pmidi);
#ifdef PLATFORM_TBD
  } else if (port == UARTP4_PORT) {
    drivers = DriverRegistry::generic_drivers();
    device_idx = portToLogicalIdx(port);
#endif
  } else {
    return;
  }
  disconnect_driver_list(drivers, device_idx, port, pmidi);
  device_manager.detach_port(port);
}

void MidiActivePeering::force_connect(uint8_t port, MidiDevice *driver) {
  if (port < UART1_PORT || port > MIDI_PORT_COUNT) return;

  disconnect(port);
  auto *pmidi = _getMidiUart(port);
  if (pmidi) {
    pmidi->device.init();
    pmidi->device.set_name(driver->name);
    pmidi->device.set_id(driver->id);
  }
  driver->on_connection(portToLogicalIdx(port));

  device_manager.attach_port(port, driver);
}

static void probePort(uint8_t port, DriverRegistry::DriverList drivers,
                      uint8_t *resource_buf) {
  MidiUartClass *pmidi = _getMidiUart(port);
  auto *pmidi_class = _getMidiClass(port);
  if (!pmidi || !pmidi_class)
    return;
  uint8_t id = pmidi->device.get_id();
  oled_display.setTextColor(WHITE, BLACK);
  if (id != DEVICE_NULL && port != UARTUSB_PORT &&
#ifdef PLATFORM_TBD
      port != UARTP4_PORT &&
#endif
      pmidi->recvActiveSenseTimer > 300 && pmidi->speed > 31250) {

    if ((port == UART1_PORT && MidiClock.uart_clock_recv == pmidi) ||
        (port == UART2_PORT && MidiClock.uart_clock_recv == pmidi)) {
      // Disable MidiClock/Transport on disconnected port.
      MidiClock.uart_clock_recv = nullptr;
      MidiClock.init();
    }
    pmidi->set_speed((uint32_t)31250);
    DEBUG_PRINTLN("disconnecting");
    for (size_t i = 0; i < drivers.count; ++i) {
      MidiDevice *driver = drivers.items[i];
      if (driver->connected)
        driver->disconnect(portToLogicalIdx(port));
    }
    // reset MidiID to none
    pmidi->device.init();
    // reset connected device to /dev/null
    device_manager.detach_port(port);
  } else if (id == DEVICE_NULL && pmidi->recvActiveSenseTimer < 100) {
    bool probe_success = false;
    for (size_t i = 0; i < drivers.count; ++i) {
      MidiDevice *driver = drivers.items[i];

      MidiIDSysexListener.setup(pmidi_class);

      auto oldfont = oled_display.getFont();
      prepare_display(resource_buf);
      uint8_t *icon = driver->icon();
      if (icon) {
        oled_display.drawBitmap(14, 8, icon, 34, 24, WHITE);
      }
      mcl_gui.delay_progress(0);
      for (uint8_t probe_retry = 0; probe_retry < 6 && !probe_success;
           ++probe_retry) {
        DEBUG_PRINTLN("probing...");
        driver->in_probe = true;
        probe_success = driver->probe();
        driver->in_probe = false;
      } // for retries

      MidiIDSysexListener.cleanup();
      oled_display.setFont(oldfont);

      if (probe_success) {
        pmidi->device.set_id(driver->id);
        pmidi->device.set_name(driver->name);
        driver->on_connection(portToLogicalIdx(port));
        device_manager.attach_port(port, driver);
        // Re-enable MidiClock/Transport recv
        midi_setup.cfg_clock_recv();
        break;
      }
    } // for drivers
  }
}

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

  auto probe_slot = [&](uint8_t slot_idx, const PortSlot &slot) {
    if (!slot.port || slot.off) return;
    // GENER replaces the slot's driver list when this UART is set to GENER
    bool is_gener = (slot.port == UART1_PORT && mcl_cfg.uart1_device == 0) ||
                    (slot.port == UART2_PORT && mcl_cfg.uart2_device == 0);
    probePort(slot.port, drivers_for_slot(slot_idx, is_gener), resource_buf);
  };

  probe_slot(SLOT_MD, s[SLOT_MD]);
#ifdef EXT_TRACKS
  probe_slot(SLOT_ELEKT, s[SLOT_ELEKT]);
  if (resource_loaded) {
    // XXX restoring resources after the peering display doesn't work yet.
    GUI.currentPage()->init();
    resource_loaded = false;
  }
#endif
  device_manager.update_active_slots();
}
