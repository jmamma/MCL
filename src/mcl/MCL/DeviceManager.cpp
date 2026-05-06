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

void DeviceManager::set_device_for_port(uint8_t port, MidiDevice *device) {
  if (port < UART1_PORT || port > UARTUSB_PORT) return;
#ifdef PLATFORM_TBD
  if (active_ui_device_ == physical_[port - 1]) {
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
  }
#endif
  physical_[port - 1] = nonnull(device);
  update_active_slots();
}

void DeviceManager::update_active_slots() {
  PortSlot s[SLOT_COUNT];
  resolve_slots(s);
  dev1_ = s[SLOT_MD].port ? device_for_port(s[SLOT_MD].port) : &null_midi_device;
  dev2_ = s[SLOT_ELEKT].port ? device_for_port(s[SLOT_ELEKT].port) : &null_midi_device;
  // USB GENER maps to the secondary active slot.
  if (mcl_cfg.usb_device == 3) dev2_ = device_for_port(UARTUSB_PORT);
#ifdef PLATFORM_TBD
  if (active_ui_device_ && active_ui_device_ != dev1_ &&
      active_ui_device_ != dev2_) {
    active_ui_device_ = nullptr;
  }
#endif
}

MidiDevice *DeviceManager::dev1() const { return nonnull(dev1_); }
MidiDevice *DeviceManager::dev2() const { return nonnull(dev2_); }

#ifdef PLATFORM_TBD
void DeviceManager::ui_loop() {
  MidiDevice *dev1 = this->dev1();
  MidiDevice *dev2 = this->dev2();
  if (dev2 == dev1) dev2 = nullptr;
  if (dev1) dev1->ui_loop();
  if (dev2) dev2->ui_loop();
}

bool DeviceManager::handle_ui_event(gui_event_t *event) {
  if (active_ui_device_) {
    if (active_ui_device_->handle_ui_event(event)) return true;
    if (!active_ui_device_->is_ui_active()) active_ui_device_ = nullptr;
    return false;
  }

  MidiDevice *dev1 = this->dev1();
  MidiDevice *dev2 = this->dev2();
  if (dev2 == dev1) dev2 = nullptr;
  if (dev1 && dev1->handle_ui_event(event)) {
    if (dev1->is_ui_active()) active_ui_device_ = dev1;
    return true;
  }
  if (dev2 && dev2->handle_ui_event(event)) {
    if (dev2->is_ui_active()) active_ui_device_ = dev2;
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

  MidiDevice *dev1 = this->dev1();
  MidiDevice *dev2 = this->dev2();
  if (dev2 == dev1) dev2 = nullptr;
  if (dev1 && dev1->enter_ui(event)) {
    if (dev1->is_ui_active()) active_ui_device_ = dev1;
    return true;
  }
  if (dev2 && dev2->enter_ui(event)) {
    if (dev2->is_ui_active()) active_ui_device_ = dev2;
    return true;
  }
  return false;
}

bool DeviceManager::is_ui_active() const {
  if (active_ui_device_) return active_ui_device_->is_ui_active();
  MidiDevice *dev1 = this->dev1();
  MidiDevice *dev2 = this->dev2();
  if (dev2 == dev1) dev2 = nullptr;
  return (dev1 && dev1->is_ui_active()) ||
         (dev2 && dev2->is_ui_active());
}

void DeviceManager::exit_ui() {
  if (active_ui_device_) {
    active_ui_device_->exit_ui();
    active_ui_device_ = nullptr;
    return;
  }

  MidiDevice *dev1 = this->dev1();
  MidiDevice *dev2 = this->dev2();
  if (dev2 == dev1) dev2 = nullptr;
  if (dev1) dev1->exit_ui();
  if (dev2) dev2->exit_ui();
}
#endif
