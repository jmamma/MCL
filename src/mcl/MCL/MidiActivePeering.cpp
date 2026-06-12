#include "MidiActivePeering.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiID.h"
#include "MidiIDSysex.h"
#include "Midi.h"
#include "MidiUart.h"
#include "MidiSetup.h"
#include "DeviceManager.h"
#include "TurboLight.h"
#include "ResourceManager.h"
#include "../Drivers/DriverRegistry.h"
#include "../Drivers/MidiDevice.h"

using DriverList = DriverRegistry::DriverList;
using DriverRegistry::elektron_drivers;
using DriverRegistry::generic_drivers;
using DriverRegistry::md_drivers;

static uint8_t portToLogicalIdx(uint8_t port) {
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT)
    return (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) ? 0 : 1;
#endif
  if (port == UARTUSB_PORT)
    return (mcl_cfg.usb_device >= 2) ? 1 : 0;
  return port - 1;
}

static bool resource_loaded = false;
static void prepare_display() {
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

static bool disconnect_driver_list(DriverList drivers, DeviceIdx device_idx,
                                   uint8_t port, MidiUartClass *pmidi) {
  bool disconnected_attached = false;
  MidiDevice *attached = device_manager.device_for_port(port);

  for (uint8_t i = 0; i < drivers.count; ++i) {
    MidiDevice *driver = drivers.items[i];
    if (driver != attached && driver->port != port) continue;
    if (!driver->connected && driver != attached) continue;
    if (driver->asElektronDevice()) turbo_light.set_speed(0, pmidi);
    driver->disconnect(device_idx);
    if (driver == attached) disconnected_attached = true;
  }
  return disconnected_attached;
}

static void disconnect_attached_device(uint8_t port, DeviceIdx device_idx,
                                       MidiUartClass *pmidi,
                                       bool disconnected_attached) NOINLINE();

static void disconnect_attached_device(uint8_t port, DeviceIdx device_idx,
                                       MidiUartClass *pmidi,
                                       bool disconnected_attached) {
  MidiDevice *attached = device_manager.device_for_port(port);
  if (attached != &null_midi_device && !disconnected_attached) {
    if (attached->asElektronDevice()) turbo_light.set_speed(0, pmidi);
    attached->disconnect(device_idx);
  }
}

static DriverList drivers_for_slot(uint8_t slot_idx, bool force_generic) {
  if (force_generic) return generic_drivers();
  return slot_idx == SLOT_MD ? md_drivers() : elektron_drivers();
}

void MidiActivePeering::disconnect(uint8_t port) {
  DEBUG_PRINTLN("disconnect");
  DEBUG_PRINTLN(port);
  MidiClass *pmidi_class = midi_class_for_port(port);
  if (!pmidi_class) {
    return;
  }
  MidiUartClass *pmidi = pmidi_class->uart;
  if (!pmidi) {
    return;
  }
#ifdef PLATFORM_TBD
  if (port == UARTP4_PORT) {
    disconnect_driver_list(generic_drivers(), DeviceIdx::Primary, port, pmidi);
    disconnect_driver_list(generic_drivers(), DeviceIdx::Secondary, port, pmidi);
    MidiDevice *attached = device_manager.device_for_port(port);
    if (attached != &null_midi_device) {
      attached->disconnect(DeviceIdx::Primary);
      attached->disconnect(DeviceIdx::Secondary);
    }
    device_manager.detach_port(port);
    return;
  }
#endif
  DriverList drivers = generic_drivers();
  DeviceIdx device_idx =
      static_cast<DeviceIdx>(device_manager.logical_idx_for_port(port));
  if (device_idx == DeviceIdx::None) {
    device_idx = static_cast<DeviceIdx>(portToLogicalIdx(port));
  }
  bool disconnected_attached = false;
  if (port == UART1_PORT) {
    drivers = md_drivers();
  } else if (port == UART2_PORT) {
    drivers = elektron_drivers();
  } else if (port == UARTUSB_PORT) {
    // USB port can host either MD-slot or ELEKT-slot drivers.
    drivers = md_drivers();
    disconnected_attached =
        disconnect_driver_list(elektron_drivers(), device_idx, port, pmidi);
  } else {
    return;
  }
  disconnected_attached |=
      disconnect_driver_list(drivers, device_idx, port, pmidi);
  disconnect_attached_device(port, device_idx, pmidi, disconnected_attached);
  device_manager.detach_port(port);
}

void MidiActivePeering::force_connect(uint8_t port, MidiDevice *driver) {
  if (port < UART1_PORT || port > MIDI_PORT_COUNT) return;
  if (!driver) driver = &null_midi_device;

  disconnect(port);
  MidiClass *pmidi_class = midi_class_for_port(port);
  MidiUartClass *pmidi = pmidi_class ? pmidi_class->uart : nullptr;
  if (pmidi) {
    pmidi->device.init();
    pmidi->device.set_name(driver->name);
    pmidi->device.set_id(driver->id);
  }
  uint8_t logical_idx = portToLogicalIdx(port);
  driver->on_connection(static_cast<DeviceIdx>(logical_idx));

  device_manager.attach_port(port, driver, logical_idx);
}

static void probePort(uint8_t port, DriverList drivers) {
  auto *pmidi_class = midi_class_for_port(port);
  if (!pmidi_class)
    return;
  MidiUartClass *pmidi = pmidi_class->uart;
  if (!pmidi)
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
    DeviceIdx device_idx =
        static_cast<DeviceIdx>(device_manager.logical_idx_for_port(port));
    if (device_idx == DeviceIdx::None) {
      device_idx = static_cast<DeviceIdx>(portToLogicalIdx(port));
    }
    bool disconnected_attached =
        disconnect_driver_list(drivers, device_idx, port, pmidi);
    disconnect_attached_device(port, device_idx, pmidi, disconnected_attached);
    // reset MidiID to none
    pmidi->device.init();
    // reset connected device to /dev/null
    device_manager.detach_port(port);
  } else if (id == DEVICE_NULL && pmidi->recvActiveSenseTimer < 100) {
    bool probe_success = false;
    for (uint8_t i = 0; i < drivers.count; ++i) {
      MidiDevice *driver = drivers.items[i];

      MidiIDSysexListener.setup(pmidi_class);

      auto oldfont = oled_display.getFont();
      prepare_display();
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
        uint8_t logical_idx = portToLogicalIdx(port);
        driver->on_connection(static_cast<DeviceIdx>(logical_idx));
        device_manager.attach_port(port, driver, logical_idx);
        // Re-enable MidiClock/Transport recv
        midi_setup.cfg_clock_recv();
        break;
      }
    } // for drivers
  }
}

bool usb_set_speed = true;

#ifndef PLATFORM_TBD
static uint8_t avr_grid_y_device_cfg() {
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) return 0;
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT) return 1;
  return 2;
}

static uint8_t avr_grid_y_port() {
  return (mcl_cfg.usb_device == 2 || mcl_cfg.usb_device == 3) ? UARTUSB_PORT
                                                              : UART2_PORT;
}
#endif

void MidiActivePeering::run() {
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

#ifdef PLATFORM_TBD
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);

  if (s[SLOT_MD].port && !s[SLOT_MD].off) {
    bool is_gener = (s[SLOT_MD].port == UART1_PORT && mcl_cfg.uart1_device == 0) ||
                    (s[SLOT_MD].port == UART2_PORT && mcl_cfg.uart2_device == 0);
    probePort(s[SLOT_MD].port, drivers_for_slot(SLOT_MD, is_gener));
  }
#ifdef EXT_TRACKS
  if (s[SLOT_ELEKT].port && !s[SLOT_ELEKT].off) {
    bool is_gener = (s[SLOT_ELEKT].port == UART1_PORT && mcl_cfg.uart1_device == 0) ||
                    (s[SLOT_ELEKT].port == UART2_PORT && mcl_cfg.uart2_device == 0);
    probePort(s[SLOT_ELEKT].port, drivers_for_slot(SLOT_ELEKT, is_gener));
  }
#endif
#else
  uint8_t md_port = (mcl_cfg.usb_device == 1) ? UARTUSB_PORT : UART1_PORT;
  uint8_t ext_port = avr_grid_y_port();
  uint8_t md_device_cfg =
      (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD) ? 1 : 2;
  uint8_t ext_device_cfg = avr_grid_y_device_cfg();

  if (md_device_cfg != 2) {
    probePort(md_port, drivers_for_slot(SLOT_MD, md_device_cfg == 0));
  }
#ifdef EXT_TRACKS
  if (ext_device_cfg != 2) {
    probePort(ext_port, drivers_for_slot(SLOT_ELEKT, ext_device_cfg == 0));
  }
#endif
#endif
#ifdef EXT_TRACKS
  if (resource_loaded) {
    // XXX restoring resources after the peering display doesn't work yet.
    GUI.currentPage()->init();
    resource_loaded = false;
  }
#endif
  device_manager.update_active_slots();
}
