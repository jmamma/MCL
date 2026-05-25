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
         device->connected && device->supports_ui();
}

inline gui_event_t ui_slot_entry_event(const gui_event_t *event,
                                       uint8_t slot) {
  gui_event_t slot_event = *event;
  if (slot == DeviceManager::UI_SLOT_PRIMARY) {
    slot_event.source = ButtonsClass::BUTTON2;
  } else if (slot == DeviceManager::UI_SLOT_SECONDARY) {
    slot_event.source = ButtonsClass::TBD_BUTTON_TR;
  }
  return slot_event;
}
#endif

} // namespace

DeviceManager device_manager;

DeviceManager::DeviceManager() {
  for (uint8_t i = 0; i < MIDI_PORT_COUNT; ++i) {
    physical_[i] = nullptr;
    logical_idx_[i] = LOGICAL_SLOT_NONE;
  }
  primary_ = nullptr;
  secondary_ = nullptr;
}

MidiDevice *DeviceManager::device_for_port(uint8_t port) const {
  if (port >= UART1_PORT && port <= MIDI_PORT_COUNT) {
    return nonnull(physical_[port - 1]);
  }
  return &null_midi_device;
}

uint8_t DeviceManager::logical_idx_for_port(uint8_t port) const {
  if (port >= UART1_PORT && port <= MIDI_PORT_COUNT) {
    return logical_idx_[port - 1];
  }
  return LOGICAL_SLOT_NONE;
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

void DeviceManager::attach_port(uint8_t port, MidiDevice *device,
                                uint8_t logical_idx) {
  set_device_for_port(port, device);
  if (port >= UART1_PORT && port <= MIDI_PORT_COUNT) {
    logical_idx_[port - 1] =
        nonnull(device) == &null_midi_device ? LOGICAL_SLOT_NONE : logical_idx;
  }
  update_active_slots();
}

void DeviceManager::detach_port(uint8_t port) {
  attach_port(port, &null_midi_device, LOGICAL_SLOT_NONE);
}

void DeviceManager::on_forwarded_cc(MidiUartClass *uart, uint8_t *msg) {
  if (uart == nullptr) {
    return;
  }
#if defined(__AVR__)
  MidiDevice *primary = primary_;
  if (primary != nullptr && primary->uart == uart) {
    primary->on_forwarded_cc(msg);
    return;
  }

  MidiDevice *secondary = secondary_;
  if (secondary != nullptr && secondary->uart == uart) {
    secondary->on_forwarded_cc(msg);
    return;
  }
#else
  for (uint8_t i = 0; i < MIDI_PORT_COUNT; ++i) {
    MidiDevice *device = nonnull(physical_[i]);
    if (device != &null_midi_device && device->uart == uart) {
      device->on_forwarded_cc(msg);
      return;
    }
  }
#endif
}

void DeviceManager::update_active_slots() {
#ifdef PLATFORM_TBD
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);
  primary_ =
      s[SLOT_MD].port ? device_for_port(s[SLOT_MD].port) : &null_midi_device;
  secondary_ =
      s[SLOT_ELEKT].port ? device_for_port(s[SLOT_ELEKT].port) : &null_midi_device;
  // USB GENER maps to the secondary active slot.
  if (mcl_cfg.usb_device == 3) secondary_ = device_for_port(UARTUSB_PORT);
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
#else
  uint8_t primary_port =
      (mcl_cfg.usb_device == 1) ? UARTUSB_PORT : UART1_PORT;
  uint8_t secondary_port = (mcl_cfg.usb_device == 2 ||
                            mcl_cfg.usb_device == 3)
                               ? UARTUSB_PORT
                               : UART2_PORT;
  primary_ = device_for_port(primary_port);
  secondary_ = device_for_port(secondary_port);
#endif
}

MidiDevice *DeviceManager::primary_device() const {
  return nonnull(primary_);
}

MidiDevice *DeviceManager::secondary_device() const {
  return nonnull(secondary_);
}

MidiDevice *DeviceManager::device_for_idx(DeviceIdx device_idx) const {
  if (device_idx == DeviceIdx::Secondary) {
    return secondary_device();
  }
  return device_idx == DeviceIdx::Primary ? primary_device()
                                          : &null_midi_device;
}

DeviceContext DeviceManager::primary_context() const {
  return DeviceContext::primary(primary_device());
}

DeviceContext DeviceManager::secondary_context() const {
  return DeviceContext::secondary(secondary_device());
}

DeviceContext DeviceManager::context_for_device(DeviceIdx device_idx) const {
  return DeviceContext::for_device(device_for_idx(device_idx), device_idx);
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

bool DeviceManager::enter_ui_slot_tap(uint8_t slot, gui_event_t *event) {
  if (event == nullptr || !EVENT_BUTTON(event) ||
      event->mask != EVENT_BUTTON_PRESSED) {
    return false;
  }

  uint8_t resolved_slot = slot;
  MidiDevice *device = nullptr;
  if (slot == UI_SLOT_PRIMARY) {
    device = primary_device();
  } else if (slot == UI_SLOT_SECONDARY) {
    device = secondary_device();
    MidiDevice *primary = primary_device();
    if (!ui_device_available(device) && ui_device_available(primary)) {
      device = primary;
      resolved_slot = UI_SLOT_PRIMARY;
    }
  } else {
    return false;
  }

  device = nonnull(device);
  if (device == &null_midi_device) return false;

  if (active_ui_device_ && active_ui_device_ != device) {
    notify_ui_slot_button(active_ui_device_, active_ui_slot_, false);
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  } else if (active_ui_device_ == device && active_ui_slot_ != resolved_slot) {
    notify_ui_slot_button(device, active_ui_slot_, false);
  }

  gui_event_t entry_event = ui_slot_entry_event(event, resolved_slot);
  if (!device->enter_ui(&entry_event)) return false;
  if (device->is_ui_active()) {
    active_ui_device_ = device;
    active_ui_slot_ = resolved_slot;
  } else if (active_ui_device_ == device) {
    active_ui_device_ = nullptr;
    active_ui_slot_ = UI_SLOT_NONE;
  }
  return true;
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
    if (!ui_device_available(device) && ui_device_available(primary)) {
      device = primary;
      resolved_slot = UI_SLOT_PRIMARY;
    }
  } else {
    return false;
  }

  device = nonnull(device);
  if (device == &null_midi_device) return false;

  if (active_ui_device_ == nullptr && device->is_ui_active()) {
    active_ui_device_ = device;
    active_ui_slot_ = resolved_slot;
  } else if (active_ui_device_ == device && active_ui_slot_ == UI_SLOT_NONE &&
             device->is_ui_active()) {
    active_ui_slot_ = resolved_slot;
  }

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

  gui_event_t entry_event = ui_slot_entry_event(event, resolved_slot);
  if (!device->enter_ui(&entry_event)) return false;
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

bool DeviceManager::is_ui_slot_active(uint8_t slot) const {
  uint8_t resolved_slot = slot;
  MidiDevice *device = nullptr;
  if (slot == UI_SLOT_PRIMARY) {
    device = primary_device();
  } else if (slot == UI_SLOT_SECONDARY) {
    device = secondary_device();
    MidiDevice *primary = primary_device();
    if (!ui_device_available(device) && ui_device_available(primary)) {
      device = primary;
      resolved_slot = UI_SLOT_PRIMARY;
    }
  } else {
    return false;
  }

  device = nonnull(device);
  if (device == &null_midi_device || !device->is_ui_active()) return false;
  if (active_ui_device_ == nullptr) return true;
  return active_ui_device_ == device &&
         (active_ui_slot_ == resolved_slot || active_ui_slot_ == UI_SLOT_NONE);
}

bool DeviceManager::notify_active_ui_button(gui_event_t *event) {
  if (event == nullptr || !EVENT_BUTTON(event) || active_ui_device_ == nullptr) {
    return false;
  }
  const bool pressed = event->mask == EVENT_BUTTON_PRESSED;
  const bool released = event->mask == EVENT_BUTTON_RELEASED;
  if (!pressed && !released) return false;
  if (!active_ui_device_->is_ui_active()) return false;

  notify_ui_slot_button(active_ui_device_, active_ui_slot_, pressed);
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

bool DeviceManager::is_ui_collapsed() const {
  if (active_ui_device_) return active_ui_device_->is_ui_collapsed();
  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  return (primary && primary->is_ui_collapsed()) ||
         (secondary && secondary->is_ui_collapsed());
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
