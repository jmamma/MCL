#include "DeviceManager.h"

#include "../Drivers/MidiDevice.h"
#include "MCLSysConfig.h"
#include "MidiSetup.h"

namespace {

inline MidiDevice *nonnull(MidiDevice *device) {
  return device ? device : &null_midi_device;
}

#ifdef PLATFORM_TBD
inline void notify_ui_slot_button(MidiDevice *device, uint8_t slot,
                                  bool pressed) {
  if (device == nullptr || device == &null_midi_device ||
      slot == DeviceManager::UI_SLOT_NONE) {
    return;
  }
  device->on_ui_slot_button(slot, pressed);
}

inline bool ui_device_available(MidiDevice *device) {
  return device != nullptr && device != &null_midi_device &&
         device->connected;
}
#endif

} // namespace

DeviceManager device_manager;

MidiDevice *DeviceManager::device_for_port(uint8_t port) const {
  if (port >= UART1_PORT && port <= MIDI_PORT_COUNT) {
    return nonnull(physical_[port - 1]);
  }
  return &null_midi_device;
}

bool DeviceManager::port_supports(uint8_t port,
                                  MidiDeviceCapability capability) const {
  return device_for_port(port)->supports_capability(capability);
}

bool DeviceManager::port_is_elektron(uint8_t port) const {
  return device_for_port(port)->asElektronDevice() != nullptr;
}

void DeviceManager::set_device_for_port(uint8_t port, MidiDevice *device) {
  if (port < UART1_PORT || port > MIDI_PORT_COUNT) return;
#ifdef PLATFORM_TBD
  if (active_ui_device_ && active_ui_device_ == physical_[port - 1]) {
    notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  }
#endif
  physical_[port - 1] = nonnull(device);
}

void DeviceManager::attach_port(uint8_t port, MidiDevice *device) {
  set_device_for_port(port, device);
  update_active_slots();
}

void DeviceManager::detach_port(uint8_t port) {
  attach_port(port, &null_midi_device);
}

void DeviceManager::update_active_slots() {
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);
  primary_ =
      s[SLOT_MD].port ? device_for_port(s[SLOT_MD].port) : &null_midi_device;
  secondary_ =
      s[SLOT_ELEKT].port ? device_for_port(s[SLOT_ELEKT].port) : &null_midi_device;
  // USB GENER maps to the secondary active slot.
  if (mcl_cfg.usb_device == 3) secondary_ = device_for_port(UARTUSB_PORT);
#ifdef PLATFORM_TBD
  MidiDevice *p4_device = device_for_port(UARTP4_PORT);
  if (p4_device != &null_midi_device) {
    if (s[SLOT_MD].port == UARTP4_PORT) {
      primary_ = p4_device;
    }
    if (s[SLOT_ELEKT].port == UARTP4_PORT ||
        s[SLOT_GENER].port == UARTP4_PORT) {
      secondary_ = p4_device;
    }
  }
  if (active_ui_device_ && active_ui_device_ != primary_ &&
      active_ui_device_ != secondary_) {
    notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  }
#endif
}

MidiDevice *DeviceManager::primary_device() const {
  return nonnull(primary_);
}

MidiDevice *DeviceManager::secondary_device() const {
  return nonnull(secondary_);
}

#ifdef PLATFORM_TBD
void DeviceManager::ui_loop() {
  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary) primary->ui_loop();
  if (secondary) secondary->ui_loop();
}

bool DeviceManager::handle_ui_event(gui_event_t *event) {
  if (active_ui_device_) {
    if (active_ui_device_->handle_ui_event(event)) return true;
    if (!active_ui_device_->is_ui_active()) {
      notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
      active_ui_device_ = nullptr;
      active_ui_slot_ = UI_SLOT_NONE;
    }
    return false;
  }

  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary && primary->handle_ui_event(event)) {
    if (primary->is_ui_active()) {
      active_ui_device_ = primary;
      active_ui_slot_ = UI_SLOT_PRIMARY;
    }
    return true;
  }
  if (secondary && secondary->handle_ui_event(event)) {
    if (secondary->is_ui_active()) {
      active_ui_device_ = secondary;
      active_ui_slot_ = UI_SLOT_SECONDARY;
    }
    return true;
  }
  return false;
}

bool DeviceManager::enter_ui(gui_event_t *event) {
  if (active_ui_device_) {
    return enter_ui(active_ui_device_, event);
  }

  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary && enter_ui(primary, event)) return true;
  if (secondary && enter_ui(secondary, event)) return true;
  return false;
}

bool DeviceManager::enter_ui(MidiDevice *device, gui_event_t *event) {
  device = nonnull(device);
  if (device == &null_midi_device) return false;
  if (active_ui_device_ && active_ui_device_ != device) {
    notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  }
  if (!device->enter_ui(event)) return false;
  if (device->is_ui_active()) {
    active_ui_device_ = device;
    active_ui_slot_ = UI_SLOT_NONE;
  } else if (active_ui_device_ == device) {
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  }
  return true;
}

bool DeviceManager::enter_ui_slot(uint8_t slot, gui_event_t *event,
                                  bool allow_toggle) {
  return handle_ui_slot_button(slot, event, allow_toggle);
}

bool DeviceManager::handle_ui_slot_button(uint8_t slot, gui_event_t *event,
                                          bool allow_toggle) {
  if (event == nullptr || !EVENT_BUTTON(event)) return false;
  const bool pressed = event->mask == EVENT_BUTTON_PRESSED;
  const bool released = event->mask == EVENT_BUTTON_RELEASED;
  if (!pressed && !released) return false;

  uint8_t resolved_slot = slot;
  MidiDevice *device = nullptr;
  if (slot == UI_SLOT_PRIMARY) {
    device = primary_device();
  } else if (slot == UI_SLOT_SECONDARY) {
    device = secondary_device();
    MidiDevice *primary = primary_device();
    if (!ui_device_available(device) && ui_device_available(primary) &&
        primary->port == UART1_PORT) {
      device = primary;
      resolved_slot = UI_SLOT_PRIMARY;
    }
  } else {
    return false;
  }

  device = nonnull(device);
  if (device == &null_midi_device) return false;

  if (released) {
    if (active_ui_device_ == device && active_ui_slot_ == resolved_slot &&
        device->is_ui_active()) {
      notify_ui_slot_button(device, resolved_slot, false);
    }
    return true;
  }

  if (active_ui_device_ == device && active_ui_slot_ == resolved_slot &&
      device->is_ui_active()) {
    if (allow_toggle) {
      notify_ui_slot_button(device, resolved_slot, false);
      device->exit_ui();
      active_ui_device_ = nullptr;
      active_ui_slot_ = UI_SLOT_NONE;
    } else {
      notify_ui_slot_button(device, resolved_slot, true);
    }
    return true;
  }

  if (active_ui_device_ && active_ui_device_ != device) {
    notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  } else if (active_ui_device_ == device && active_ui_slot_ != resolved_slot) {
    notify_ui_slot_button(device, active_ui_slot_, false);
  }

  if (!device->enter_ui(event)) return false;
  if (device->is_ui_active()) {
    active_ui_device_ = device;
    active_ui_slot_ = resolved_slot;
    notify_ui_slot_button(device, resolved_slot, true);
  } else if (active_ui_device_ == device) {
    notify_ui_slot_button(device, active_ui_slot_, false);
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  }
  return true;
}

bool DeviceManager::is_ui_active() const {
  if (active_ui_device_) return active_ui_device_->is_ui_active();
  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  return (primary && primary->is_ui_active()) ||
         (secondary && secondary->is_ui_active());
}

void DeviceManager::exit_ui() {
  if (active_ui_device_) {
    notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
    return;
  }

  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary) primary->exit_ui();
  if (secondary) secondary->exit_ui();
  active_ui_slot_ = UI_SLOT_NONE;
}
#endif
