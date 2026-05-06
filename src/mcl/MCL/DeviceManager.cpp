#include "DeviceManager.h"

#include "../Drivers/MidiDevice.h"
#include "MCLSysConfig.h"
#include "MidiSetup.h"

namespace {

inline MidiDevice *nonnull(MidiDevice *device) {
  return device ? device : &null_midi_device;
}

} // namespace

DeviceManager device_manager;

MidiDevice *DeviceManager::device_for_port(uint8_t port) const {
  if (port >= UART1_PORT && port <= UARTUSB_PORT) {
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
  if (port < UART1_PORT || port > UARTUSB_PORT) return;
#ifdef PLATFORM_TBD
  if (active_ui_device_ == physical_[port - 1]) {
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
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
  if (active_ui_device_ && active_ui_device_ != primary_ &&
      active_ui_device_ != secondary_) {
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
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
    if (!active_ui_device_->is_ui_active()) active_ui_device_ = nullptr;
    return false;
  }

  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary && primary->handle_ui_event(event)) {
    if (primary->is_ui_active()) active_ui_device_ = primary;
    return true;
  }
  if (secondary && secondary->handle_ui_event(event)) {
    if (secondary->is_ui_active()) active_ui_device_ = secondary;
    return true;
  }
  return false;
}

bool DeviceManager::enter_ui(gui_event_t *event) {
  if (active_ui_device_) {
    if (!active_ui_device_->enter_ui(event)) return false;
    if (!active_ui_device_->is_ui_active()) active_ui_device_ = nullptr;
    return true;
  }

  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary && primary->enter_ui(event)) {
    if (primary->is_ui_active()) active_ui_device_ = primary;
    return true;
  }
  if (secondary && secondary->enter_ui(event)) {
    if (secondary->is_ui_active()) active_ui_device_ = secondary;
    return true;
  }
  return false;
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
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    return;
  }

  MidiDevice *primary = primary_device();
  MidiDevice *secondary = secondary_device();
  if (secondary == primary) secondary = nullptr;
  if (primary) primary->exit_ui();
  if (secondary) secondary->exit_ui();
}
#endif
